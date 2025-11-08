#ifndef STDCMD_H
#define STDCMD_H


#define CMD_CONTINUE   0b0001
#define CMD_PAUSE      0b0010
#define CMD_STOP       0b0011
#define CMD_SET_VOLUME 0b0100

#define CMD_FADE 0b0101

#define CMD_ERROR 0b1111

typedef enum {
    cmd_undefinedErr,
    cmd_targetOverflowErr,
    cmd_zeroDurationErr,
    cmd_u8QueueEmptyErr,
    cmd_u32SlotEmptyErr,
    cmd_commandNotFoundErr
} CmdErrorType;

typedef enum { CMDB1, CMDB4 } TQCMDUT;

typedef struct {
    TQCMDUT type;
    union {
       uint8_t b1;
       uint64_t b4;
    } command;
} ThreadQueueCommand;

ThreadQueueCommand cmd_u8(uint8_t cmd) {
    return (ThreadQueueCommand){ .type=CMDB1, .command.b1=cmd };
}

ThreadQueueCommand cmd_u32(uint32_t cmd) {
    return (ThreadQueueCommand){ .type=CMDB4, .command.b4=cmd };
}

#endif