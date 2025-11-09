// ##########################################################
// 
// 이 코드는 현재 버전의 코드와 호환되지 않습니다.
// 코드에 존재하는 모든 함수는 외부로 분산 및 이동되었습니다.
//
// ##########################################################

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include "soundStruct.h"
#include "audioStruct.h"
#include "wavFormat.h"

#include "playWav.h"
#include "constants.h"


void* audioChannelThread(void* arg) {
    AudioChannelData* data = (AudioChannelData*) arg;

    pthread_mutex_lock(&data->mutex);

    while ( data->running ) {
        while ( !data->hasRequest ) {
            pthread_cond_wait(&data->cond, &data->mutex);
        }

        char totalTag[
            (MAX_TAG_LEN*MAX_TAG) + 
            (2*(MAX_TAG-1))       +
            1
        ]; totalTag[0] = '\0';
        {
            char (*tagIter)[MAX_TAG_LEN] = data->request.tags;
            int  count          = 0;
    
            if ( **tagIter != '\0' ) {
                strcpy(totalTag, *tagIter);
                tagIter++;
                count  ++;
            }
            
            while ( **tagIter!='\0' && count<MAX_TAG ) {
                strcat(totalTag, ", ");
                strcat(totalTag, *tagIter);
                tagIter++;
                count  ++;
            }
        }

        printf(
            "[Thread %d(%s)] 재생 시작\n",
            data->id,
            totalTag
        );
        pthread_mutex_unlock(&data->mutex);
        // wait the time

        pthread_mutex_lock(&data->mutex);
        printf("[Thread %d(%s)] 재생 완료\n", data->id, totalTag);

        data->hasRequest = false;
    }

    pthread_mutex_unlock(&data->mutex);

    return NULL;
}

void dispatchRequest(AudioManager* mgr) {
    AudioRequest req;
    if ( queuePop(mgr, &req) ) return;

    for ( int i=0; i<mgr->threadCount; i++ ) {
        AudioChannelData* t = &mgr->threads[i];
        pthread_mutex_lock(&t->mutex);
        if ( !t->hasRequest ) {
            t->request    = req ;
            t->hasRequest = true;

            pthread_cond_signal(&t->cond);
            pthread_mutex_unlock(&t->mutex);

            return;
        }

        pthread_mutex_unlock(&t->mutex);
    }
    queuePush(mgr, req);
}

bool checkThread(AudioManager* mgr) {
    for ( int i=0; i<mgr->threadCount; i++ ) {
        AudioChannelData* t = &mgr->threads[i];
        pthread_mutex_lock(&t->mutex);
        if ( t->hasRequest ) {
            pthread_mutex_unlock(&t->mutex);

            return false;
        }

        pthread_mutex_unlock(&t->mutex);
    }

    return true;
}

int _initializeThread(AudioChannelData* thread, int id) {
    thread->id         = id   ;
    thread->running    = true ;
    thread->hasRequest = false;

    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);

    pthread_t tid;
    if ( pthread_create(&tid, NULL, audioChannelThread, thread) != 0 ) {
        pthread_mutex_destroy(&thread->mutex);
        pthread_cond_destroy(&thread->cond);

        return 1;
    }
    pthread_detach(tid);

    return 0;
}

AudioChannelData* _initializeThreadData(int count) {
    return (AudioChannelData*)malloc(sizeof(AudioChannelData)*count);
}

void _setManagerParams(
    AudioManager* mgr        ,
    const char    type[3]    ,
    int           threadCount
) {
    memcpy(mgr->type, type, sizeof(mgr->type));
    
    mgr->threadCount = threadCount
   ;mgr->front = mgr->rear = mgr->size = 0
   ;
}

void _destroyAllThreads(AudioManager* mgr, int index) {
    for ( int i=(index-1); i>=0; i-- ) {
        pthread_mutex_lock(&mgr->threads[i].mutex);
        mgr->threads[i].running = false;
        pthread_cond_signal(&mgr->threads[i].cond);
        pthread_mutex_unlock(&mgr->threads[i].mutex);

        pthread_mutex_destroy(&mgr->threads[i].mutex);
        pthread_cond_destroy(&mgr->threads[i].cond);
    }
}

/**
 * # 타입
 * 에 따라 다른
 * # 매니저
 * 를
 * # 반환
 * 합니다.
 */
AudioManager* initializeManager(const char type[3]) {
    AudioManager* mgr = (AudioManager*)malloc(sizeof(AudioManager));
    if ( mgr == NULL ) {
        perror("Malloc failed");

        return NULL;
    }

    pthread_mutex_init(&mgr->mutex, NULL);

    if ( strcmp(type, "BGM") == 0 ) {
        mgr->threads = _initializeThreadData(BGM_THREADS);
        if ( mgr->threads == NULL ) {
            perror("Threads malloc failed");

            goto cleanup;
        }

        _setManagerParams(mgr, type, BGM_THREADS);

        for ( int i=0; i<BGM_THREADS; i++ ) {
            if ( _initializeThread(&mgr->threads[i], i) != 0 ) {
                _destroyAllThreads(mgr, i);
                free(mgr->threads);

                goto cleanup;
            }
        }

        return mgr;
    }

    if ( strcmp(type, "SFX") == 0 ) {
        mgr->threads = _initializeThreadData(SFX_THREADS);
        if ( mgr->threads == NULL ) {
            perror("Threads malloc failed");

            goto cleanup;
        }

        _setManagerParams(mgr, type, SFX_THREADS);

        for( int i=0; i<SFX_THREADS; i++ ) {
            if ( _initializeThread(&mgr->threads[i], i) != 0 ) {
                _destroyAllThreads(mgr, i);
                free(mgr->threads);

                goto cleanup;
            }
        }

        return mgr;
    }

    cleanup:
        pthread_mutex_destroy(&mgr->mutex);
        free(mgr);

        return NULL;
}

/**
 * 사실상 사용법입니다.
 * 제가 만든 거 아닙니다.
 */
int main() {
    AudioManager* bgmMgr = initializeManager("BGM");
    AudioManager* sfxMgr = initializeManager("SFX");

    printf("[MAIN] 메인 루프 시작\n");

    printf("[MAIN] SFX/BGM 요청이 각각 10개씩 제공됩니다.\n");
    printf("[INFO] SFX 스레드 수 : %d\n", SFX_THREADS);
    printf("[INFO] BGM 스레드 수 : %d\n\n", BGM_THREADS);
    for ( int i=0; i<10; i++ ) {
        enqueueRequest(bgmMgr, 2, 0);
        enqueueRequest(sfxMgr, 1, 1);
    }

    while ( 1 ) {
        dispatchRequest(bgmMgr);
        dispatchRequest(sfxMgr);

        if ( (
            checkThread(bgmMgr) &&
            checkThread(sfxMgr)
        ) ) { break; }
    }

    printf("[MAIN] 메인 루프가 종료됩니다.\n");
}