#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Build strict GPOL v1 from the pinned Natural Earth 110m country ZIP (stdlib only)."""
import argparse
import hashlib
import math
import struct
import unicodedata
import zipfile
import zlib

SOURCE_SHA = '0f243aeac8ac6cf26f0417285b0bd33ac47f1b5bdb719fd3e0df37d03ea37110'

def vector(point):
    lon, lat = map(math.radians, point)
    return (math.cos(lat)*math.sin(lon), math.sin(lat), math.cos(lat)*math.cos(lon))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def area(points):
    vs=list(map(vector,points)); a=vs[0]
    result=sum(2*math.atan2(dot(a,cross(b,c)),1+dot(a,b)+dot(b,c)+dot(c,a)) for b,c in zip(vs[1:],vs[2:]))
    return (result+2*math.pi)%(4*math.pi)-2*math.pi

def encode(countries):
    """Input: [(ASCII name, [(hole flag, parent ring-local index, [(lon,lat),...])])]."""
    cr=bytearray();rr=bytearray();pp=bytearray();text=bytearray();rings=0;points=0
    for name,parts in countries:
        title=name.encode('ascii')
        if not 1<=len(title)<=95 or any(c<32 or c>126 for c in title): raise ValueError('name')
        cr+=struct.pack('<IIII',rings,len(parts),len(text),len(title));text+=title
        for hole,parent,raw in parts:
            coords=[]
            for lon,lat in raw:
                if not math.isfinite(lon+lat) or not -180.000000001<=lon<=180.000000001 or not -90<=lat<=90: raise ValueError('coordinate')
                q=(round(max(-180,min(180,lon))*1000000),round(lat*1000000))
                if not coords or q!=coords[-1]: coords.append(q)
            if coords and coords[-1]==coords[0]: coords.pop()
            if len(coords)<3: raise ValueError('short ring')
            if area([(x/1e6,y/1e6) for x,y in coords])<0: coords.reverse()
            first=struct.unpack_from('<I',cr,len(cr)-16)[0]
            rr+=struct.pack('<IIII',points,len(coords),int(hole),first+parent if hole else rings)
            pp+=b''.join(struct.pack('<ii',*p) for p in coords)
            points+=len(coords);rings+=1
    if not 1<=len(countries)<=4096 or rings>65536 or points>1000000 or len(text)>1048576: raise ValueError('limits')
    header=struct.pack('<4sHHIIIIII',b'GPOL',1,0,len(countries),rings,points,len(text),0,0)
    result=bytearray(header+cr+rr+pp+text)
    struct.pack_into('<I',result,28,zlib.crc32(result[32:],zlib.crc32(result[:28])))
    return bytes(result)

def read_dbf(data):
    n=struct.unpack_from('<I',data,4)[0];header,row=struct.unpack_from('<HH',data,8)
    fields=[];start=1
    for i in range(32,header-1,32):
        name=data[i:i+11].rstrip(b'\0').decode('ascii');length=data[i+16];fields.append((name,start,length));start+=length
    if start!=row or header+n*row>len(data): raise ValueError('DBF bounds')
    result=[]
    for i in range(n):
        record=data[header+i*row:header+(i+1)*row]
        if record[0]!=32: raise ValueError('deleted DBF record')
        result.append({name:record[pos:pos+length].decode('utf-8').strip(' \0') for name,pos,length in fields})
    return result

def read_source(path, attributes=None):
    raw=open(path,'rb').read()
    if hashlib.sha256(raw).hexdigest()!=SOURCE_SHA: raise ValueError('source hash mismatch')
    with zipfile.ZipFile(path) as z:
        base='ne_110m_admin_0_countries';dbf=read_dbf(z.read(base+'.dbf'));shp=z.read(base+'.shp')
    if struct.unpack_from('<II',shp,28)!=(1000,5): raise ValueError('expected Polygon shapefile')
    result=[];offset=100
    while offset<len(shp):
        _,words=struct.unpack_from('>II',shp,offset);offset+=8;record=shp[offset:offset+words*2];offset+=words*2
        if len(record)!=words*2 or struct.unpack_from('<I',record)[0]!=5: raise ValueError('record')
        n,p=struct.unpack_from('<II',record,36);starts=list(struct.unpack_from('<'+'I'*n,record,44))+[p]
        if len(record)!=44+4*n+16*p: raise ValueError('length')
        coords=[struct.unpack_from('<dd',record,44+4*n+16*i) for i in range(p)]
        rings=[];exterior=None
        for a,b in zip(starts,starts[1:]):
            points=coords[a:b]
            # ESRI exterior clockwise / holes counterclockwise, in source planar coordinates.
            signed=sum(x*v-u*y for (x,y),(u,v) in zip(points,points[1:]))
            hole=signed>0
            if not hole: exterior=len(rings)
            if exterior is None: raise ValueError('hole before exterior')
            rings.append((hole,exterior,points))
        row = dbf[len(result)]
        if attributes is not None:
            attributes.append(row)
        name=row['ADMIN']
        name=unicodedata.normalize('NFKD',name).encode('ascii','ignore').decode()
        result.append((name,rings))
    if len(result)!=len(dbf): raise ValueError('record count')
    return result

