#ifndef PLAY_WAV_H
#define PLAY_WAV_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <AudioUnit/AudioUnit.h>

#include "audioBaseTypes.h"
#include "audioSystemArgs.h"


typedef struct {
    AudioManager* mgr;

    atomic_bool* finishFlag;
    atomic_bool* pause     ;

    AudioReadContext audioReadContext;

    uint32_t     size      ;
    int          bitDepth  ;
    int          channels  ;
    int          sampleRate;

    float*       currVol;
    float*       reflVol;
} inUserData_t;

#endif