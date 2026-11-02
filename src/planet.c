#include "planet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PI 3.14159265358979323846
int frame_init(Frame *f,size_t w,size_t h) {
    if (!f) return 0;
    f->rgb=NULL; f->width=f->height=0;
    f->ink[0]=100;f->ink[1]=220;f->ink[2]=255;
    if (!w || !h || w>4096 || h>4096 || w>SIZE_MAX/h/3) return 0;
    f->rgb=calloc(w*h,3);
    if (!f->rgb) return 0;
    f->width=w; f->height=h; return 1;
}
void frame_free(Frame *f) { free(f->rgb); f->rgb=NULL; f->width=f->height=0; }
void frame_clear(Frame *f) { memset(f->rgb,0,f->width*f->height*3); }
/* Liang-Barsky clipping: bound floating coordinates before integer conversion. */
static int bound(double p,double q,double *lo,double *hi) {
    double t;
    if (p==0) return q>=0;
    t=q/p;
    if (p<0) { if(t>*hi)return 0; if(t>*lo)*lo=t; }
    else { if(t<*lo)return 0; if(t<*hi)*hi=t; }
    return 1;
}
void frame_line(Frame *f,double ax,double ay,double bx,double by) {
    double dx=bx-ax,dy=by-ay,lo=0,hi=1;
    int x,y,x1,y1,ix,iy,sx,sy,err;
    if (!f->rgb || !isfinite(ax)||!isfinite(ay)||!isfinite(bx)||!isfinite(by)
        || !isfinite(dx)||!isfinite(dy)) return;
    if(!bound(-dx,ax,&lo,&hi)||!bound(dx,(double)f->width-1-ax,&lo,&hi)
       ||!bound(-dy,ay,&lo,&hi)||!bound(dy,(double)f->height-1-ay,&lo,&hi))return;
    x=(int)lround(ax+lo*dx); y=(int)lround(ay+lo*dy);
    x1=(int)lround(ax+hi*dx); y1=(int)lround(ay+hi*dy);
    ix=abs(x1-x); iy=-abs(y1-y); sx=x<x1?1:-1; sy=y<y1?1:-1; err=ix+iy;
    for(;;) {
        int twice=2*err;
        if(x>=0 && y>=0 && (size_t)x<f->width && (size_t)y<f->height) {
            size_t p=((size_t)y*f->width+(size_t)x)*3;
            memcpy(f->rgb+p,f->ink,3);
        }
        if(x==x1 && y==y1)break;
        if(twice>=iy){err+=iy;x+=sx;}
        if(twice<=ix){err+=ix;y+=sy;}
    }
}
int frame_ppm(const Frame *f,const char *path) {
    FILE *out=fopen(path,"wb"); int ok;
    if(!out)return 0;
    ok=fprintf(out,"P6\n%zu %zu\n255\n",f->width,f->height)>0;
    if(fwrite(f->rgb,3,f->width*f->height,out)!=f->width*f->height)ok=0;
    if(fclose(out)!=0)ok=0;
    return ok;
}
Vec geographic(double lon,double lat) {
    double a=lon*PI/180,b=lat*PI/180;
    Vec v={cos(b)*sin(a),sin(b),cos(b)*cos(a)}; return v;
}
Vec orient(Vec v,double yaw,double pitch) {
    double a=yaw*PI/180,b=pitch*PI/180;
    Vec t={cos(a)*v.x+sin(a)*v.z,v.y,-sin(a)*v.x+cos(a)*v.z};
    Vec r={t.x,cos(b)*t.y-sin(b)*t.z,sin(b)*t.y+cos(b)*t.z};return r;
}
/* Geometry consists of short straight chords; clip each chord at camera z=0. */
int horizon(Vec *a,Vec *b) {
    Vec *hidden; double t; Vec v;
    if(a->z<0 && b->z<0)return 0;
    if(a->z>=0 && b->z>=0)return 1;
    t=a->z/(a->z-b->z);
    v.x=a->x+t*(b->x-a->x);v.y=a->y+t*(b->y-a->y);v.z=0;
    hidden=a->z<0?a:b;*hidden=v;return 1;
}
static void segment(Frame *f,Vec a,Vec b,double scale) {
    if(horizon(&a,&b))frame_line(f,(f->width-1)*0.5+a.x*scale,
       (f->height-1)*0.5-a.y*scale,(f->width-1)*0.5+b.x*scale,
       (f->height-1)*0.5-b.y*scale);
}
static void grid_overlay(Frame *f,double yaw,double pitch,double zoom) {
    int lat,lon; double scale=(f->width<f->height?f->width:f->height)*0.45*zoom;
    for(lat=-75;lat<=75;lat+=15)for(lon=-180;lon<180;lon+=2)
        segment(f,orient(geographic(lon,lat),yaw,pitch),orient(geographic(lon+2,lat),yaw,pitch),scale);
    for(lon=-180;lon<180;lon+=15)for(lat=-90;lat<90;lat+=2)
        segment(f,orient(geographic(lon,lat),yaw,pitch),orient(geographic(lon,lat+2),yaw,pitch),scale);
}

void globe(Frame *f,double yaw,double pitch,double zoom) {
    frame_clear(f);grid_overlay(f,yaw,pitch,zoom);
}

