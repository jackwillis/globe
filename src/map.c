#include "planet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int u32(FILE *f,uint32_t *v) {
    unsigned char b[4];
    if(fread(b,1,4,f)!=4)return 0;
    *v=((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];return 1;
}
void map_free(Map *m) { free(m->points);free(m->ends);memset(m,0,sizeof(*m)); }
int map_load(Map *m,const char *path) {
    FILE *f;unsigned char magic[8];uint32_t version,lines,points,i,j,n,pos=0;
    memset(m,0,sizeof(*m));f=fopen(path,"rb");if(!f)return 0;
    if(fread(magic,1,8,f)!=8 || memcmp(magic,"PLNTMAP\0",8) || !u32(f,&version)
       ||version!=1||!u32(f,&lines)||!u32(f,&points)||!lines||lines>100000
       ||points<2||points>1000000||lines>points/2)goto fail;
    m->points=calloc(points,sizeof(Vec));m->ends=calloc(lines,sizeof(uint32_t));
    if(!m->points||!m->ends)goto fail;
    for(i=0;i<lines;i++) {
        if(!u32(f,&n)||n<2||n>points-pos)goto fail;
        for(j=0;j<n;j++) {
            uint32_t lon,lat;
            if(!u32(f,&lon)||!u32(f,&lat)||lon>360000000||lat>180000000)goto fail;
            m->points[pos++]=geographic(lon/1000000.0-180,lat/1000000.0-90);
        }
        m->ends[i]=pos;
    }
    if(pos!=points||fgetc(f)!=EOF||ferror(f))goto fail;
    if(fclose(f)!=0){map_free(m);return 0;}
    m->count=points;m->lines=lines;return 1;
fail:
    fclose(f);map_free(m);return 0;
}
