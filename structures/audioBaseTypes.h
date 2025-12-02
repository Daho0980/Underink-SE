#ifndef AUDIO_BASE_TYPES_H
#define AUDIO_BASE_TYPES_H

#include <stdint.h>


typedef struct {
    uint8_t      *origin   ;
    uint32_t     size      ;
    unsigned int sampleRate;
    unsigned int channels  ;
    unsigned int bits      ;
} Sound;

typedef struct {
    uint8_t* origin   ;
    int      frameSize;
} AudioFrameConfig;

typedef union {
    uint8_t*         audioOrigin;
    AudioFrameConfig config     ;
} AudioModeUnion;

typedef struct {
    uint8_t* origin;
    uint64_t offset;
} AudioReadContext;

static inline AudioFrameConfig initAudioFrameConfig(
    uint8_t *audioData,
    int     frameSize
) {
    AudioFrameConfig output;
    output.origin    = audioData;
    output.frameSize = frameSize;

    return output;
}

#endif