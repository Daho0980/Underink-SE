#ifndef AUDIO_STRUCT_H
#define AUDIO_STRUCT_H

#include <time.h>
#include <float.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "constants.h"
#include "soundStruct.h"


#define MAX_COMMAND_QUEUE 4

typedef struct {
    Sound* soundData                 ;
    char   tags[MAX_TAG][MAX_TAG_LEN];
} AudioRequest;

typedef struct {
    uint64_t mixTotal;
    int      head    ;
    int      tail    ;

    pthread_mutex_t mutex;
} CommandQueue;

typedef struct {
    int          id        ;
    atomic_bool  running   ;
    atomic_bool  finished  ;
    bool         hasRequest;
    AudioRequest request   ;
    CommandQueue command   ;

    pthread_mutex_t mutex;
    pthread_cond_t  cond ;
} AudioChannelData;

typedef struct {
    atomic_bool running;

    AudioChannelData* threads   ;
    int              threadCount;
    float            volume     ;

    AudioRequest     queue[MAX_REQUEST_QUEUE];
    int              front                   ;
    int              rear                    ;
    int              size                    ;

    char type[3];
    char mode[8];

    pthread_mutex_t mutex;
    pthread_cond_t  cond ;
} AudioManager;

typedef struct {
    AudioChannelData* thread;
    AudioManager*    mgr    ;
} ThreadArg;

#endif