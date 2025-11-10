#ifndef PLAY_WAV_H
#define PLAY_WAV_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <AudioUnit/AudioUnit.h>

#include "soundStruct.h"


typedef struct {
    atomic_bool* finishFlag;
    atomic_bool* pause     ;
    SampleSound sampleSound;
    uint32_t    size       ;
    int         bits       ;
    float*      volume     ;
} inUserData_t;

void play(AudioUnit* queue, inUserData_t* inUserData, const char mode[8], SampleOrDataOnly data, uint32_t size, int sampleRate, int channels, int bits);

#endif