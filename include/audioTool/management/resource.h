#ifndef RESOURCE_H
#define RESOURCE_H

#include "audioBaseTypes.h"


Sound* loadSound(const char* filePath);
Sound* getSound(const char* filePath);
void freeSound(Sound* soundData);

#endif