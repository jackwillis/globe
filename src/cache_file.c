#define _POSIX_C_SOURCE 200809L
#include "cache.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static int path_name(char *out,size_t capacity,const char *root,const char *name) {
    int n=snprintf(out,capacity,"%s/%s",root,name);return n>0&&(size_t)n<capacity;
}
static int read_bytes(const char *path,unsigned char **bytes,size_t *length) {
    FILE *f=fopen(path,"rb");long count;unsigned char *data;int ok;
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)||(count=ftell(f))<0||(unsigned long)count>PLACES_MAX_BYTES+36||fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    data=malloc(count?(size_t)count:1);if(!data){fclose(f);return 0;}
    ok=fread(data,1,(size_t)count,f)==(size_t)count&&!ferror(f);if(fclose(f))ok=0;
    if(!ok){free(data);return 0;}*bytes=data;*length=(size_t)count;return 1;
}
static int atomic_write(const char *path,const unsigned char *bytes,size_t length) {
    char temporary[1200];int fd,ok=1;size_t written=0;
    if(snprintf(temporary,sizeof(temporary),"%s.tmp-XXXXXX",path)>=(int)sizeof(temporary))return 0;
    fd=mkstemp(temporary);if(fd<0)return 0;
    while(written<length) {
        ssize_t n=write(fd,bytes+written,length-written);
        if(n<0&&errno==EINTR)continue;
        if(n<=0){ok=0;break;}written+=(size_t)n;
    }
    if(ok&&fsync(fd))ok=0;if(close(fd))ok=0;
    if(ok&&rename(temporary,path))ok=0;
    if(!ok)unlink(temporary);
    return ok;
}
static int directory(const char *path) {
    struct stat info;
    if(mkdir(path,0700)&&errno!=EEXIST)return 0;
    return !stat(path,&info)&&S_ISDIR(info.st_mode);
}
int cache_read_file(const char *root,CacheRecord *record) {
    const char *names[]={"current.plntcache","previous.plntcache"};size_t i;
    for(i=0;i<2;i++) {
        char path[1024];unsigned char *bytes=NULL;size_t length=0;int ok;
        if(!path_name(path,sizeof(path),root,names[i])||!read_bytes(path,&bytes,&length))continue;
        ok=cache_decode(record,bytes,length);free(bytes);if(ok)return 1;
    }
    return 0;
}
static int archive(const char *root,const CacheRecord *record) {
    unsigned index;uint32_t crc=cache_crc32(record->bytes,record->length);
    for(index=0;index<1024;index++) {
        char path[1024],name[96];unsigned char *bytes=NULL;size_t length=0;struct stat info;
        snprintf(name,sizeof(name),"history/%08x-%lu-%u.plnt",crc,(unsigned long)record->length,index);
        if(!path_name(path,sizeof(path),root,name))return 0;
        if(!stat(path,&info)) {
            int same=read_bytes(path,&bytes,&length)&&length==record->length&&!memcmp(bytes,record->bytes,length);
            free(bytes);if(same)return 1;
        } else if(errno==ENOENT)return atomic_write(path,record->bytes,record->length);
        else return 0;
    }
    return 0;
}
int cache_write_file(const char *root,const CacheRecord *record) {
    char current[1024],previous[1024],history[1024];unsigned char *encoded=NULL,*old_bytes=NULL;
    size_t length=0,old_length=0;CacheRecord old={0};int ok=0;
    if(!path_name(current,sizeof(current),root,"current.plntcache")
       ||!path_name(previous,sizeof(previous),root,"previous.plntcache")
       ||!path_name(history,sizeof(history),root,"history")||!cache_encode(record,&encoded,&length))return 0;
    if(!directory(root)||!directory(history)||!archive(root,record))goto cleanup;
    if(read_bytes(current,&old_bytes,&old_length)&&cache_decode(&old,old_bytes,old_length)) {
        if(!archive(root,&old)||!atomic_write(previous,old_bytes,old_length))goto cleanup;
    }
    ok=atomic_write(current,encoded,length);
cleanup:
    free(old_bytes);free(encoded);cache_record_free(&old);return ok;
}
