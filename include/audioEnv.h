#ifndef AUDIO_ENV_H
#define AUDIO_ENV_H

typedef struct {
    int samplingChunkSize;
} AudioEnvSettings;

extern AudioEnvSettings gAudioEnv;

void setSamplingChunkSize(int size);

#endif