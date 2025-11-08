#ifndef QUEUE_H
#define QUEUE_H

#include "audioThreadStruct.h"


int queuePush(AudioManager* mgr, AudioRequest req);
int queuePop(AudioManager* mgr, AudioRequest* out);
int enqueueRequest(AudioManager* mgr, Sound* soundData, const char* firstTag, ...);
AudioThreadData* tagMatch(AudioManager* mgr, int* outCount, const char* firstTag, ...);

#endif