#ifndef PLANET_PIXELS_H
#define PLANET_PIXELS_H
#include "planet.h"
/* Byte order: 0 = least significant byte first, 1 = most significant first. */
typedef struct { uint32_t red[256],green[256],blue[256]; unsigned bytes; int msb; } PixelPacker;
int pixel_packer_init(PixelPacker *,unsigned,uint32_t,uint32_t,uint32_t,int);
int pixel_pack(const PixelPacker *,const Frame *,unsigned char *,size_t,size_t);
#endif
