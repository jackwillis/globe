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

def build(source,output,cell_degrees=1.0,expected_sha256=SOURCE_SHA256):
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
    data=encode(width,height,cells)
    Path(output).write_bytes(data)
    print(f'{width}x{height}; {len(data)} bytes; SHA-256 {hashlib.sha256(data).hexdigest()}')
    return data

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source');p.add_argument('output');p.add_argument('--cell-degrees',type=float,default=1.0)
    p.add_argument('--source-sha256',default=SOURCE_SHA256)
    args=p.parse_args()
    build(args.source,args.output,args.cell_degrees,args.source_sha256)
if __name__=='__main__': main()
