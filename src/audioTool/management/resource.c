#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "wavFormat.h"
#include "audioBaseTypes.h"

#include "management/cache_internal.h"

typedef struct {
    char     chunkID[4];
    uint32_t chunkSize ;
} ChunkHeader;


Sound* loadSound(const char* filePath) {
    printf("[loadSound] loadSound 호출됨\n");
    FILE* fp = fopen(filePath, "rb");
    if ( !fp ) {
        perror("파일을 여는 데 실패했습니다. ");

        return NULL;
    }

    RIFFHeader riff;
    fread(&riff, sizeof(RIFFHeader), 1, fp);
    if (
        memcmp(riff.chunkID, "RIFF", 4)!=0 ||
        memcmp(riff.format , "WAVE", 4)!=0
    ) {
        fprintf(stderr,
            "[loadSound]\x1b[31m(NotAWav)\x1b[0mWAV 파일이 아닙니다.\n"
        );
        fclose(fp);

        return NULL;
    }

    ChunkHeader ch;
    FMTSubchunk fmt;
    {
        bool foundFmt = false;
        while ( fread(&ch, sizeof(ChunkHeader), 1, fp) == 1 ) {
            if ( memcmp(ch.chunkID, "fmt ", 4) == 0 ) {
                fmt.subchunk1Size = ch.chunkSize;
                fread(&fmt.audioFormat, ch.chunkSize, 1, fp);

                foundFmt = true;
    
                break;
            } else {
                fseek(fp, ch.chunkSize, SEEK_CUR);
            }
        }

        if ( !foundFmt ) {
            fprintf(stderr,
                "[loadSound]\x1b[31m(CannotFoundChunk)\x1b[0mfmt 청크를 찾을 수 없습니다.\n"
            );
            fclose(fp);

            return NULL;
        }
    }

    if ( fmt.subchunk1Size > 16 ) {
        fseek(fp, fmt.subchunk1Size-16, SEEK_CUR);
    }

    DataSubchunk data;
    while ( 1 ) {
        fread(&data, sizeof(DataSubchunk), 1, fp);
        if ( memcmp(data.subchunk2ID, "data", 4) == 0 ) break;
        fseek(fp, data.subchunk2Size, SEEK_CUR);
    }

    uint8_t* index = malloc(data.subchunk2Size);
    fread(index, data.subchunk2Size, 1, fp);
    fclose(fp);

    Sound* sound = malloc(sizeof(Sound));
    if ( sound ) {
        sound->origin     = index             ;
        sound-> size      = data.subchunk2Size;
        sound->sampleRate = fmt.sampleRate    ;
        sound->channels   = fmt.numChannels   ;
        sound->bitDepth   = fmt.bitsPerSample ;
    }
    printf("[loadSound]\x1b[32m(SUCCESS)\x1b[0m 데이터를 성공적으로 불러왔습니다.\n");

    return sound;
}

Sound* _copyFromOriginal(Sound *cached) {
    printf("[_copyFromOriginal] _copyFromOriginal 호출됨\n");
    Sound *newSound = malloc(sizeof(Sound));
    if ( newSound == NULL ) return NULL;

    newSound->origin = malloc(cached->size);
    if ( newSound->origin == NULL ) {
        free(newSound);

        return NULL;
    }

    memcpy(
        newSound->origin, cached->origin,
        cached->size
    );
    newSound->size       = cached->size
   ;newSound->sampleRate = cached->sampleRate
   ;newSound->channels   = cached->channels
   ;newSound->bitDepth   = cached->bitDepth
   ;
    printf("[_copyFromOriginal]\x1b[32m(SUCCESS)\x1b[0m 복제된 데이터를 반환합니다.\n");

    return newSound;
}

Sound* getSound(const char* filePath) {
    printf("[getSound] getSound 호출됨\n");
    CachedSound* soundCache = getSoundCache();

    for ( int i=0; i<MAX_CACHED_SOUNDS; i++ ) {
        if ( strcmp(soundCache[i].filePath, filePath) == 0 ) {
            printf(
                "[getSound] \x1b[33m'%s'\x1b[0m 사운드를 캐시에서 발견했습니다.\n",
                filePath
            );

            Sound* newSound = _copyFromOriginal(soundCache[i].soundData);
            if ( newSound == NULL ) return NULL;

            return newSound;
        }
    }

    int* cacheCount = getSoundCacheSize();
    if ( *cacheCount >= MAX_CACHED_SOUNDS ) {
        fprintf(stderr,
            "[getSound]\x1b[31m(CacheFull)\x1b[0m 캐시가 가득 찼습니다. 새로운 사운드를 추가할 수 없습니다.\n"
        );

        return NULL;
    }

    printf(
        "[getSound]\x1b[33m(CachedNotFound)\x1b[0m 캐시된 사운드가 없습니다. \x1b[32m'%s'\x1b[0m 파일에서 데이터를 가져옵니다.\n",
        filePath
    );
    Sound* newSound = loadSound(filePath);
    if ( newSound == NULL ) {
        fprintf(stderr,
            "[getSound]\x1b[31m(SoundLoadFailed)\x1b[0m '%s' 사운드를 불러오는 데 실패했습니다.\n",
            filePath
        );

        return NULL;
    }

    strcpy(soundCache[*cacheCount].filePath, filePath);
    soundCache[*cacheCount].soundData = newSound;
    (*cacheCount)++;

    printf("[getSound]\x1b[32m(SUCCESS)\x1b[0m 성공적으로 캐시되었습니다.\n");

    Sound* outSound = _copyFromOriginal(newSound);
    if ( outSound == NULL ) return NULL;

    return outSound;
}

void freeSound(Sound* soundData) {
    free(soundData->origin);
    free(soundData);
}