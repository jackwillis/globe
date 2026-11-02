#define _POSIX_C_SOURCE 200809L
#include "planet.h"
#include "bench_meta.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/resource.h>
static double now(void) {
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)){perror("clock_gettime");exit(1);}
    return t.tv_sec+t.tv_nsec/1e9;
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
int main(int argc,char **argv) {
    Map coasts={0},borders={0};Frame ref={0},fast={0};
    View view={0,0,1,1};ViewCache cache={0};
    double reference[120],optimized[120],begin,t;size_t n=0;
    unsigned width=640,height=480;struct utsname machine;struct rusage usage;
    int ok=0;
    alarm(55);
    if(argc>2 || (argc==2 && strcmp(argv[1],"320") && strcmp(argv[1],"640"))) {
        fprintf(stderr,"usage: planet-view-bench [320|640]\n");return 2;
    }
    if(argc==2 && !strcmp(argv[1],"320")){width=320;height=240;}
    if(!map_load(&coasts,"data/coastline.plnt")||!map_load(&borders,"data/borders.plnt")
       ||!frame_init(&ref,width,height)||!frame_init(&fast,width,height))goto cleanup;
    begin=now();
    while(n<120 && now()-begin<45) {
        double yaw=n*3.0;
        /* Alternate order to reduce systematic ordering bias. Cold first frame included. */
        if(n%2==0) {
            t=now();map_view(&ref,&coasts,&borders,yaw,-15,1,&view);reference[n]=now()-t;
        }
        t=now();
        if(!map_view_fast(&fast,&coasts,&borders,yaw,-15,1,&view,&cache))goto cleanup;
        optimized[n]=now()-t;
        if(n%2) {
            t=now();map_view(&ref,&coasts,&borders,yaw,-15,1,&view);reference[n]=now()-t;
        }
        if(memcmp(ref.rgb,fast.rgb,(size_t)width*height*3)) {
            fprintf(stderr,"reference mismatch at frame %zu\n",n);goto cleanup;
        }
        n++;
    }
    if(!n||uname(&machine))goto cleanup;
    printf("{\"schema\":1,\"workload\":\"colored_coasts_borders\",\"backend\":\"headless\",\"os\":\"%s\",\"release\":\"%s\",\"architecture\":\"%s\",\"compiler\":\"%s\",\"width\":%u,\"height\":%u,\"frames\":%zu,\"elapsed_seconds\":%.6f,\"all_frames_identical\":true,",machine.sysname,machine.release,machine.machine,__VERSION__,width,height,n,now()-begin);
    if(!getrusage(RUSAGE_SELF,&usage))printf("\"peak_rss_kib\":%ld,",usage.ru_maxrss);
    bench_build_flags();
    puts("\"measurements\":{");stats("reference",reference,n,1);stats("optimized",optimized,n,0);puts("}}");ok=1;
cleanup:
    view_cache_free(&cache);frame_free(&ref);frame_free(&fast);map_free(&coasts);map_free(&borders);
    return ok?0:1;
}
