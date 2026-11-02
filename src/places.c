#include "places.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { const unsigned char *bytes;size_t length,position; } Reader;
static int read_bytes(Reader *r,void *out,size_t count) {
    if(count>r->length-r->position)return 0;
    memcpy(out,r->bytes+r->position,count);r->position+=count;return 1;
}
static int u32(Reader *r,uint32_t *v) {
    unsigned char b[4];if(!read_bytes(r,b,4))return 0;
    *v=((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];return 1;
}
static int text(Reader *r,char *dst,size_t capacity) {
    unsigned char b[2];size_t n,i;
    if(!read_bytes(r,b,2))return 0;
    n=(size_t)b[0]*256+b[1];if(n>=capacity||!read_bytes(r,dst,n))return 0;
    for(i=0;i<n;i++)if((unsigned char)dst[i]<32||(unsigned char)dst[i]>126)return 0;
    dst[n]=0;return 1;
}
static int date_valid(const char *date) {
    static const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    int year,month,day,limit;size_t i;
    for(i=0;i<10;i++) {
        if(i==4||i==7){if(date[i]!='-')return 0;}
        else if(date[i]<'0'||date[i]>'9')return 0;
    }
    year=(date[0]-'0')*1000+(date[1]-'0')*100+(date[2]-'0')*10+date[3]-'0';
    month=(date[5]-'0')*10+date[6]-'0';day=(date[8]-'0')*10+date[9]-'0';
    if(year<1970||month<1||month>12)return 0;
    limit=days[month-1];if(month==2&&year%4==0&&(year%100!=0||year%400==0))limit++;
    return day>=1&&day<=limit;
}
void places_free(Places *places) {free(places->items);memset(places,0,sizeof(*places));}
int places_decode(Places *places,const unsigned char *bytes,size_t length) {
    Reader reader;unsigned char magic[8];uint32_t version,count,lon,lat;size_t i,j;
    Places result={0};
    if(!bytes||length>PLACES_MAX_BYTES)return 0;
    reader.bytes=bytes;reader.length=length;reader.position=0;
    if(!read_bytes(&reader,magic,8)||memcmp(magic,"PLNTPOI\0",8)||!u32(&reader,&version)||version!=1
       ||!u32(&reader,&count)||count<1||count>1024||!read_bytes(&reader,result.date,10)||!date_valid(result.date))goto fail;
    result.items=calloc(count,sizeof(Place));if(!result.items)goto fail;
    for(i=0;i<count;i++) {
        Place *p=result.items+i;
        if(!u32(&reader,&p->qid)||!p->qid||p->qid>999999999||!u32(&reader,&p->revision)||!p->revision
           ||!u32(&reader,&lon)||!u32(&reader,&lat)||lon>360000000||lat>180000000
           ||!text(&reader,p->label,sizeof(p->label))||!p->label[0]
           ||!text(&reader,p->description,sizeof(p->description))||!text(&reader,p->population,sizeof(p->population))
           ||!text(&reader,p->wikipedia,sizeof(p->wikipedia)))goto fail;
        if(p->wikipedia[0]&&strncmp(p->wikipedia,"https://en.wikipedia.org/wiki/",30))goto fail;
        for(j=0;j<i;j++)if(result.items[j].qid==p->qid)goto fail;
        p->longitude=lon/1000000.0-180;p->latitude=lat/1000000.0-90;
        p->sphere=geographic(p->longitude,p->latitude);
    }
    if(reader.position!=reader.length)goto fail;
    result.count=count;*places=result;return 1;
fail:
    places_free(&result);return 0;
}
int places_load(Places *places,const char *path) {
    FILE *file=fopen(path,"rb");unsigned char *bytes;long length;int ok;
    if(!file)return 0;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<0||(unsigned long)length>PLACES_MAX_BYTES||fseek(file,0,SEEK_SET)) {fclose(file);return 0;}
    bytes=malloc(length?(size_t)length:1);if(!bytes){fclose(file);return 0;}
    ok=fread(bytes,1,(size_t)length,file)==(size_t)length&&!ferror(file);
    if(fclose(file))ok=0;
    if(ok)ok=places_decode(places,bytes,(size_t)length);
    free(bytes);return ok;
}
int place_project(const Place *p,double yaw,double pitch,double zoom,size_t w,size_t h,int *x,int *y) {
    Vec v;double scale,px,py;
    if(!w||!h||w>4096||h>4096||!isfinite(yaw)||!isfinite(pitch)||!isfinite(zoom)||zoom<=0)return 0;
    v=orient(p->sphere,yaw,pitch);if(v.z<0)return 0;
    scale=(w<h?w:h)*0.45*zoom;px=(w-1)*0.5+v.x*scale;py=(h-1)*0.5-v.y*scale;
    if(!isfinite(px)||!isfinite(py)||px<0||py<0||px>w-1||py>h-1)return 0;
    *x=(int)lround(px);*y=(int)lround(py);return 1;
}
int places_pick(const Places *places,double yaw,double pitch,double zoom,size_t w,size_t h,int x,int y) {
    size_t i;int chosen=-1;double nearest=81;
    for(i=0;i<places->count;i++) {
        int px,py;
        if(place_project(places->items+i,yaw,pitch,zoom,w,h,&px,&py)) {
            double dx=(double)px-x,dy=(double)py-y,distance=dx*dx+dy*dy;
            if(distance<nearest){nearest=distance;chosen=(int)i;}
        }
    }
    return chosen;
}
void places_render(Frame *f,const Places *places,double yaw,double pitch,double zoom,int selected) {
    size_t i;unsigned char saved[3];memcpy(saved,f->ink,3);
    for(i=0;i<places->count;i++) {
        int x,y;
        if(!place_project(places->items+i,yaw,pitch,zoom,f->width,f->height,&x,&y))continue;
        f->ink[0]=255;f->ink[1]=(int)i==selected?180:245;f->ink[2]=(int)i==selected?90:220;
        frame_line(f,x-3,y,x,y-3);frame_line(f,x,y-3,x+3,y);
        frame_line(f,x+3,y,x,y+3);frame_line(f,x,y+3,x-3,y);
    }
    memcpy(f->ink,saved,3);
}
