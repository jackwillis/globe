#include "ximage.h"
#include <X11/Xutil.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    static const struct { int bits,depth; unsigned long r,g,b; } layouts[]={
        {8,8,0xe0,0x1c,3},{16,16,0xf800,0x7e0,0x1f},
        {16,15,0x7c00,0x3e0,0x1f},{24,24,0xff0000,0xff00,0xff},
        {32,24,0xff0000,0xff00,0xff},{32,24,0xff,0xff00,0xff0000},
        {32,30,0x3ff00000,0xffc00,0x3ff}
    };
    Frame frame;size_t j,i;int order;assert(frame_init(&frame,17,5));
    for(i=0;i<frame.width*frame.height*3;i++)frame.rgb[i]=(unsigned char)(i*73+19);
    for(j=0;j<sizeof(layouts)/sizeof(layouts[0]);j++)for(order=0;order<2;order++) {
        XImage image,reference;ImageBuffer buffer;size_t bytes;
        memset(&image,0,sizeof(image));memset(&buffer,0,sizeof(buffer));
        image.width=17;image.height=5;image.format=ZPixmap;image.depth=layouts[j].depth;
        image.bits_per_pixel=layouts[j].bits;image.byte_order=order;
        image.bitmap_unit=32;image.bitmap_pad=32;image.bitmap_bit_order=order;
        image.bytes_per_line=((17*image.bits_per_pixel+31)/32)*4;
        image.red_mask=layouts[j].r;image.green_mask=layouts[j].g;image.blue_mask=layouts[j].b;
        bytes=(size_t)image.bytes_per_line*5;
        image.data=calloc(bytes,1);assert(image.data);assert(XInitImage(&image));
        reference=image;reference.data=calloc(bytes,1);assert(reference.data);assert(XInitImage(&reference));
        buffer.image=&image;
        buffer.direct=pixel_packer_init(&buffer.packer,(unsigned)image.bits_per_pixel,
            (uint32_t)image.red_mask,(uint32_t)image.green_mask,(uint32_t)image.blue_mask,order==MSBFirst);
        assert(buffer.direct);assert(image_convert(&buffer,&frame));
        image_convert_reference(&reference,&frame);assert(!memcmp(image.data,reference.data,bytes));
        free(image.data);free(reference.data);
    }
    frame_free(&frame);puts("XPutPixel equivalence passed: 7 layouts, both byte orders, padded rows");return 0;
}
