#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

#include "platform.h"
#include "audioStruct.h"

#include "stdcmd.h"
#include "audioTool/management/queue.h"
#include "audioTool/management/resource.h"

#include "audioSystem/audioChannel.h"
#include "audioSystem/audioManager.h"
#include "commandQueue.h"


typedef struct { AudioManager* mgr; } ManagerTableUnit;
typedef struct {
    int              max    ;
    int              count  ;
    ManagerTableUnit table[];
} ManagerTable;


void destroyAllThreads(AudioManager* mgr, int index) {
    for ( int i=(index-1); i>=0; i-- ) {
        printf("[destroyAllThreads] \x1b[33m%d\x1b[0m번째 스레드 파괴 중...", i);
        AudioChannelData* target = &mgr->threads[i];

        atomic_store(&target->running, false);
        pthread_cond_signal(&target->cond);
        commandPush(target, pkcmd_u8(cmd_stop()));
        while ( !atomic_load(&target->finished) ) {
            usleep(1000);
        }

        pthread_mutex_destroy(&target->mutex);
        pthread_cond_destroy(&target->cond);
        printf(" 완료\n");
    }

    free(mgr->threads);
    printf("[destroyAllThreads]\x1b[32m(SUCCESS)\x1b[0m 모든 스레드가 파괴되었습니다.\n");
}

void destroyManager(AudioManager* mgr) {
    printf("[destroyManager] 매니저 파괴 중...\n");
    destroyAllThreads(mgr, mgr->threadCount);
    pthread_mutex_destroy(&mgr->mutex);
    free(mgr);
    printf("[destroyManager]\x1b[32m(SUCCESS)\x1b[0m 매니저가 파괴되었습니다.\n");
}

void destroyManagerTable(ManagerTable* mgrt) {
    printf("[destroyManagerTable] 매니저 테이블 파괴 중...\n");
    for ( int i=0; i<mgrt->count; i++ ) {
        destroyManager(mgrt->table[i].mgr);
    }

    free(mgrt);
    printf("[destroyManagerTable]\x1b[32m(SUCCESS)\x1b[0m 매니저 테이블이 파괴되었습니다.\n");
}

ManagerTable* initializeManagerTable(int size) {
    ManagerTable* mgrt = malloc(sizeof(ManagerTable) + (sizeof(ManagerTableUnit)*size));
    if ( mgrt == NULL ) {
        printf("[initializeManagerTable]\x1b[31m(NewManagerTableFailed)\x1b[0m 새 매니저 테이블 생성 도중 문제가 발생했습니다.\n");

        return NULL;
    }

    mgrt->max   = size;
    mgrt->count = 0   ;

    printf("[initializeManagerTable]\x1b[32m(SUCCESS)\x1b[0m 매니저 테이블이 성공적으로 생성되었습니다.\n");

    return mgrt;
}

int findManagerType(ManagerTable* mgrt, const char type[3]) {
    for ( int i=0; i<mgrt->count; i++ ) {
        if ( memcmp(mgrt->table[i].mgr->type, type, 3) == 0 ) {
            return i;
        }
    }

    return -1;
}

int registerManager(
    ManagerTable* mgrt,
    AudioManager* mgr
) {
    printf("[registerManager] registerManager 호출됨\n");
    if ( mgrt->count >= mgrt->max ) {
        fprintf(stderr,
            "[registerManager]\x1b[31m(ManagerTableFull)\x1b[0m 매니저 테이블이 가득 찼습니다.\n"
        );
        if ( mgrt->count > mgrt->max ) {
            fprintf(stderr,
                "[registerManager]\x1b[33m(TooManyManagers)\x1b[0m 현재 테이블의 매니저 수가 최대 매니저 수보다 많습니다. 이는 비정상적인 동작으로 간주될 수 있습니다.\n"
            );
        }

        return 1;
    }

    mgrt->table[mgrt->count].mgr = mgr
   ;mgrt->count++
   ;
    printf("[registerManager]\x1b[32m(SUCCESS)\x1b[0m 매니저 \x1b[33m%p\x1b[0m가 등록되었습니다.\n", mgr);

    return 0;
}

