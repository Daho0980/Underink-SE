#include <stdio.h>
#include <process.h>

#include <windows.h>

#include "audioTool/management/resource.h"


int testRunner(int (*test)(), char* name) {
    printf("\n[ -- %s 시작 -- ]\n\n", name);

    if ( test() ) {
        printf("[testRunner]|[\x1b[33m%s\x1b[0m]\x1b[31m(TestFailed)\x1b[0m 테스트가 비정상 종료되었습니다.\n", name);

        return 1;
    }
    printf("\n[testRunner]|[\x1b[33m%s\x1b[0m]\x1b[32m(SUCCESS)\x1b[0m 테스트가 정상 종료되었습니다.\n", name);

    printf("\n[ -- %s 종료 -- ]\n\n", name);

    return 0;
}

unsigned __stdcall threadFunc(void* pParam) {
    int* pThreadNum = (int*)pParam;
    int  threadNum  = *pThreadNum ;

    printf("[threadFunc]|[Thread \x1b[33m%d\x1b[0m] 스레드 시작...\n", threadNum);

    Sleep(500);
    printf("[threadFunc]|[Thread \x1b[33m%d\x1b[0m] 작업 완료 및 종료\n", threadNum);

    _endthreadex(0);

    return 0;
}

int test_Thread() {
    HANDLE   hThread ;
    unsigned threadID;

    int myThreadArg = 1;

    printf("[MainThread] \x1b[33m'threadFunc'\x1b[0m 스레드를 생성합니다.\n");
    hThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        threadFunc,
        &myThreadArg,
        0,
        &threadID
    );

    if ( hThread == NULL ) {
        printf("[MainThread]\x1b[31m(ThreadGenerationFailed)\x1b[0m 스레드 생성에 실패헀습니다.\n");

        return 1;
    }

    printf("[MainThread]\x1b[32m(SUCCESS)\x1b[0m threadFunc(ID : \x1b[33m%d\x1b[0m)가 생성되었습니다.\n", threadID);
    printf("[MainThread] 작업 완료 대기 중...\n");

    WaitForSingleObject(hThread, INFINITE);
    printf("완료.\n");

    CloseHandle(hThread);

    return 0;
}

int test_GetWave() {
    if ( getSound("assets/sine24.wav") == NULL ) {
        return 1;
    }

    return 0;
}

int main() {
    printf("\n[ -- main 시작 -- ]\n\n");
    testRunner(test_Thread, "Thread");
    testRunner(test_GetWave, "GetWave");
    printf("\n[ -- main 종료 -- ]\n");

    return 0;
}