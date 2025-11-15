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
}