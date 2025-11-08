#ifndef SET_MANAGER_H
#define SET_MANAGER_H

#include "audioThreadStruct.h"
#include "constants.h"


void initializeManager(AudioManager* mgr, int threadCount, const char* type);
void cleanupManager(AudioManager* mgr);

#endif