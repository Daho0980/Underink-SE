#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <stdbool.h>

#include "audioStruct.h"


typedef struct AudioManager_t AudioManager_t;

AudioManager* initializeManager(ManagerTable* mgrt, const char type[3], const char mode[8], int threadCount);
void destroyManager(AudioManager* mgr, bool force);
AudioManager_t* audioManager(AudioManager* mgr);

AudioManager* setBGMManager(ManagerTable* mgrt);
AudioManager* setSFXManager(ManagerTable* mgrt);

#endif
