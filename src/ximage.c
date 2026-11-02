#include "ximage.h"
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
static unsigned long channel(unsigned char value,unsigned long mask) {
    unsigned shift=0;unsigned long maximum;
    if(!mask)return 0;
    while(!(mask&1)){mask>>=1;shift++;}
    maximum=mask;
    return (((unsigned long)value*maximum+127)/255)<<shift;
}
void image_buffer_free(ImageBuffer *buffer) {
    if(buffer->image)XDestroyImage(buffer->image);
    memset(buffer,0,sizeof(*buffer));
}
int image_buffer_resize(ImageBuffer *buffer,Display *d,unsigned width,unsigned height) {
    XImage *image;
    if(buffer->image && buffer->image->width==(int)width && buffer->image->height==(int)height)return 1;
    image_buffer_free(buffer);
    image=XCreateImage(d,DefaultVisual(d,DefaultScreen(d)),(unsigned)DefaultDepth(d,DefaultScreen(d)),ZPixmap,0,NULL,width,height,32,0);
    if(!image)return 0;
    if(image->bytes_per_line<=0 || !height || (size_t)height>SIZE_MAX/(size_t)image->bytes_per_line) {
        XDestroyImage(image);return 0;
    }
    image->data=calloc((size_t)image->bytes_per_line,height);
    if(!image->data){XDestroyImage(image);return 0;}
    buffer->image=image;
    if(image->red_mask<=UINT32_MAX && image->green_mask<=UINT32_MAX && image->blue_mask<=UINT32_MAX
       && (image->byte_order==LSBFirst ||image->byte_order==MSBFirst))
        buffer->direct=pixel_packer_init(&buffer->packer,(unsigned)image->bits_per_pixel,
          (uint32_t)image->red_mask,(uint32_t)image->green_mask,(uint32_t)image->blue_mask,image->byte_order==MSBFirst);
    return 1;
}
void image_convert_reference(XImage *image,const Frame *frame) {
    size_t x,y;
    for(y=0;y<frame->height;y++)for(x=0;x<frame->width;x++) {
        size_t p=(y*frame->width+x)*3;
        unsigned long value=channel(frame->rgb[p],image->red_mask)|channel(frame->rgb[p+1],image->green_mask)|channel(frame->rgb[p+2],image->blue_mask);
        XPutPixel(image,(int)x,(int)y,value);
    }
}
int image_convert(ImageBuffer *buffer,const Frame *frame) {
    if(!buffer->image || buffer->image->width!=(int)frame->width || buffer->image->height!=(int)frame->height)return 0;
    if(buffer->direct)return pixel_pack(&buffer->packer,frame,(unsigned char *)buffer->image->data,
            (size_t)buffer->image->bytes_per_line,(size_t)buffer->image->bytes_per_line*frame->height);
    image_convert_reference(buffer->image,frame);return 1;
}
