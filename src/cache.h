#ifndef PLANET_CACHE_H
#define PLANET_CACHE_H
#include "places.h"
#define CACHE_FRESH_SECONDS (30u*24u*60u*60u)
typedef struct { uint64_t received_at,checked_at; } CacheMeta;
typedef struct { unsigned char *bytes;size_t length;CacheMeta meta; } CacheRecord;
typedef struct { unsigned char *bytes;size_t length,expected;int failed; } PlaceTransfer;
uint32_t cache_crc32(const unsigned char *,size_t);
int cache_fresh(CacheMeta,uint64_t,uint64_t);
void cache_record_free(CacheRecord *);
int cache_decode(CacheRecord *,const unsigned char *,size_t);
int cache_encode(const CacheRecord *,unsigned char **,size_t *);
int transfer_begin(PlaceTransfer *,size_t);
int transfer_feed(PlaceTransfer *,const unsigned char *,size_t);
int transfer_finish(PlaceTransfer *,CacheMeta,CacheRecord *);
void transfer_cancel(PlaceTransfer *);
/* Host filesystem adapter. Mac OS 9 replaces this file, not the core above. */
int cache_read_file(const char *,CacheRecord *);
int cache_write_file(const char *,const CacheRecord *);
#endif
