#ifndef CACHE_H
#define CACHE_H

#include "soundStruct.h"


#define MAX_CACHED_SOUNDS 16

typedef struct {
    char   filePath[256];
    Sound* soundData    ;
} CachedSound;

void initializeCache();
void freeAllCache();

#endif