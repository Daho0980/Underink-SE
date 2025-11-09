#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "stdcmd.h"

#include "audioStruct.h"


// ***********************
// Queue Command Functions
// ***********************

/**
 * @brief   continue 명령을 생성합니다.
 */
uint8_t cmd_continue() {
    return CMD_CONTINUE<<4;
}

/**
 * @brief   pause 명령을 생성합니다.
 */
uint8_t cmd_pause() {
    return CMD_PAUSE<<4;
}

/**
 * @brief   stop 명령을 생성합니다.
 */
uint8_t cmd_stop() {
    return CMD_STOP<<4;
}

/**
 * @brief   setVolume 명령을 생성합니다.
 */
uint8_t cmd_setVolume() {
    return CMD_SET_VOLUME<<4;
}

/**
 * @brief   fade 명령을 생성합니다.
 * 
 * @param target   목표 볼륨 (0~4095, 12비트, 4095 = 100%)
 * @param duration 페이드 지속 시간 (밀리초 단위)
 * @return uint32_t 32비트 명령값
 * ```
 * [  opcode(4b)  ][  target(12b)  ][  duration(16b)  ]
 * ```
 */
uint32_t cmd_fade(uint16_t target, uint16_t duration) {
    if ( target > 0xFFF ) {
        fprintf(stderr,
            "[cmd_fade]\x1b[31m(TargetOverflow)\x1b[0m목표치는 4095 레벨을 넘을 수 없습니다. 현재 : %u\n",
            target
        );

        return ((CMD_ERROR<<4)|cmd_targetOverflowErr)<<24;
    }

    if ( !duration ) {
        fprintf(stderr,
            "[cmd_fade]\x1b[31m(ZeroDuration)\x1b[0m지속 시간은 0밀리초일 수 없습니다.\n"
        );

        return ((CMD_ERROR<<4)|cmd_zeroDurationErr)<<24;
    }

    return (
        (CMD_FADE<<28)         |
        ((uint32_t)target<<12) |
        duration
    );
}

// **************************
// Queue Management Functions
// **************************

bool isCommandQueueEmpty_u8(CommandQueue* queue) {
    return queue->head==queue->tail;
}

bool isCommandQueueEmpty_u32(CommandQueue* queue) {
    return !((queue->mixTotal>>60)&0xF);
}

bool isCommandQueueFull_u8(CommandQueue* queue) {
    return ((queue->tail+1)%MAX_COMMAND_QUEUE)==queue->head;
}

bool isCommandQueueFull_u32(CommandQueue* queue) {
    return (queue->mixTotal>>60)&0xF;
}

int commandPush(CommandQueue* queue, ThreadQueueCommand cmd) {
    pthread_mutex_lock(&queue->mutex);

    if ( cmd.type == CMDB1 ) {
        if ( isCommandQueueFull_u8(queue) ) goto failed;

        queue->mixTotal &= ~(((1<<8)-1)<<(8*queue->tail));
        queue->mixTotal |= (cmd.command.b1<<(8*queue->tail));
        queue->tail = (queue->tail+1) % MAX_COMMAND_QUEUE;
    }
    else if ( cmd.type == CMDB4 ) {
        if ( isCommandQueueFull_u32(queue) ) goto failed;

        queue->mixTotal |= ((uint64_t)cmd.command.b4<<32);
    }

    pthread_mutex_unlock(&queue->mutex);

    return 0;

    failed:
        fprintf(stderr,
            "[commandPush]\x1b[31m(CommandQueueFull)\x1b[0m 큐가 가득 찼습니다.\n"
        );
        pthread_mutex_unlock(&queue->mutex);

        return 1;
}

uint32_t _commandPop_u8(CommandQueue* queue) {
    if ( isCommandQueueEmpty_u8(queue) ) {
        fprintf(stderr,
            "[_commandPop_u8]\x1b[31m(CommandQueueEmpty)\x1b[0m 큐가 비어 있습니다.\n"
        );

        return ((CMD_ERROR<<4)|cmd_u8QueueEmptyErr) << 24;
    }

    uint32_t cmd = ((queue->mixTotal>>(8*queue->tail))&0xFF)<<24;
    queue->head  = (queue->head+1) % MAX_COMMAND_QUEUE;

    return cmd;
}

uint32_t _commandPop_u32(CommandQueue* queue) {
    if ( isCommandQueueEmpty_u32(queue) ) {
        fprintf(stderr,
            "[_commandPop_u32]\x1b[31m(u32SlotEmpty)\x1b[0m 슬롯이 비어 있습니다.\n"
        );

        return ((CMD_ERROR<<4)|cmd_u32SlotEmptyErr) << 24;
    }

    uint32_t cmd = (uint32_t)(queue->mixTotal>>32);
    queue->mixTotal &= 0xFFFFFFFFULL;

    return cmd;
}

uint32_t commandPop(CommandQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    uint32_t out;
    out = _commandPop_u32(queue);
    if ( (out>>28) != CMD_ERROR ) goto success;

    out = _commandPop_u8(queue);
    if ( (out>>28) != CMD_ERROR ) goto success;

    pthread_mutex_unlock(&queue->mutex);

    fprintf(stderr,
        "[commandPop]\x1b[31m(CommandNotFound)\x1b[0m 명령을 찾지 못했습니다.\n"
    );

    return ((CMD_ERROR<<4)|cmd_commandNotFoundErr)<<24;

    success:
        pthread_mutex_unlock(&queue->mutex);

        return out;
}