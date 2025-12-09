#include <string.h>

#include "easing.h"


const EasingEntry easingTable[] = {
    { "linear"      , linear       },
    { "smoothstep"  , smoothstep   },
    { "smootherstep", smootherstep },
};

const int easingTableCount = 
    sizeof(easingTable) / sizeof(EasingEntry);

EasingEntry getEasingFunc(const char* name) {
    for ( int i=0; i<easingTableCount; i++ ) {
        if (strcmp(easingTable[i].name, name) == 0) {
            return easingTable[i];
        }
    }

    return (EasingEntry){ .name="NULL", .func = NULL };
}