// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "aa.hpp"
#include <cstdio>
#include <vector>
int main() {
    unsigned count; int closed;
    if (std::scanf("%u%d", &count, &closed)!=2 || count<2 || count>1024) return 2;
    std::vector<globe::ScreenPoint> points(count);
    for (auto& p: points) if(std::scanf("%lf%lf", &p.x,&p.y)!=2) return 2;
    auto fb=globe::Framebuffer::create(48,48);
    auto ws=aa_experiment::Workspace::create(48,48,48*48);
    if(!fb || !ws) return 3;
    fb->clear(0);
    aa_experiment::Chain chain{points,closed!=0};
    if(aa_experiment::render_feature(*fb,{&chain,1},0xffff,*ws)!=aa_experiment::Result::Ok) return 4;
    for(int y=0;y<48;++y) for(int x=0;x<48;++x) {
        unsigned v=fb->pixel(x,y); std::putchar(v&255);std::putchar(v>>8);
    }
}
