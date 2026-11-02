#ifndef PLANET_BENCH_META_H
#define PLANET_BENCH_META_H
#include <stdio.h>
#ifndef PLANET_BUILD_FLAGS
#define PLANET_BUILD_FLAGS "unspecified; record compiler invocation"
#endif
static void bench_build_flags(void) {
    const unsigned char *p=(const unsigned char *)PLANET_BUILD_FLAGS;
    fputs("\"build_flags\":\"",stdout);
    for(;*p;p++) {
        if(*p=='"'||*p=='\\'){putchar('\\');putchar(*p);}
        else if(*p<32)printf("\\u%04x",(unsigned)*p);
        else putchar(*p);
    }
    fputs("\",",stdout);
}
#endif
