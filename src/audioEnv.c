#include "audioEnv.h"


AudioEnvSettings gAudioEnv = {
    .samplingChunkSize = 1024
};

void setSamplingChunkSize(int size) {
    gAudioEnv.samplingChunkSize = size;
}