#include "pixels.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    Frame f;PixelPacker p;unsigned char bytes[32];
    const unsigned char rgb[]={255,0,0, 0,255,0, 0,0,255};
    assert(frame_init(&f,3,1));memcpy(f.rgb,rgb,9);
    assert(pixel_packer_init(&p,16,0xf800,0x7e0,0x1f,0));
    memset(bytes,0xaa,sizeof(bytes));assert(pixel_pack(&p,&f,bytes,8,sizeof(bytes)));
    assert(!memcmp(bytes,"\x00\xf8\xe0\x07\x1f\x00\xaa\xaa",8));
    assert(pixel_packer_init(&p,16,0xf800,0x7e0,0x1f,1));
    assert(pixel_pack(&p,&f,bytes,8,sizeof(bytes)));
    assert(!memcmp(bytes,"\xf8\x00\x07\xe0\x00\x1f",6));
    assert(!pixel_pack(&p,&f,bytes,5,sizeof(bytes)));
    assert(!pixel_pack(&p,&f,bytes,8,7));
    assert(!pixel_pack(&p,&f,bytes,SIZE_MAX,sizeof(bytes)));
    assert(pixel_packer_init(&p,24,0xff0000,0xff00,0xff,1));
    assert(pixel_pack(&p,&f,bytes,12,sizeof(bytes)));assert(!memcmp(bytes,rgb,9));
    assert(pixel_packer_init(&p,32,0xff0000,0xff00,0xff,0));
    assert(pixel_pack(&p,&f,bytes,12,sizeof(bytes)));
    assert(!memcmp(bytes,"\x00\x00\xff\x00\x00\xff\x00\x00\xff\x00\x00\x00",12));
    assert(!pixel_packer_init(&p,16,0xf800,0xf800,0x1f,0));
    assert(!pixel_packer_init(&p,16,0xf800,0x7a0,0x1f,0));
    assert(!pixel_packer_init(&p,16,0xff0000,0xff00,0xff,0));
    assert(!pixel_packer_init(&p,12,0xf00,0xf0,0xf,0));
    assert(!pixel_packer_init(&p,32,0xff0000,0xff00,0xff,2));
    assert(pixel_packer_init(&p,8,0xe0,0x1c,0x3,0));
    assert(pixel_pack(&p,&f,bytes,3,sizeof(bytes)));
    assert(bytes[0]==0xe0&&bytes[1]==0x1c&&bytes[2]==3);
    frame_free(&f);puts("pixel layout and bounds tests passed");return 0;
}
