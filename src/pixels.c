#include "pixels.h"
static int table(uint32_t *out,uint32_t mask) {
    unsigned shift=0,i;uint32_t maximum;
    if(!mask)return 0;
    while(!(mask&1)){mask>>=1;shift++;}
    maximum=mask;
    if(maximum&(maximum+1))return 0; /* Require contiguous channel bits. */
    for(i=0;i<256;i++)out[i]=(uint32_t)(((uint64_t)i*maximum+127)/255)<<shift;
    return 1;
}
int pixel_packer_init(PixelPacker *p,unsigned bits,uint32_t r,uint32_t g,uint32_t b,int msb) {
    uint32_t all=r|g|b;
    if(!p || (bits!=8 && bits!=16 && bits!=24 && bits!=32) || (msb!=0 && msb!=1)
       ||(r&g)||(r&b)||(g&b)||(bits<32 && (all>>bits)))return 0;
    if(!table(p->red,r)||!table(p->green,g)||!table(p->blue,b))return 0;
    p->bytes=bits/8;p->msb=msb;return 1;
}
int pixel_pack(const PixelPacker *p,const Frame *f,unsigned char *dst,size_t stride,size_t capacity) {
    size_t x,y;
    if(!p||!f||!dst||!f->rgb||!f->width||!f->height||p->bytes<1||p->bytes>4
       ||f->width>SIZE_MAX/p->bytes||stride<f->width*p->bytes
       ||f->height>SIZE_MAX/stride||capacity<stride*f->height)return 0;
    for(y=0;y<f->height;y++) {
        const unsigned char *src=f->rgb+y*f->width*3;
        unsigned char *row=dst+y*stride;
        /* Choose the layout once per row instead of branching for every byte. */
        if(p->bytes==2) {
            unsigned a=p->msb?8:0,b=p->msb?0:8;
            for(x=0;x<f->width;x++,src+=3,row+=2) {
                uint32_t v=p->red[src[0]]|p->green[src[1]]|p->blue[src[2]];
                row[0]=(unsigned char)(v>>a);row[1]=(unsigned char)(v>>b);
            }
        } else if(p->bytes==4) {
            unsigned a=p->msb?24:0,b=p->msb?16:8,c=p->msb?8:16,d=p->msb?0:24;
            for(x=0;x<f->width;x++,src+=3,row+=4) {
                uint32_t v=p->red[src[0]]|p->green[src[1]]|p->blue[src[2]];
                row[0]=(unsigned char)(v>>a);row[1]=(unsigned char)(v>>b);
                row[2]=(unsigned char)(v>>c);row[3]=(unsigned char)(v>>d);
            }
        } else if(p->bytes==3) {
            unsigned a=p->msb?16:0,c=p->msb?0:16;
            for(x=0;x<f->width;x++,src+=3,row+=3) {
                uint32_t v=p->red[src[0]]|p->green[src[1]]|p->blue[src[2]];
                row[0]=(unsigned char)(v>>a);row[1]=(unsigned char)(v>>8);row[2]=(unsigned char)(v>>c);
            }
        } else {
            for(x=0;x<f->width;x++,src+=3,row++)
                row[0]=(unsigned char)(p->red[src[0]]|p->green[src[1]]|p->blue[src[2]]);
        }
    }
    return 1;
}
