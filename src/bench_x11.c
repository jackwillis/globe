#define _POSIX_C_SOURCE 200809L
#include "planet.h"
#include "bench_meta.h"
#include <unistd.h>
#include "ximage.h"
#include <X11/Xutil.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#define SAMPLES 120
static double now(void) {
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)){perror("clock_gettime");exit(1);}
    return t.tv_sec+t.tv_nsec/1e9;
}
static void until(double deadline) {
    double remaining;
    while((remaining=deadline-now())>0) {
        struct timespec delay;
        delay.tv_sec=(time_t)remaining;delay.tv_nsec=(long)((remaining-delay.tv_sec)*1e9);
        if(nanosleep(&delay,NULL) && errno!=EINTR){perror("nanosleep");exit(1);}
    }
}
static int compare(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);
}
static void stats(const char *name,double *a,size_t n,int comma) {
    double sum=0;size_t i;
    for(i=0;i<n;i++)sum+=a[i];
    qsort(a,n,sizeof(*a),compare);
    printf("\"%s\":{\"average_ms\":%.6f,\"median_ms\":%.6f,\"p95_ms\":%.6f,\"p99_ms\":%.6f,\"effective_fps\":%.3f}%s\n",
       name,1000*sum/n,1000*a[(n-1)/2],1000*a[(95*n+99)/100-1],1000*a[(99*n+99)/100-1],n/sum,comma?",":"");
}
static int model(ImageBuffer *buffer,unsigned width,unsigned height) {
    XImage *image=calloc(1,sizeof(*image));
    if(!image)return 0;
    image->width=(int)width;image->height=(int)height;image->format=ZPixmap;
    image->depth=16;image->bits_per_pixel=16;image->byte_order=MSBFirst;
    image->bitmap_unit=32;image->bitmap_pad=32;image->bitmap_bit_order=MSBFirst;
    image->red_mask=0xf800;image->green_mask=0x7e0;image->blue_mask=0x1f;
    image->bytes_per_line=(int)((width*2+3)/4*4);
    if(!XInitImage(image)){free(image);return 0;}
    image->data=calloc((size_t)image->bytes_per_line,height);
    if(!image->data){XDestroyImage(image);return 0;}
    buffer->image=image;buffer->direct=pixel_packer_init(&buffer->packer,16,0xf800,0x7e0,0x1f,1);
    return buffer->direct;
}
int main(int argc,char **argv) {
    Display *d=NULL;Window window=0;GC gc=0;XEvent event;
    Map coasts={0},borders={0};Frame frame={0};View view={0,0,1,1};ViewCache cache={0};
    ImageBuffer image={0},reference={0};
    double old_pack[SAMPLES],new_pack[SAMPLES],render[SAMPLES],submit[SAMPLES],sync_time[SAMPLES],paced[SAMPLES];
    size_t n=0,m=0;unsigned width=640,height=480;int headless=0,ok=0,arg;
    double begin,t,start;struct utsname machine;struct rusage usage;
    alarm(55);
    for(arg=1;arg<argc;arg++) {
        if(!strcmp(argv[arg],"--headless"))headless=1;
        else if(!strcmp(argv[arg],"320")){width=320;height=240;}
        else if(!strcmp(argv[arg],"640")){width=640;height=480;}
        else {fprintf(stderr,"usage: planet-x11-bench [--headless] [320|640]\n");return 2;}
    }
    if(!map_load(&coasts,"data/coastline.plnt")||!map_load(&borders,"data/borders.plnt")||!frame_init(&frame,width,height))goto cleanup;
    if(headless) {
        if(!model(&image,width,height)||!model(&reference,width,height))goto cleanup;
    } else {
        d=XOpenDisplay(NULL);
        if(!d){fprintf(stderr,"cannot open existing authorized X display; use --headless for conversion only\n");goto cleanup;}
        if(DefaultVisual(d,DefaultScreen(d))->class!=TrueColor){fprintf(stderr,"TrueColor required\n");goto cleanup;}
        if(!image_buffer_resize(&image,d,width,height)||!image_buffer_resize(&reference,d,width,height))goto cleanup;
        window=XCreateSimpleWindow(d,DefaultRootWindow(d),0,0,width,height,0,0,0);
        XStoreName(d,window,"Planet bounded presentation benchmark");
        XSelectInput(d,window,StructureNotifyMask);gc=XCreateGC(d,window,0,NULL);XMapWindow(d,window);
        XSync(d,False);
        /* Do not wait indefinitely for a window manager; the mapped window is enough. */
        while(XPending(d))XNextEvent(d,&event);
    }
    begin=now();
    while(n<SAMPLES && now()-begin<45) {
        double yaw=n*3.0;
        /* Compare conversion paths separately from paced frame measurements. */
        if(!map_view_fast(&frame,&coasts,&borders,yaw,-15,1,&view,&cache))goto cleanup;
        if(n%2==0) {
            t=now();image_convert_reference(reference.image,&frame);old_pack[n]=now()-t;
        }
        t=now();if(!image_convert(&image,&frame))goto cleanup;new_pack[n]=now()-t;
        if(n%2) {
            t=now();image_convert_reference(reference.image,&frame);old_pack[n]=now()-t;
        }
        if(memcmp(reference.image->data,image.image->data,(size_t)image.image->bytes_per_line*height)) {
            fprintf(stderr,"pixel packing mismatch at frame %zu\n",n);goto cleanup;
        }
        n++;
    }
    if(!headless) {
        while(m<n && now()-begin<45) {
            double yaw=m*3.0;
            start=now();
            while(XPending(d))XNextEvent(d,&event);
            t=now();if(!map_view_fast(&frame,&coasts,&borders,yaw,-15,1,&view,&cache))goto cleanup;
            render[m]=now()-t;
            if(!image_convert(&image,&frame))goto cleanup;
            t=now();XPutImage(d,window,gc,image.image,0,0,0,0,width,height);XFlush(d);submit[m]=now()-t;
            t=now();XSync(d,False);sync_time[m]=now()-t;
            until(start+1.0/30.0);paced[m]=now()-start;m++;
        }
        if(!m)goto cleanup;
    }
    if(!n||uname(&machine))goto cleanup;
    printf("{\"schema\":1,\"backend\":\"%s\",\"os\":\"%s\",\"release\":\"%s\",\"architecture\":\"%s\",\"compiler\":\"%s\",\"width\":%u,\"height\":%u,\"frames\":%zu,\"elapsed_seconds\":%.6f,\"depth\":%d,\"bits_per_pixel\":%d,\"bytes_per_line\":%d,\"byte_order\":%d,\"red_mask\":%lu,\"green_mask\":%lu,\"blue_mask\":%lu,\"direct_packing\":%s,\"all_frames_identical\":true,",headless?"modeled_rgb565_no_display":"XPutImage_XSync",machine.sysname,machine.release,machine.machine,__VERSION__,width,height,n,now()-begin,image.image->depth,image.image->bits_per_pixel,image.image->bytes_per_line,image.image->byte_order,image.image->red_mask,image.image->green_mask,image.image->blue_mask,image.direct?"true":"false");
    if(!getrusage(RUSAGE_SELF,&usage))printf("\"peak_rss_kib\":%ld,",usage.ru_maxrss);
    printf("\"presentation_frames\":%zu,",m);
    bench_build_flags();
    puts("\"measurements\":{");stats("XPutPixel_conversion",old_pack,n,1);
    stats("lookup_conversion",new_pack,n,!headless);
    if(!headless) {
        stats("colored_render",render,m,1);stats("submission_and_flush",submit,m,1);
        stats("XSync_proxy",sync_time,m,1);stats("paced_frame",paced,m,0);
    }
    puts("}}");ok=1;
cleanup:
    image_buffer_free(&image);image_buffer_free(&reference);view_cache_free(&cache);
    frame_free(&frame);map_free(&coasts);map_free(&borders);
    if(d){if(gc)XFreeGC(d,gc);if(window)XDestroyWindow(d,window);XCloseDisplay(d);}
    return ok?0:1;
}
