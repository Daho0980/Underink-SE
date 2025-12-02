#ifndef AUDIO_MANAGER_TABLE_H
#define AUDIO_MANAGER_TABLE_H

#include "audioSystemArgs.h"


ManagerTable* initializeManagerTable(int size);
void destroyManagerTable(ManagerTable* mgrt, bool force);

int registerManager(ManagerTable* mgrt, AudioManager* mgr);
int unregisterManager(ManagerTable* mgrt, const char type[3]);
void unregisterAndDestroyManager(ManagerTable* mgrt, AudioManager* mgr, bool force);

int findManagerType(ManagerTable* mgrt, const char type[3]);

#endif