#ifndef PLAY_WAV_H
#define PLAY_WAV_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <AudioUnit/AudioUnit.h>

#include "audioBaseTypes.h"


typedef struct {
    atomic_bool* finishFlag          ;
    atomic_bool* pause               ;
    AudioReadContext audioReadContext;
    uint32_t     size                ;
    int          bits                ;
    int          channels            ;
    float*       volume              ;
} inUserData_t;

#endif