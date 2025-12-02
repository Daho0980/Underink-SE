#ifndef AUDIO_ENV_H
#define AUDIO_ENV_H

typedef struct {
    int samplingFrameSize;
} AudioEnvSettings;

extern AudioEnvSettings gAudioEnv;

void setSamplingFrameSize(int size);

#endif