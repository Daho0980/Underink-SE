#ifndef WAV_FORMAT_H
#define WAV_FORMAT_H
// I think this name is sucks

#include <stdint.h>


#pragma pack(push, 1)
typedef struct {
    char     chunkID[4];
    uint32_t chunkSize ;
    char     format[4] ;
} RIFFHeader;

typedef struct {
    char     subchunk1ID[4];
    uint32_t subchunk1Size ;
    uint16_t audioFormat   ;
    uint16_t numChannels   ;
    uint32_t sampleRate    ;
    uint32_t byteRate      ;
    uint16_t blockAlign    ;
    uint16_t bitsPerSample ;
} FMTSubchunk;

typedef struct {
    char     subchunk2ID[4];
    uint32_t subchunk2Size ;
} DataSubchunk;
#pragma pack(pop)

#endif