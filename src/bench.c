#define _POSIX_C_SOURCE 200809L
#include "planet.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#define LIMIT 1000
static double now(void) {
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)!=0){perror("clock_gettime");exit(1);}
    return t.tv_sec+t.tv_nsec/1e9;
}
static int compare(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);
}
static void report(const char *name,double *samples,size_t n,int comma) {
    size_t i;double sum=0;
    for(i=0;i<n;i++)sum+=samples[i];
    qsort(samples,n,sizeof(double),compare);
    printf("\"%s\":{\"average_ms\":%.6f,\"median_ms\":%.6f,\"p95_ms\":%.6f,\"p99_ms\":%.6f,\"effective_fps\":%.3f}%s\n",
        name,sum/n*1000,samples[(n-1)/2]*1000,samples[(95*n+99)/100-1]*1000,
        samples[(99*n+99)/100-1]*1000,n/sum,comma?",":"");
}
int main(int argc,char **argv) {
    Map m;Frame f;Vec *points;size_t i,n=0;double begin,t;
    double transform[LIMIT],raster[LIMIT],total[LIMIT];
    struct utsname machine;struct rusage usage;
    if(argc!=2){fprintf(stderr,"usage: planet-bench map.plnt\n");return 2;}
    if(!map_load(&m,argv[1])||!frame_init(&f,320,240)){fprintf(stderr,"initialization failed\n");return 1;}
    points=calloc(m.count,sizeof(Vec));if(!points){frame_free(&f);map_free(&m);return 1;}
    begin=now();
    /* Stop starting samples after 45 seconds; external timeout is also required. */
    while(n<120 && now()-begin<45) {
        double yaw=(double)n*3;
        t=now();for(i=0;i<m.count;i++)points[i]=orient(m.points[i],yaw,-15);
        transform[n]=now()-t;
        t=now();map_raster(&f,&m,points,1);raster[n]=now()-t;
        t=now();map_render(&f,&m,yaw,-15,1);total[n]=now()-t;
        n++;
    }
    if(!n){free(points);frame_free(&f);map_free(&m);return 1;}
    if(uname(&machine)!=0){perror("uname");return 1;}
    printf("{\"schema\":1,\"backend\":\"headless\",\"width\":320,\"height\":240,\"frames_per_measurement\":%zu,\"points\":%u,\"elapsed_seconds\":%.6f,",n,m.count,now()-begin);
    printf("\"os\":\"%s\",\"release\":\"%s\",\"architecture\":\"%s\",",machine.sysname,machine.release,machine.machine);
    if(getrusage(RUSAGE_SELF,&usage)==0)printf("\"peak_rss_kib\":%ld,",usage.ru_maxrss);
    printf("\"measurements\":{\n");
    report("transform",transform,n,1);report("raster",raster,n,1);report("total_headless",total,n,0);
    puts("}}");free(points);frame_free(&f);map_free(&m);return 0;
}
