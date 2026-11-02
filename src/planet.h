#ifndef PLANET_H
#define PLANET_H
#include <stddef.h>
#include <stdint.h>
typedef struct { double x,y,z; } Vec;
typedef struct { size_t width,height; unsigned char *rgb; unsigned char ink[3]; } Frame;
int frame_init(Frame *, size_t, size_t);
void frame_free(Frame *);
void frame_clear(Frame *);
void frame_line(Frame *, double,double,double,double);
int frame_ppm(const Frame *, const char *);
Vec geographic(double longitude, double latitude);
Vec orient(Vec, double yaw, double pitch);
int horizon(Vec *, Vec *);
typedef struct { Vec *points; uint32_t *ends; uint32_t count,lines; } Map;
int map_load(Map *, const char *);
void map_free(Map *);
void map_raster(Frame *, const Map *, const Vec *, double);
void map_render(Frame *, const Map *, double,double,double);
typedef struct { unsigned theme; int grid,borders,coasts; } View;
typedef struct { Frame background; double zoom; unsigned theme; int valid; } ViewCache;
void view_cache_free(ViewCache *);
int map_view_fast(Frame *, const Map *, const Map *, double,double,double,const View *,ViewCache *);
void map_view(Frame *, const Map *, const Map *, double,double,double,const View *);
void globe(Frame *, double yaw, double pitch, double zoom);
#endif
