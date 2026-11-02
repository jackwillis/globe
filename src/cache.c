#include "cache.h"
#include <stdlib.h>
#include <string.h>
static uint32_t get32(const unsigned char *p) {return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put32(unsigned char *p,uint32_t n) {p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static uint64_t get64(const unsigned char *p) {return (uint64_t)get32(p)<<32|get32(p+4);}
static void put64(unsigned char *p,uint64_t n) {put32(p,(uint32_t)(n>>32));put32(p+4,(uint32_t)n);}
uint32_t cache_crc32(const unsigned char *p,size_t length) {
    uint32_t crc=0xffffffffu;size_t i;unsigned bit;
    for(i=0;i<length;i++){crc^=p[i];for(bit=0;bit<8;bit++)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}
    return ~crc;
}
int cache_fresh(CacheMeta meta,uint64_t now,uint64_t ttl) {
    return meta.checked_at>0&&now>=meta.checked_at&&now-meta.checked_at<ttl;
}
void cache_record_free(CacheRecord *record) {free(record->bytes);memset(record,0,sizeof(*record));}
static int valid_payload(const unsigned char *bytes,size_t length) {
    Places places={0};int ok=places_decode(&places,bytes,length);places_free(&places);return ok;
}
int cache_decode(CacheRecord *result,const unsigned char *bytes,size_t length) {
    CacheRecord next={0};uint32_t count;
    if(!bytes||length<36||memcmp(bytes,"PLNTCCH\0",8)||get32(bytes+8)!=1)return 0;
    count=get32(bytes+12);
    if(count>PLACES_MAX_BYTES||length-36!=count||cache_crc32(bytes+36,count)!=get32(bytes+32)
       ||!valid_payload(bytes+36,count))return 0;
    next.bytes=malloc(count);if(!next.bytes)return 0;
    memcpy(next.bytes,bytes+36,count);next.length=count;
    next.meta.received_at=get64(bytes+16);next.meta.checked_at=get64(bytes+24);*result=next;return 1;
}
int cache_encode(const CacheRecord *record,unsigned char **bytes,size_t *length) {
    unsigned char *out;
    if(record->length>PLACES_MAX_BYTES||!valid_payload(record->bytes,record->length))return 0;
    out=malloc(record->length+36);if(!out)return 0;
    memcpy(out,"PLNTCCH\0",8);put32(out+8,1);put32(out+12,(uint32_t)record->length);
    put64(out+16,record->meta.received_at);put64(out+24,record->meta.checked_at);
    put32(out+32,cache_crc32(record->bytes,record->length));memcpy(out+36,record->bytes,record->length);
    *bytes=out;*length=record->length+36;return 1;
}
void transfer_cancel(PlaceTransfer *transfer) {free(transfer->bytes);memset(transfer,0,sizeof(*transfer));}
int transfer_begin(PlaceTransfer *transfer,size_t expected) {
    memset(transfer,0,sizeof(*transfer));
    if(!expected||expected>PLACES_MAX_BYTES)return 0;
    transfer->bytes=malloc(expected);if(!transfer->bytes)return 0;
    transfer->expected=expected;return 1;
}
int transfer_feed(PlaceTransfer *transfer,const unsigned char *bytes,size_t length) {
    if(!transfer->bytes||transfer->failed||length>transfer->expected-transfer->length||(!bytes&&length)) {
        transfer->failed=1;return 0;
    }
    if(length)memcpy(transfer->bytes+transfer->length,bytes,length);
    transfer->length+=length;return 1;
}
int transfer_finish(PlaceTransfer *transfer,CacheMeta meta,CacheRecord *record) {
    if(transfer->failed||!transfer->bytes||transfer->length!=transfer->expected
       ||!valid_payload(transfer->bytes,transfer->length))return 0;
    record->bytes=transfer->bytes;record->length=transfer->length;record->meta=meta;
    memset(transfer,0,sizeof(*transfer));return 1;
}