static void map_overlay(Frame *f,const Map *m,double yaw,double pitch,double zoom) {
    uint32_t line,start=0,i;
    double scale=(f->width<f->height?f->width:f->height)*0.45*zoom;
    for(line=0;line<m->lines;line++) {
        for(i=start+1;i<m->ends[line];i++)
            segment(f,orient(m->points[i-1],yaw,pitch),orient(m->points[i],yaw,pitch),scale);
        start=m->ends[line];
    }
}

void map_render(Frame *f,const Map *m,double yaw,double pitch,double zoom) {
    frame_clear(f);map_overlay(f,m,yaw,pitch,zoom);
}

/* Raster-only entry point for separately measured transformed geometry. */
void map_raster(Frame *f,const Map *m,const Vec *points,double zoom) {
    uint32_t line,start=0,i;
    double scale=(f->width<f->height?f->width:f->height)*0.45*zoom;
    frame_clear(f);
    for(line=0;line<m->lines;line++) {
        for(i=start+1;i<m->ends[line];i++)segment(f,points[i-1],points[i],scale);
        start=m->ends[line];
    }
}

/* Flat ocean disc with optional vector overlays; no elevation is implied. */
void map_view(Frame *f,const Map *coasts,const Map *borders,double yaw,double pitch,
              double zoom,const View *view) {
    static const unsigned char palettes[3][5][3]={
        {{6,12,24},{16,45,67},{113,218,205},{235,183,102},{32,78,99}},
        {{21,18,14},{47,41,30},{235,216,164},{205,138,95},{80,69,47}},
        {{0,0,0},{4,16,10},{109,245,158},{255,211,103},{15,59,34}}
    };
    const unsigned char (*p)[3]=palettes[view->theme%3];
    double radius=(f->width<f->height?f->width:f->height)*0.45*zoom;
    double cx=(f->width-1)*0.5,cy=(f->height-1)*0.5;
    size_t x,y;
    unsigned char saved[3];memcpy(saved,f->ink,3);
    for(y=0;y<f->height;y++)for(x=0;x<f->width;x++) {
        double dx=x-cx,dy=y-cy;
        memcpy(f->rgb+(y*f->width+x)*3,p[dx*dx+dy*dy<=radius*radius?1:0],3);
    }
    if(view->grid){memcpy(f->ink,p[4],3);grid_overlay(f,yaw,pitch,zoom);}
    if(view->borders && borders){memcpy(f->ink,p[3],3);map_overlay(f,borders,yaw,pitch,zoom);}
    if(view->coasts){memcpy(f->ink,p[2],3);map_overlay(f,coasts,yaw,pitch,zoom);}
    memcpy(f->ink,saved,3);
}

/* Same arithmetic ordering as orient(), with trigonometry shared by the frame. */
typedef struct { double cy,sy,cp,sp; } Rotation;
static Vec rotate_cached(Vec v,const Rotation *r) {
    Vec t={r->cy*v.x+r->sy*v.z,v.y,-r->sy*v.x+r->cy*v.z};
    Vec result={t.x,r->cp*t.y-r->sp*t.z,r->sp*t.y+r->cp*t.z};return result;
}
static void overlay_cached(Frame *f,const Map *map,const Rotation *rotation,double scale) {
    uint32_t line,start=0,i;
    for(line=0;line<map->lines;line++) {
        Vec previous=rotate_cached(map->points[start],rotation);
        for(i=start+1;i<map->ends[line];i++) {
            Vec current=rotate_cached(map->points[i],rotation);
            segment(f,previous,current,scale);previous=current;
        }
        start=map->ends[line];
    }
}
void view_cache_free(ViewCache *cache) {
    frame_free(&cache->background);memset(cache,0,sizeof(*cache));
}
int map_view_fast(Frame *f,const Map *coasts,const Map *borders,double yaw,double pitch,
                  double zoom,const View *view,ViewCache *cache) {
    static const unsigned char colors[3][3][3]={
        {{113,218,205},{235,183,102},{32,78,99}},
        {{235,216,164},{205,138,95},{80,69,47}},
        {{109,245,158},{255,211,103},{15,59,34}}
    };
    unsigned theme=view->theme%3;
    double a=yaw*PI/180,b=pitch*PI/180;
    Rotation rotation={cos(a),sin(a),cos(b),sin(b)};
    double scale=(f->width<f->height?f->width:f->height)*0.45*zoom;
    unsigned char saved[3];
    if(!cache->valid || cache->background.width!=f->width || cache->background.height!=f->height
       ||cache->zoom!=zoom ||cache->theme!=theme) {
        View background={theme,0,0,0};
        cache->valid=0;
        if(cache->background.width!=f->width ||cache->background.height!=f->height) {
            frame_free(&cache->background);
            if(!frame_init(&cache->background,f->width,f->height))return 0;
        }
        map_view(&cache->background,coasts,NULL,0,0,zoom,&background);
        cache->zoom=zoom;cache->theme=theme;cache->valid=1;
    }
    memcpy(f->rgb,cache->background.rgb,f->width*f->height*3);memcpy(saved,f->ink,3);
    /* Grid remains on the portable reference path; identical sampling and clipping. */
    if(view->grid){memcpy(f->ink,colors[theme][2],3);grid_overlay(f,yaw,pitch,zoom);}
    if(view->borders && borders){memcpy(f->ink,colors[theme][1],3);overlay_cached(f,borders,&rotation,scale);}
    if(view->coasts){memcpy(f->ink,colors[theme][0],3);overlay_cached(f,coasts,&rotation,scale);}
    memcpy(f->ink,saved,3);return 1;
}
