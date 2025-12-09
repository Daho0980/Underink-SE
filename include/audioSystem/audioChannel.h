#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include <stdbool.h>

#include "audioSystemArgs.h"


#define VOLUME_SCALE 0.0244200244f

struct LocalVol {
    float baseVolume     ;
    float currVolume     ;
    float reflectedVolume;

    bool  fadeActive ;
    float decayFactor;
    float target     ;
};
inline struct LocalVol LocalVol_init() {
    struct LocalVol out = {
        .baseVolume      = 100.0,
        .currVolume      = 1.0,
        .reflectedVolume = 1.0,
        .fadeActive      = false,
        .decayFactor     = 0.0,
        .target          = 0.0
    };

    return out;
}

typedef struct AudioChannel_t AudioChannel_t;

void destroyAllChannelThreads(AudioManager* mgr, int index, bool force);
AudioChannel_t* audioChannel(void* arg);

#endif