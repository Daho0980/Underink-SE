#include <stdint.h>

#include "stdcmd.h"


ThreadQueueCommand pkcmd_u8(uint8_t cmd) {
    return (ThreadQueueCommand){ .type=CMDB1, .command.b1=cmd };
}

ThreadQueueCommand pkcmd_u32(uint32_t cmd) {
    return (ThreadQueueCommand){ .type=CMDB4, .command.b4=cmd };
}