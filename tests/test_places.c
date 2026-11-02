#include "places.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv) {
    Places places={0};Place origin;Frame f,g;int x,y;size_t i;
    if(argc==2){int ok=places_load(&places,argv[1]);places_free(&places);return ok?0:1;}
    assert(places_load(&places,"data/places.plnt"));assert(places.count==13);
    assert(!strcmp(places.date,"2026-11-01"));
    for(i=0;i<places.count;i++) {
        Place *p=places.items+i;
        assert(p->qid&&p->revision&&p->label[0]);
        assert(fabs(p->sphere.x*p->sphere.x+p->sphere.y*p->sphere.y+p->sphere.z*p->sphere.z-1)<1e-12);
    }
    memset(&origin,0,sizeof(origin));origin.sphere=geographic(0,0);
    {
        Places single={&origin,1,"2026-11-01"};
        assert(place_project(&origin,0,0,1,320,240,&x,&y));assert(x==160&&y==120);
        assert(places_pick(&single,0,0,1,320,240,x,y)==0);
        assert(places_pick(&single,0,0,1,320,240,x+12,y)==-1);
        assert(places_pick(&single,180,0,1,320,240,x,y)==-1);
        assert(!place_project(&origin,NAN,0,1,320,240,&x,&y));
        assert(!place_project(&origin,0,0,0,320,240,&x,&y));
        assert(!place_project(&origin,0,0,1,0,240,&x,&y));
        origin.sphere=geographic(90,0);assert(!place_project(&origin,0,0,8,320,240,&x,&y));
        origin.sphere=geographic(0,90);assert(place_project(&origin,0,0,1,320,240,&x,&y));assert(y==12);
    }
    assert(frame_init(&f,320,240));assert(frame_init(&g,320,240));
    places_render(&f,&places,0,-15,1,-1);places_render(&g,&places,0,-15,1,-1);
    assert(!memcmp(f.rgb,g.rgb,320*240*3));assert(f.ink[0]==100&&f.ink[1]==220&&f.ink[2]==255);
    frame_clear(&g);places_render(&g,&places,0,-15,1,0);assert(memcmp(f.rgb,g.rgb,320*240*3));
    frame_free(&f);frame_free(&g);places_free(&places);puts("place loading, projection, picking and markers passed");return 0;
}
