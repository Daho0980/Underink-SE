#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audioSystemArgs.h"

#include "audioSystem/audioManager.h"


ManagerTable* initializeManagerTable(int size);
void destroyManagerTable(ManagerTable* mgrt, bool force);

int registerManager(ManagerTable* mgrt, AudioManager* mgr);
int unregisterManager(ManagerTable* mgrt, const char type[3]);
void unregisterAndDestroyManager(ManagerTable* mgrt, AudioManager* mgr, bool force);

int findManagerType(ManagerTable* mgrt, const char type[3]);



ManagerTable* initializeManagerTable(int size) {
    ManagerTable* mgrt = malloc(sizeof(ManagerTable) + (sizeof(ManagerTableUnit)*size));
    if ( mgrt == NULL ) {
        printf("[initializeManagerTable]\x1b[31m(NewManagerTableFailed)\x1b[0m 새 매니저 테이블 생성 도중 문제가 발생했습니다.\n");

        return NULL;
    }

    mgrt->max   = size
   ;mgrt->count = 0
   ;
    printf("[initializeManagerTable]\x1b[32m(SUCCESS)\x1b[0m 매니저 테이블이 성공적으로 생성되었습니다.\n");

    return mgrt;
}

void destroyManagerTable(ManagerTable* mgrt, bool force) {
    printf("[destroyManagerTable] 매니저 테이블 파괴 중...\n");
    printf("[destroyManagerTable] 현재 테이블에 포함된 매니저 수 : \x1b[33m%d\x1b[0m\n", mgrt->count);

    int count = mgrt->count;
    for ( int i=0; i<count; i++ ) {
        printf("[destroyManagerTable] \x1b[33m%d\x1b[0m번째 매니저 파괴 중...\n", i);
        unregisterAndDestroyManager(mgrt, mgrt->table[i].mgr, force);
        printf("완료\n");
        printf("[destroyManagerTable] 남은 매니저 수 : \x1b[33m%d\x1b[0m\n", mgrt->count);
    }

    free(mgrt);
    printf("[destroyManagerTable]\x1b[32m(SUCCESS)\x1b[0m 매니저 테이블이 파괴되었습니다.\n");
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

void unregisterAndDestroyManager(ManagerTable* mgrt, AudioManager* mgr, bool force) {
    unregisterManager(mgrt, mgr->type);
    destroyManager(mgr, force);
}

int findManagerType(ManagerTable* mgrt, const char type[3]) {
    for ( int i=0; i<mgrt->count; i++ ) {
        if ( memcmp(mgrt->table[i].mgr->type, type, 3) == 0 ) {
            return i;
        }
    }

    return -1;
}