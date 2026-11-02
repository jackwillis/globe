#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Deterministic south-first GELV packs; h5py/numpy are build-time dependencies only."""
import argparse
import bisect
import math
import hashlib
from pathlib import Path
import struct
import zlib
import zipfile
SOURCE_SHA256='74bbc5c23e85188d93f0beba8116babf69e1a8f160e8dc9f053bf8df01b197b6'
ANCHORS=((0,-11000),(64,-2000),(112,-200),(127,-1),(128,0),(192,1000),(240,5000),(255,9000))
def lookup():
    values=[]
    for (i,a),(j,b) in zip(ANCHORS,ANCHORS[1:]):
        for k in range(i,j): values.append(a+((b-a)*(k-i)+(j-i)//2)//(j-i))
    return tuple(values+[ANCHORS[-1][1]])
def quantize(meters, table=None):
    table=lookup() if table is None else table
    if not math.isfinite(meters): raise ValueError('nonfinite height')
    hi=min(255,max(1,bisect.bisect_left(table,meters)))
    return hi-1 if meters-table[hi-1]<=table[hi]-meters else hi

def encode(width,height,cells,extent=(-180000000,-90000000,180000000,90000000)):
    west,south,east,north=extent
    if not (0<width<=32768 and 0<height<=32768 and width*height<=16777216 and len(cells)==width*height):
        raise ValueError('invalid grid dimensions/payload')
    if not (-180000000<=west<east<=180000000 and -90000000<=south<north<=90000000):
        raise ValueError('invalid extent')
    header=struct.pack('<4sHHIIiiiiHHI',b'GELV',1,0,width,height,*extent,256,0,0)
    body=struct.pack('<256h',*lookup())+bytes(cells)
    crc=zlib.crc32(body,zlib.crc32(header[:36]))
    return header[:36]+struct.pack('<I',crc)+body

LAKES_SHA256='f28d42c286d96b57a17aac2cbeb432f8c65532c20063495711fbc64e24666df3'
def read_lakes(path):
    raw=Path(path).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=LAKES_SHA256: raise ValueError('lakes source SHA-256 mismatch')
    with zipfile.ZipFile(path) as archive:
        base='ne_50m_lakes'
        if archive.read(base+'.VERSION.txt').decode('ascii').strip()!='5.0.0': raise ValueError('lakes version')
        shp=archive.read(base+'.shp');dbf=archive.read(base+'.dbf')
    if len(shp)<100 or struct.unpack_from('>I',shp)[0]!=9994 or struct.unpack_from('<II',shp,28)!=(1000,5):
        raise ValueError('expected Polygon shapefile')
    count=struct.unpack_from('<I',dbf,4)[0];header,row_size=struct.unpack_from('<HH',dbf,8)
    name_at=None;field_at=1
    for at in range(32,header-1,32):
        name=dbf[at:at+11].rstrip(b'\0').decode('ascii');length=dbf[at+16]
        if name=='name': name_at=(field_at,length)
        field_at+=length
    if name_at is None or field_at!=row_size or header+count*row_size>len(dbf): raise ValueError('DBF schema')
    records=[];offset=100
    while offset<len(shp):
        if offset+8>len(shp): raise ValueError('record header')
        words=struct.unpack_from('>I',shp,offset+4)[0];offset+=8
        record=shp[offset:offset+2*words];offset+=2*words
        if len(record)!=2*words or len(record)<44 or struct.unpack_from('<I',record)[0]!=5: raise ValueError('record')
        parts,points=struct.unpack_from('<II',record,36)
        if not parts or len(record)!=44+4*parts+16*points: raise ValueError('parts')
        starts=list(struct.unpack_from('<'+'I'*parts,record,44))+[points]
        coords=[struct.unpack_from('<dd',record,44+4*parts+16*i) for i in range(points)]
        if starts[0]!=0 or any(not 0<=a<b<=points for a,b in zip(starts,starts[1:])): raise ValueError('part bounds')
        if any(not math.isfinite(x+y) or not -180<=x<=180 or not -90<=y<=90 for x,y in coords): raise ValueError('coordinate')
        if len(records)>=count: raise ValueError('record count')
        row=dbf[header+len(records)*row_size:header+(len(records)+1)*row_size]
        if row[0]!=32: raise ValueError('deleted DBF row')
        at,length=name_at;name=row[at:at+length].decode('utf-8').strip(' \0')
        records.append((name,[coords[a:b] for a,b in zip(starts,starts[1:])]))
    if len(records)!=count: raise ValueError('record count')
    return records

def lake_cells(width,height,polygons):
    """South-first cell centers, even-odd rings; unwrap each polygon across the date line."""
    selected=set();dx=360/width;dy=180/height
    for parts in polygons:
        rings=[];anchor=None
        for part in parts:
            if len(part)<3: raise ValueError('short lake ring')
            ring=[];previous=part[0][0]
            for lon,lat in part:
                lon=previous+(lon-previous+180)%360-180
                ring.append((lon,lat));previous=lon
            middle=sum(x for x,_ in ring)/len(ring)
            if anchor is None: anchor=middle
            shift=360*round((anchor-middle)/360)
            rings.append([(x+shift,y) for x,y in ring])
        if not rings: continue
        low=min(y for ring in rings for _,y in ring);high=max(y for ring in rings for _,y in ring)
        for row in range(max(0,math.ceil((low+90)/dy-0.5)),min(height,math.ceil((high+90)/dy-0.5))):
            latitude=-90+(row+0.5)*dy;crossings=[]
            for ring in rings:
                for a,b in zip(ring,ring[1:]+ring[:1]):
                    if (a[1]<=latitude<b[1]) or (b[1]<=latitude<a[1]):
                        crossings.append(a[0]+(latitude-a[1])*(b[0]-a[0])/(b[1]-a[1]))
            crossings.sort()
            if len(crossings)%2: raise ValueError('odd lake intersections')
            for left,right in zip(crossings[::2],crossings[1::2]):
                for col in range(math.ceil((left+180)/dx-0.5),math.ceil((right+180)/dx-0.5)):
                    selected.add(row*width+col%width)
    return selected

def mark_water(cells,selected):
    table=lookup()
    for index in selected:
        if table[cells[index]]>=0: cells[index]=127 # -1 m: v1 shallow-water color proxy

def build(source,output,cell_degrees=1.0,expected_sha256=SOURCE_SHA256,lakes=None):
    # Import only for actual source conversion: encode/fixtures use the standard library alone.
    import h5py
    import numpy as np
    digest=hashlib.sha256()
    with open(source,'rb') as f:
        while chunk:=f.read(1024*1024): digest.update(chunk)
    if digest.hexdigest()!=expected_sha256: raise ValueError('source SHA-256 mismatch')
    if not 0<cell_degrees<=180: raise ValueError('invalid cell size')
    width,height=round(360/cell_degrees),round(180/cell_degrees)
    if abs(width*cell_degrees-360)>1e-8 or abs(height*cell_degrees-180)>1e-8 or width*height>16777216:
        raise ValueError('resolution must divide global extent and fit decoder budget')
    records=read_lakes(lakes) if lakes else []
    cells=bytearray()
    table=lookup()
    with h5py.File(source,'r',rdcc_nbytes=64*1024*1024) as f:
        z=f['z']; lat=f['lat'][:]; lon=f['lon'][:]
        if z.shape!=(10800,21600) or z.shape!=(len(lat),len(lon)):
            raise ValueError('unexpected source grid')
        if not (np.allclose(np.diff(lat),1/60,atol=1e-10,rtol=0) and np.allclose(np.diff(lon),1/60,atol=1e-10,rtol=0)
                and abs(lat[0]+90-1/120)<1e-10 and abs(lon[0]+180-1/120)<1e-10):
            raise ValueError('unexpected source coordinates or ordering')
        if len(lat)%height or len(lon)%width: raise ValueError('resolution must align to source cells')
        fy,fx=len(lat)//height,len(lon)//width
        for y in range(height):
            block=z[y*fy:(y+1)*fy,:]
            if not np.all(np.isfinite(block)) or np.any(block==-99999): raise ValueError('missing source heights')
            means=block.reshape(fy,width,fx).mean(axis=(0,2),dtype=np.float64)
            cells.extend(quantize(float(value),table) for value in means)
    if records:
        selected=lake_cells(width,height,[parts for _,parts in records])
        changed=sum(table[cells[i]]>=0 for i in selected)
        mark_water(cells,selected)
        great=[parts for name,parts in records if name in ('Lake Superior','Lake Michigan','Lake Huron','Lake Erie','Lake Ontario')]
        print('Lakes: %d records; %d center cells; %d changed; Great Lakes %d center cells' % (len(records),len(selected),changed,len(lake_cells(width,height,great))))
    data=encode(width,height,cells)
    Path(output).write_bytes(data)
    print(f'{width}x{height}; {len(data)} bytes; SHA-256 {hashlib.sha256(data).hexdigest()}')
    return data

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source');p.add_argument('output');p.add_argument('--cell-degrees',type=float,default=1.0)
    p.add_argument('--source-sha256',default=SOURCE_SHA256)
    p.add_argument('--lakes',help='pinned Natural Earth 50m lakes ZIP; cell-center water mask')
    args=p.parse_args()
    build(args.source,args.output,args.cell_degrees,args.source_sha256,args.lakes)
if __name__=='__main__': main()
