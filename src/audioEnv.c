#include "audioEnv.h"


AudioEnvSettings gAudioEnv = {
    .samplingFrameSize = 512
};

void setSamplingFrameSize(int size) {
    gAudioEnv.samplingFrameSize = size;
}