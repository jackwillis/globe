#define _POSIX_C_SOURCE 200809L
#include "places.h"
#include "bench_meta.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
static double now(void) {
    struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)){perror("clock_gettime");exit(1);}
    return t.tv_sec+t.tv_nsec/1e9;
}
static int compare(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);
}
static void stats(const char *name,double *a,size_t n,int comma) {
    size_t i;double sum=0;for(i=0;i<n;i++)sum+=a[i];qsort(a,n,sizeof(*a),compare);
    printf("\"%s\":{\"average_ms\":%.6f,\"median_ms\":%.6f,\"p95_ms\":%.6f,\"p99_ms\":%.6f}%s\n",
      name,sum/n*1000,a[(n-1)/2]*1000,a[(95*n+99)/100-1]*1000,a[(99*n+99)/100-1]*1000,comma?",":"");
}
int main(void) {
    Map coasts={0},borders={0};Places places={0};Frame frame={0};View view={0,0,1,1};ViewCache cache={0};
    struct utsname machine;double marker[120],picking[120],begin,t;size_t n=0;int ok=0;volatile int hits=0;
    alarm(55);
    if(!map_load(&coasts,"data/coastline.plnt")||!map_load(&borders,"data/borders.plnt")
       ||!places_load(&places,"data/places.plnt")||!frame_init(&frame,640,480))goto cleanup;
    begin=now();
    while(n<120&&now()-begin<45) {
        double yaw=n*3.0;
        if(!map_view_fast(&frame,&coasts,&borders,yaw,-15,1,&view,&cache))goto cleanup;
        t=now();places_render(&frame,&places,yaw,-15,1,-1);marker[n]=now()-t;
        t=now();hits+=places_pick(&places,yaw,-15,1,640,480,320,240)+1;picking[n]=now()-t;n++;
    }
    if(!n||uname(&machine))goto cleanup;
    printf("{\"schema\":1,\"backend\":\"headless\",\"workload\":\"place_markers_and_picking_only\",\"os\":\"%s\",\"release\":\"%s\",\"architecture\":\"%s\",\"compiler\":\"%s\",\"width\":640,\"height\":480,\"places\":%zu,\"frames\":%zu,\"elapsed_seconds\":%.6f,",machine.sysname,machine.release,machine.machine,__VERSION__,places.count,n,now()-begin);
    bench_build_flags();puts("\"measurements\":{");stats("marker_overlay",marker,n,1);stats("point_picking",picking,n,0);puts("}}");ok=1;
cleanup:
    (void)hits;frame_free(&frame);view_cache_free(&cache);places_free(&places);map_free(&coasts);map_free(&borders);return ok?0:1;
}
