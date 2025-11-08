#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>


typedef struct {
    uint8_t      *audioData;
    uint32_t     size      ;
    unsigned int sampleRate;
    unsigned int channels  ;
    unsigned int bits      ;
} Sound;

typedef struct {
    uint8_t *audioData;
    int      chunkSize;
    uint64_t offset   ;
} SampleSound;

typedef union {
    uint8_t     *dataOnly ;
    SampleSound sampleData;
} SampleOrDataOnly;

static inline SampleSound sampleSound_init(
    uint8_t *audioData,
    int     chunkSize
) {
    SampleSound output;
    output.audioData = audioData;
    output.chunkSize = chunkSize;
    output.offset    = 0        ;

    return output;
}

#endif