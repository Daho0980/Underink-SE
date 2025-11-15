#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audioStruct.h"


ManagerTable* initialiizeManagerTable(int size);
void destroyManagerTable(ManagerTable* mgrt, bool force);


ManagerTable* initialiizeManagerTable(int size) {
    ManagerTable* mgrt = malloc(sizeof(ManagerTable) + (sizeof(ManagerTableUnit)*size));
    if ( mgrt = NULL ) {
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
        unregisterAndDestroyManager(mgrt, mgrt->table[i].mgr, force); // 현재 없음
        printf("완료\n");
        printf("[destroyManagerTable] 남은 매니저 수 : \x1b[33m%d\x1b[0m\n", mgrt->count);
    }

    free(mgrt);
    printf("[destroyManagerTable]\x1b[32m(SUCCESS)\x1b[0m 매니저 테이블이 파괴되었습니다.\n");
}
