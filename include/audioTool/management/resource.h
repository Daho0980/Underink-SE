#ifndef RESOURCE_H
#define RESOURCE_H

#include "soundStruct.h"


Sound* loadSound(const char* filePath);
Sound* getSound(const char* filePath);
void freeSound(Sound* sound);

#endif