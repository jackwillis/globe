#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Retained-index adapter; numerical implementation is immutable scale-v1."""
import sys,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-enclosure-scale-v1'))
from enclose import Geometry,rgb565
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-experiment'))
from reference import read_geometry

class Retained:
    def __init__(self,features,data):
        if len(data)!=640*480*2:raise ValueError('background size')
        self.geometry=Geometry(features);self.data=data
    def tile(self,tx,ty,checkpoint=None):
        if not(0<=tx<80 and 0<=ty<60):raise ValueError('tile domain')
        g=self.geometry
        if checkpoint is None:
            if hasattr(g,'checkpoint'):del g.checkpoint
        else:g.checkpoint=checkpoint
        def bg(x,y):return rgb565(struct.unpack_from('<H',self.data,2*(y*640+x))[0])
        return g.tile(tx,ty,bg,max_visits=6400000,seconds=75,target_den=2560)
