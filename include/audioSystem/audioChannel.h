#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include <stdbool.h>

#include "audioStruct.h"


typedef struct AudioChannel_t AudioChannel_t;

void destroyAllChannelThreads(AudioManager* mgr, int index, bool force);
AudioChannel_t* audioChannel(void* arg);

#endif