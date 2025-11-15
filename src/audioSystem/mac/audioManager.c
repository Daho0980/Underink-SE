#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "audioStruct.h"

#include "audioSystem/audioChannel.h"
#include "audioSystem/audioManagerTable.h"
#include "queue.h"


AudioManager* initializeManager(ManagerTable* mgrt, const char type[3], const char mode[8], int threadCount);
void destroyManager(AudioManager* mgr, bool force);
void* audioManager(AudioManager* mgr);

static void _setManagerParams(AudioManager* mgr, const char type[3], const char mode[8], int threadCount);
static int _initializeChannelData(AudioChannelData* channel, int id, AudioManager* mgr);

AudioManager* setBGMManager(ManagerTable* mgrt);
AudioManager* setSFXManager(ManagerTable* mgrt);


AudioManager* initializeManager(
    ManagerTable* mgrt,
    const char    type[3],
    const char    mode[8],
    int           threadCount
) {
    printf("[initializeManager] initializeManager 호출됨\n");
    if ( !mgrt )  {
        printf("[initializeManager] 매니저 테이블이 설정되지 않았습니다. 해당 매니저는 테이블에 등록되지 않습니다.\n");
    }

    if ( mgrt && findManagerType(mgrt, type) != -1 ) {
        fprintf(stderr,
            "[initializeManager]\x1b[31m(DuplicatedManagerType)\x1b[0m 동일한 타입의 매니저가 이미 존재합니다. 현재 : \x1b[33m%.*s\x1b[0m\n",
            3, type
        );

        return NULL;
    }

    AudioManager* mgr = (AudioManager*)malloc(sizeof(AudioManager));
    if ( mgr == NULL ) {
        perror("Malloc failed");

        return NULL;
    }
    printf("[initializeManager]\x1b[32m(SUCCESS)\x1b[0m 매니저 구조체가 할당되었습니다 : \x1b[33m%p\x1b[0m\n", mgr);

    pthread_mutex_init(&mgr->mutex, NULL);
    pthread_cond_init(&mgr->cond, NULL);

    mgr->threads = (AudioChannelData*)malloc(sizeof(AudioChannelData)*threadCount);
    if ( mgr->threads == NULL ) {
        perror("Channel threads malloc failed");

        goto cleanup;
    }
    printf("[initializeManager]\x1b[32m(SUCCESS)\x1b[0m 스레드 메모리가 할당되었습니다.\n");

    _setManagerParams(mgr, type, mode, threadCount);

    for ( int i=0; i<threadCount; i++ ) {
        if ( _initializeChannelData(&mgr->threads[i], i, mgr) == 1 ) {
            destroyAllChannelThreads(mgr, i, true);

            goto cleanup;
        }
        printf("[initializeManager]\x1b[32m(SUCCESS)\x1b[0m %d번째 스레드가 초기화되었습니다.\n", i);
    }

    if ( mgrt ) registerManager(mgrt, mgr);
    mgr->running = true;

    pthread_t tid;
    if ( pthread_create(&tid, NULL, (void*)audioManager, mgr) != 0 ) {
        fprintf(stderr,
            "[initializeManager]\x1b[31m(ThreadCreationFailed)\x1b[0m 매니저 스레드 초기화 도중 문제가 발생했습니다.\n"
        );

        goto cleanup;
    }
    pthread_detach(tid);

    return mgr;

    cleanup:
        pthread_mutex_destroy(&mgr->mutex);
        pthread_cond_destroy(&mgr->cond);
        free(mgr);

        return NULL;
}

void _setManagerParams(
    AudioManager* mgr        ,
    const char    type[3]    ,
    const char    mode[8]    ,
    int           threadCount
) {
    strncpy(mgr->type, type, 3);
    strncpy(mgr->mode, mode, 8);

;mgr->threadCount = threadCount
;mgr->front = mgr->rear = mgr->size = 0
;
}

int _initializeChannelData(
    AudioChannelData* channel,
    int              id      ,
    AudioManager*    mgr
) {
    channel->id               = id
;channel->running          = true
;channel->finished         = false
;channel->hasRequest       = false
;channel->command.mixTotal = 0
;
    pthread_mutex_init(&channel->mutex, NULL);
    pthread_cond_init(&channel->cond, NULL);

    pthread_t tid;

    ThreadArg* arg = malloc(sizeof(ThreadArg));
    arg->thread = channel;
    arg->mgr    = mgr    ;
    printf("[_initializeChannelData] %d번째 스레드 \x1b[33m%p\x1b[0m 할당됨\n", id, arg);

    if ( pthread_create(&tid, NULL, (void*)audioChannel, arg) != 0 ) {
        free(arg);
        pthread_mutex_destroy(&channel->mutex);
        pthread_cond_destroy(&channel->cond);

        return 1;
    }
    pthread_detach(tid);

    return 0;
}

AudioManager* setBGMManager(ManagerTable* mgrt) {
    printf("[setBGMManager] setBGMManager 호출됨\n");
    AudioManager *mgr = initializeManager(
        mgrt,
        "BGM",
        "SAMPLING",
        BGM_THREADS
    );
    if ( mgr == NULL ) return NULL;

    return mgr;
}

AudioManager* setSFXManager(ManagerTable* mgrt) {
    AudioManager *mgr = initializeManager(
        mgrt,
        "SFX",
        "ALLINONE",
        SFX_THREADS
    );
    if( mgr == NULL ) return NULL;

    return mgr;
}

void destroyManager(AudioManager* mgr, bool force) {
    printf("[destroyManager] 매니저 파괴 중...\n");
    destroyAllChannelThreads(mgr, mgr->threadCount, force);
    pthread_mutex_destroy(&mgr->mutex);
    free(mgr);
    printf("[destroyManager]\x1b[32m(SUCCESS)\x1b[0m 매니저가 파괴되었습니다.\n");
}

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