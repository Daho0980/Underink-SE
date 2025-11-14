#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "management/cache.h"
#include "management/cache_internal.h"


CachedSound soundCache[MAX_CACHED_SOUNDS];
int         cacheCount = 0;

void initializeCache() {
    memset(soundCache, 0, sizeof(soundCache));
    cacheCount = 0;
}

void freeAllCache() {
    printf("\x1b[33m[INFO]\x1b[0m 모든 캐시를 해제 중... ");

    for ( int i=0; i<cacheCount; i++ ) {
        if ( soundCache[i].soundData != NULL ) {
            if ( soundCache[i].soundData->audioData != NULL ) {
                free(soundCache[i].soundData->audioData);
            }

            free(soundCache[i].soundData);
        }

        memset(&soundCache[i], 0, sizeof(CachedSound));
    }

    cacheCount = 0;

    printf("완료되었습니다.\n");
}

CachedSound* getSoundCache() {
    return soundCache;
}

int* getSoundCacheSize() {
    return &cacheCount;
}