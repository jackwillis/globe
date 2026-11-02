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

def read_source(path):
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
        name=dbf[len(result)]['ADMIN']
        name=unicodedata.normalize('NFKD',name).encode('ascii','ignore').decode()
        result.append((name,rings))
    if len(result)!=len(dbf): raise ValueError('record count')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('source');parser.add_argument('output');args=parser.parse_args()
    data=encode(read_source(args.source));open(args.output,'wb').write(data)
    print(len(data),hashlib.sha256(data).hexdigest())
