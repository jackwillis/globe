#include "planet.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static int number(const char *s,double *v) {
    char *end;errno=0;*v=strtod(s,&end);
    return !errno && end!=s && !*end && isfinite(*v) && fabs(*v)<=3600;
}
int main(int argc,char **argv) {
    Frame f; Map map; double yaw=0,pitch=0;
    if(argc<2 || argc>5 || (argc>2&&!number(argv[2],&yaw)) || (argc>3&&!number(argv[3],&pitch))) {
        fprintf(stderr,"usage: planet OUTPUT.ppm [yaw-degrees [pitch-degrees [map.plnt]]]\n");return 2;
    }
    if(!frame_init(&f,320,240)){fprintf(stderr,"frame allocation failed\n");return 1;}
    if(argc==5) {
        if(!map_load(&map,argv[4])){fprintf(stderr,"invalid map: %s\n",argv[4]);frame_free(&f);return 1;}
        map_render(&f,&map,yaw,pitch,1);map_free(&map);
    } else globe(&f,yaw,pitch,1);
    if(!frame_ppm(&f,argv[1])){perror(argv[1]);frame_free(&f);return 1;}
    frame_free(&f);return 0;
}
