#include "planet.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){return fabs(a-b)<1e-10;}
int main(void) {
    Frame f,g;Map map;Vec *transformed;Vec a,b,v;size_t i,count;
    v=geographic(0,0);assert(near(v.x,0)&&near(v.y,0)&&near(v.z,1));
    v=geographic(90,0);assert(near(v.x,1)&&near(v.z,0));
    v=geographic(17,90);assert(near(v.y,1)&&near(v.x,0));
    a=geographic(-180,20);b=geographic(180,20);assert(near(a.x,b.x)&&near(a.z,b.z));
    v=orient(geographic(0,0),90,0);assert(near(v.x,1)&&near(v.z,0));
    a=(Vec){0,0,-1};b=(Vec){1,0,1};assert(horizon(&a,&b));assert(near(a.x,0.5)&&a.z==0);
    a=(Vec){0,0,-1};b=(Vec){1,0,-2};assert(!horizon(&a,&b));
    a=(Vec){0,1,0};b=(Vec){1,0,0};assert(horizon(&a,&b));
    assert(!frame_init(&f,0,20));assert(!frame_init(&f,SIZE_MAX,2));
    assert(frame_init(&f,8,8));frame_line(&f,-100,4,100,4);
    count=0;for(i=0;i<64;i++)if(f.rgb[i*3])count++;
    assert(count==8);
    frame_clear(&f);frame_line(&f,-5,-5,-1,-1);frame_line(&f,NAN,0,1,1);
    for(i=0;i<192;i++)assert(f.rgb[i]==0);
    frame_line(&f,-100,-100,100,100);count=0;
    for(i=0;i<64;i++)if(f.rgb[i*3])count++;
    assert(count==8);
    frame_free(&f);assert(frame_init(&f,320,240));assert(frame_init(&g,320,240));
    globe(&f,31,22,1);globe(&g,31,22,1);assert(!memcmp(f.rgb,g.rgb,320*240*3));
    count=0;for(i=0;i<320*240;i++)if(f.rgb[i*3])count++;assert(count>1000);
    globe(&g,0,0,1);assert(memcmp(f.rgb,g.rgb,320*240*3));
    assert(map_load(&map,"data/coastline.plnt"));
    transformed=calloc(map.count,sizeof(Vec));assert(transformed);
    for(i=0;i<map.count;i++)transformed[i]=orient(map.points[i],31,22);
    map_render(&f,&map,31,22,1);map_raster(&g,&map,transformed,1);
    assert(!memcmp(f.rgb,g.rgb,320*240*3));
    free(transformed);
    {
        Map borders;View view={0,0,0,1};ViewCache cache={0};
        size_t test;
        assert(map_load(&borders,"data/borders.plnt"));
        map_view(&f,&map,&borders,31,22,1,&view);
        map_view(&g,&map,&borders,31,22,1,&view);
        assert(!memcmp(f.rgb,g.rgb,320*240*3));
        assert(memcmp(f.rgb,f.rgb+(120*320+160)*3,3)); /* disc differs from space */
        view.grid=1;map_view(&g,&map,&borders,31,22,1,&view);
        assert(memcmp(f.rgb,g.rgb,320*240*3));
        view.grid=0;view.borders=1;map_view(&g,&map,&borders,31,22,1,&view);
        assert(memcmp(f.rgb,g.rgb,320*240*3));
        view.borders=0;view.coasts=0;map_view(&g,&map,&borders,31,22,1,&view);
        assert(memcmp(f.rgb,g.rgb,320*240*3));
        view.coasts=1;view.theme=1;map_view(&g,&map,&borders,31,22,1,&view);
        assert(memcmp(f.rgb,g.rgb,320*240*3));
        view.theme=2;map_view(&g,&map,&borders,31,22,8,&view);
        for(test=0;test<24;test++) {
            double yaw=(double)test*37-360,pitch=(double)(test%5)*45-90;
            double zoom=test%4==0?0.1:test%4==1?1:test%4==2?3:8;
            view.theme=(unsigned)(test%3);view.grid=(int)(test%2);
            view.borders=(int)((test/2)%2);view.coasts=(int)((test/3)%2);
            map_view(&f,&map,&borders,yaw,pitch,zoom,&view);
            assert(map_view_fast(&g,&map,&borders,yaw,pitch,zoom,&view,&cache));
            assert(!memcmp(f.rgb,g.rgb,320*240*3));
        }
        frame_free(&g);assert(frame_init(&g,57,39));
        assert(map_view_fast(&g,&map,&borders,0,0,1,&view,&cache));
        frame_free(&f);assert(frame_init(&f,57,39));
        map_view(&f,&map,&borders,0,0,1,&view);
        assert(!memcmp(f.rgb,g.rgb,57*39*3));
        view_cache_free(&cache);
        map_free(&borders);
    }
    map_free(&map);
    frame_free(&f);frame_free(&g);puts("all tests passed");return 0;
}
