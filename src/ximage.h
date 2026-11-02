#ifndef PLANET_XIMAGE_H
#define PLANET_XIMAGE_H
#include <X11/Xlib.h>
#include "pixels.h"
typedef struct { XImage *image; PixelPacker packer; int direct; } ImageBuffer;
int image_buffer_resize(ImageBuffer *,Display *,unsigned,unsigned);
void image_buffer_free(ImageBuffer *);
void image_convert_reference(XImage *,const Frame *);
int image_convert(ImageBuffer *,const Frame *);
#endif
