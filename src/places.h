#ifndef PLANET_PLACES_H
#define PLANET_PLACES_H
#include "planet.h"
typedef struct {
    uint32_t qid,revision;
    double longitude,latitude;Vec sphere;
    char label[64],description[192],population[96],wikipedia[256];
} Place;
typedef struct { Place *items;size_t count;char date[11]; } Places;
#define PLACES_MAX_BYTES 1048576u
int places_decode(Places *,const unsigned char *,size_t);
int places_load(Places *,const char *);
void places_free(Places *);
int place_project(const Place *,double,double,double,size_t,size_t,int *,int *);
int places_pick(const Places *,double,double,double,size_t,size_t,int,int);
void places_render(Frame *,const Places *,double,double,double,int);
#endif