def label_anchor(parts, preferred):
    """Preserve the editorial point when it is on land, otherwise sample interior clearance."""
    def local(ring, origin):
        return [(origin + (x - origin + 180) % 360 - 180, y) for x, y in ring]

    def inside(point, ring):
        x, y = point
        return sum((v > y) != (w > y) and x < u + (t - u) * (y - v) / (w - v)
                   for (u, v), (t, w) in zip(ring, ring[1:] + ring[:1])) % 2

    def land(point, index, rings):
        return inside(point, rings[index]) and not any(
            hole and parent == index and inside(point, rings[j])
            for j, (hole, parent, _) in enumerate(parts))

    rings = [local(ring, preferred[0]) for _, _, ring in parts]
    for i, (hole, _, _) in enumerate(parts):
        if not hole and land(preferred, i, rings):
            return preferred

    index = max((i for i, (hole, _, _) in enumerate(parts) if not hole),
                key=lambda i: abs(area(parts[i][2])))
    origin = parts[index][2][0][0]
    rings = [local(ring, origin) for _, _, ring in parts]
    ring = rings[index]
    xs, ys = zip(*ring)
    lo, hi, bottom, top = min(xs), max(xs), min(ys), max(ys)
    scale = math.cos(math.radians((bottom + top) / 2))

    def clearance(point):
        x, y = point
        best = math.inf
        for j, (hole, parent, _) in enumerate(parts):
            if j != index and (not hole or parent != index):
                continue
            boundary = rings[j]
            for (u, v), (t, w) in zip(boundary, boundary[1:] + boundary[:1]):
                dx, dy = (t - u) * scale, w - v
                px, py = (x - u) * scale, y - v
                length = dx * dx + dy * dy
                f = max(0, min(1, (px * dx + py * dy) / length)) if length else 0
                best = min(best, (px - f * dx)**2 + (py - f * dy)**2)
        return best

    sample = ((lo + (hi-lo)*(x+.5)/24, bottom + (top-bottom)*(y+.5)/24)
              for x in range(24) for y in range(24))
    point = max((p for p in sample if land(p, index, rings)), key=clearance, default=None)
    if point is None:
        raise ValueError('no interior sample for country label')
    return (point[0] + 180) % 360 - 180, point[1]


def label_source(countries, attributes, pack):
    rows = ['// SPDX-License-Identifier: GPL-3.0-or-later',
            '// Copyright (c) 2026 Jack Willis',
            '// Generated by build_countries.py --labels from the pinned Natural Earth ZIP.',
            f'constexpr std::uint32_t kLabelEdition = 0x{struct.unpack_from("<I", pack, 28)[0]:08x};',
            'constexpr Anchor kCountryAnchors[] = {']
    for (_, parts), fields in zip(countries, attributes, strict=True):
        lon, lat = label_anchor(parts, (float(fields['LABEL_X']), float(fields['LABEL_Y'])))
        rows.append(f'    {{{round(lat*10000)}, {round(lon*10000)}, {int(fields["LABELRANK"])}}},')
    return '\n'.join(rows + ['};', ''])


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    parser.add_argument('output')
    parser.add_argument('--labels')
    args = parser.parse_args()
    attributes = []
    countries = read_source(args.source, attributes)
    data = encode(countries)
    if args.labels:
        with open(args.labels, 'w', encoding='ascii') as output:
            output.write(label_source(countries, attributes, data))
    with open(args.output, 'wb') as output:
        output.write(data)
    print(len(data), hashlib.sha256(data).hexdigest())
