#include <stdio.h>
#include <unistd.h>

#include "platform.h"
#include "audioSystemArgs.h"

#include "stdcmd.h"
#include "audioTool/management/queue.h"
#include "audioTool/management/resource.h"

#include "audioSystem/audioManager.h"
#include "audioSystem/audioManagerTable.h"
#include "commandQueue.h"


int main() {
    printf("\n[ -- main 시작 -- ]\n\n");
    ManagerTable* table  = initializeManagerTable(4);
    AudioManager* bgmmgr = setBGMManager(table);
    AudioManager* dupmgr = setBGMManager(table);
    unregisterAndDestroyManager(table, bgmmgr, true);

    AudioManager* sfx = initializeManager(table, "SFX", "ALLINONE", 2);
    AudioManager* mgr = initializeManager(table, "TST", "SAMPLING", 1);

    enqueueRequest(
        sfx,
        getSound("assets/test1.wav"),
        "smash", NULL
    );
    enqueueRequest(
        sfx,
        getSound("assets/test.wav"),
        "blinkin'", NULL
    );
    enqueueRequest(
        sfx,
        getSound("assets/test2.wav"),
        "nevermind.",
        "nevermind.",
        "nevermind.",
        "nevermind.",
        "nevermind.",
        "nevermind.",
        "nevermind.",
        "nevermind.",
        "nevermind."
    );
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
    mgr->volume = 100.0;
    commandPush(&mgr->threads[0], pkcmd_u8(cmd_setVolume()));
    printf("볼륨이 100%%로 재설정됨\n");
    commandPush(&mgr->threads[0], pkcmd_u32(cmd_fade(0, 9000)));
    printf("페이드 적용됨\n");

    destroyManagerTable(table, false);
    printf("\n[ -- main 종료 -- ]\n\n");

    return 0;
}