#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "constants.h"
#include "soundStruct.h"
#include "audioStruct.h"


int queuePush(AudioManager* mgr, AudioRequest req) {
    pthread_mutex_lock(&mgr->mutex);
    if ( mgr->size >= MAX_REQUEST_QUEUE ) {
        pthread_mutex_unlock(&mgr->mutex);
        fprintf(stderr,
            "[queuePush]\x1b[31m(RequestQueueFull)\x1b[0m 큐가 가득 찼습니다.\n"
        );

        return 1;
    }

    mgr->queue[mgr->rear] = req
   ;mgr->rear             = (mgr->rear+1) % MAX_REQUEST_QUEUE
   ;mgr->size++
   ;
    pthread_cond_signal(&mgr->cond);
    pthread_mutex_unlock(&mgr->mutex);

    return 0;
}

int queuePop(AudioManager* mgr, AudioRequest* out) {
    if ( mgr->size == 0 ) return 1;

    pthread_mutex_lock(&mgr->mutex);

    *out       = mgr->queue[mgr->front]
   ;mgr->front = (mgr->front+1) % MAX_REQUEST_QUEUE
   ;mgr->size--
   ;
    pthread_mutex_unlock(&mgr->mutex);

    return 0;
}

int enqueueRequest(
    AudioManager* mgr      ,
    Sound*        soundData,
    const char*   firstTag ,
    ...
) {
    AudioRequest req;
    memset(&req, 0, sizeof(AudioRequest));
    req.soundData = soundData;

    {
        va_list tag;
        va_start(tag, firstTag);

        int count = 0;

        const char* currTag = firstTag;
        while ( currTag!=NULL && count<MAX_TAG ) {
            strncpy(req.tags[count], currTag, MAX_TAG_LEN-1);
            req.tags[count][MAX_TAG_LEN-1] = '\0';

            count++;
            currTag = va_arg(tag, const char*);
        }

        va_end(tag);
        if ( currTag != NULL ) {
            fprintf(stderr,
                "[enqueueRequest]\x1b[31m(TooManyTags)\x1b[0m 태그가 너무 많습니다. 최대 태그 수는 %d개 입니다.\n",
                MAX_TAG
            );

            return 1;
        }

    }
    
    if ( queuePush(mgr, req) ) {
        fprintf(stderr,
            "[enqueueRequest]\x1b[31m(RequestEnqueueFailed)\x1b[0m 요청을 큐에 넣는 데 실패했습니다.\n"
        );

        return 1;
    }

    return 0;
}

void _filterIntersection(
    AudioChannelData* main    ,
    AudioChannelData* temp    ,
    int*             mainCount,
    int*             tempCount
) {
    int newCount = 0;
    for ( int i=0; i<*mainCount; i++ ) {
        for ( int t=0; t<*tempCount; t++ ) {
            if (
                main[t].request.soundData ==
                temp[t].request.soundData
             ) {
                main[newCount++] = main[i];
    
                break;
            }
        }
    }
    *mainCount = newCount;
}

/**
 * `outcount`는 `NULL`일 수 있습니다.
 */
AudioChannelData* tagMatch(
    AudioManager* mgr     ,
    int*          outCount,
    const char*   firstTag,
    ...
) {
    pthread_mutex_lock(&mgr->mutex);

    AudioChannelData* filtered = malloc(sizeof(AudioChannelData)*mgr->threadCount)
   ;int threadCount = 0
   ;
    AudioChannelData* tempFiltered = malloc(sizeof(AudioChannelData)*mgr->threadCount)
   ;int tempCount = 0
   ;
    if (
        filtered     == NULL ||
        tempFiltered == NULL
    ) goto cleanup;

    {
        va_list tag;
        va_start(tag, firstTag);

        const char* currTag  = firstTag;
        int         tagIndex = 0       ;

        while ( currTag!=NULL && tagIndex<MAX_TAG ) {
            size_t currTagLen = strlen(currTag);

            tempCount = 0;

            for ( int i=0; i<mgr->threadCount; i++ ) {
                pthread_mutex_t* threadMutex = &mgr->threads[i].mutex;
                pthread_mutex_lock(threadMutex);

                if ( !mgr->threads[i].hasRequest ) {
                    pthread_mutex_unlock(threadMutex);

                    continue;
                }

                for ( int j=0; j<MAX_TAG; j++ ) {
                    const char* requestTag = mgr->threads[i].request.tags[j];
                    if ( requestTag[0] == '\0' ) break;

                    if ( currTagLen > MAX_TAG_LEN ) {
                        va_end(tag);
                        fprintf(stderr,
                            "[tagMatch]\x1b[31m(TooLongTag)\x1b[0m 태그 길이가 너무 깁니다. 최대 태그 길이는 %d 입니다.\n",
                            MAX_TAG_LEN
                        );

                        goto cleanup;
                    }
                    if (
                        strlen(requestTag) == currTagLen             &&
                        strncmp(requestTag, currTag, currTagLen) == 0
                    ) {
                        tempFiltered[tempCount++] = mgr->threads[i];

                        break;
                    }
                }

                pthread_mutex_unlock(threadMutex);
            }

            if ( tagIndex == 0 ) {
                threadCount = tempCount;
                for ( int k=0; k<threadCount; k++ ) filtered[k] = tempFiltered[k];
            } else {
                _filterIntersection(
                    filtered, tempFiltered,
                    &threadCount, &tempCount
                );

                if ( threadCount == 0 ) {
                    va_end(tag);

                    goto cleanup;
                }
            }

            currTag = va_arg(tag, const char*);
            tagIndex++;
        }

        va_end(tag);

        if ( currTag != NULL ) {
            fprintf(stderr,
                "[tagMatch]\x1b[31m(TooManyTags)\x1b[0m 태그가 너무 많습니다. 최대 태그 수는 %d개 입니다.\n",
                MAX_TAG
            );

            goto cleanup;
        }
    }

    free(tempFiltered);
    pthread_mutex_unlock(&mgr->mutex);
    if ( outCount ) *outCount = threadCount;

    return filtered;

    cleanup:
        free(filtered);
        free(tempFiltered);
        pthread_mutex_unlock(&mgr->mutex);
        if ( outCount ) *outCount = 0;

        return NULL;
}