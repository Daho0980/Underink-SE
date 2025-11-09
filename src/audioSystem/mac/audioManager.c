#include <stdio.h>
#include <pthread.h>

#include "audioStruct.h"

#include "audioTool/management/queue.h"


void* audioManager(AudioManager* mgr) {
    printf("[audioManager] 매니저 루프가 시작됩니다. 현재 요청 수 : \x1b[33m%d\x1b[0m\n", mgr->size);

    AudioRequest req;
    while ( mgr->running ) {
        pthread_mutex_lock(&mgr->mutex);
        while ( !mgr->size && mgr->running ) {
            pthread_cond_wait(&mgr->cond, &mgr->mutex);
        }

        if ( !mgr->running ) {
            pthread_mutex_unlock(&mgr->mutex);
            break;
        }

        pthread_mutex_unlock(&mgr->mutex);
        if ( !queuePop(mgr, &req) ) {
            bool assigned = false;

            for ( int i=0; i<mgr->threadCount; i++ ) {
                AudioChannelData* target = &mgr->threads[i];
                pthread_mutex_lock(&target->mutex);
                if ( !target->hasRequest ) {
                    target->request    = req
                   ;target->hasRequest = true
                   ;
                   pthread_mutex_unlock(&target->mutex);
                   pthread_cond_signal(&target->cond);
                   assigned = true;

                   break;
                }

                pthread_mutex_unlock(&target->mutex);
            }

            if ( !assigned ) {
                fprintf(stderr,
                    "[mgrThread]\x1b[31m(LeisurelyThreadNotFound)\x1b[0m 모든 스레드가 바쁘므로 요청이 거부됩니다.\n"
                );
            }
        }
    }

    return NULL;
}