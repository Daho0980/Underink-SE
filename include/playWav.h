#ifndef PLAY_WAV_H
#define PLAY_WAV_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "soundStruct.h"


typedef struct {
    atomic_bool* finishFlag;
    atomic_bool* pause     ;
    SampleSound sampleSound;
    uint32_t    size       ;
    int         bits       ;
    float*      volume     ;
} inUserData_t;

#endif