int unregisterManager(
    ManagerTable* mgrt,
    const char    type[3]
) {
    int idx = findManagerType(mgrt, type);
    if ( idx == -1 ) {
        fprintf(stderr,
            "[unregisterManager]\x1b[31m(TypeNotFound)\x1b[0m \x1b[33m%s\x1b[0m 매니저를 테이블에서 찾지 못했습니다.\n",
            type
        );

        return 1;
    }

    if ( mgrt->count > 1 ) {
        memmove(
            &mgrt->table[idx], &mgrt->table[idx+1],
            (mgrt->count-idx-1)*sizeof(ManagerTableUnit)
        );
    }
    mgrt->count--;

    printf("[unregisterManager]\x1b[32m(SUCCESS)\x1b[0m \x1b[33m%.*s\x1b[0m 매니저가 등록 해제되었습니다.\n", 3, type);

    return 0;
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

AudioChannelData* _allocateChannelThread(int count) {
    return (AudioChannelData*)malloc(sizeof(AudioChannelData)*count);
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

AudioManager* initializeManager(
    ManagerTable* mgrt,
    const char    type[3],
    const char    mode[8],
    int           threadCount
) {
    printf("[initializeManager] initializeManager 호출됨\n");
    if ( findManagerType(mgrt, type) != -1 ) {
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

    mgr->threads = _allocateChannelThread(threadCount);
    if ( mgr->threads == NULL ) {
        perror("Channel threads malloc failed");

        goto cleanup;
    }
    printf("[initializeManager]\x1b[32m(SUCCESS)\x1b[0m 스레드 메모리가 할당되었습니다.\n");

    _setManagerParams(mgr, type, mode, threadCount);

    for ( int i=0; i<threadCount; i++ ) {
        if ( _initializeChannelData(&mgr->threads[i], i, mgr) == 1 ) {
            destroyAllThreads(mgr, i);

            goto cleanup;
        }
        printf("[initializeManager]\x1b[32m(SUCCESS)\x1b[0m %d번째 스레드가 초기화되었습니다.\n", i);
    }

    registerManager(mgrt, mgr);
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

void unregisterAndDestroyManager(ManagerTable* mgrt, AudioManager* mgr) {
    unregisterManager(mgrt, mgr->type);
    destroyManager(mgr);
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


int main() {
    printf("\n[ -- main 시작 -- ]\n\n");
    ManagerTable* table  = initializeManagerTable(4);
    // AudioManager* bgmmgr = setBGMManager(table);
    // AudioManager* dupmgr = setBGMManager(table);
    // unregisterAndDestroyManager(table, bgmmgr);

    AudioManager* mgr = initializeManager(table, "TST", "SAMPLING", 1);

    // enqueueRequest(
    //     mgr,
    //     getSound("assets/test1.wav"),
    //     "smash", NULL
    // );
    // enqueueRequest(
    //     mgr,
    //     getSound("assets/test.wav"),
    //     "blinkin'", NULL
    // );
    // enqueueRequest(
    //     mgr,
    //     getSound("assets/test2.wav"),
    //     "nevermind.",
    //     "nevermind.",
    //     "nevermind.",
    //     "nevermind.",
    //     "nevermind.",
    //     "nevermind.",
    //     "nevermind.",
    //     "nevermind.",
    //     "nevermind."
    // );
    enqueueRequest(
        mgr,
        getSound("assets/test2.wav"),
        "music", NULL
    );
    usleep(3000000);
    printf("\n\x1b[31m[ -- 3초 경과 -- ]\x1b[0m\n");
    commandPush(&mgr->threads[0], pkcmd_u8(cmd_pause()));

    usleep(1000000);
    printf("\n\x1b[31m[ -- 4초 경과 -- ]\x1b[0m\n");
    commandPush(&mgr->threads[0], pkcmd_u8(cmd_continue()));

    usleep(3000000);
    printf("\n\x1b[31m[ -- 7초 경과 -- ]\x1b[0m\n");
    mgr->volume = 50.0;
    commandPush(&mgr->threads[0], pkcmd_u8(cmd_setVolume()));

    usleep(4000000);
    printf("\n\x1b[31m[ -- 11초 경과 -- ]\x1b[0m\n");
    // printf("\x1b[33m이 명령\x1b[0m은 \x1b[32m스레드\x1b[0m를 \x1b[31m죽입니다!\x1b[0m\n\n");
    // commandPush(&mgr->threads[0], pkcmd_u8(cmd_stop()));
    mgr->volume = 100.0;
    commandPush(&mgr->threads[0], pkcmd_u8(cmd_setVolume()));
    printf("볼륨이 100%%로 재설정됨\n");
    commandPush(&mgr->threads[0], pkcmd_u32(cmd_fade(0, 9000)));
    printf("페이드 적용됨\n");

    unregisterAndDestroyManager(table, mgr);
    destroyManagerTable(table);
    printf("\n[ -- main 종료 -- ]\n\n");
    return 0;
}