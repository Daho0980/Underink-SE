#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <stdbool.h>

#include "stdcmd.h"
#include "audioStruct.h"


/**
 * @brief   continue 명령을 생성합니다.
 */
uint8_t cmd_setVolume();

/**
 * @brief   pause 명령을 생성합니다.
 */
uint8_t cmd_continue();

/**
 * @brief   stop 명령을 생성합니다.
 */
uint8_t cmd_pause();

/**
 * @brief   setVolume 명령을 생성합니다.
 */
uint8_t cmd_stop();

/**
 * @brief   fade 명령을 생성합니다.
 * 
 * @param target   목표 볼륨 (0~4095, 12비트, 4095 = 100%)
 * @param duration 페이드 지속 시간 (밀리초 단위)
 * 
 * @return uint32_t 32비트 명령값
 *  \n
 * 명령 구조:
 * ```
 * [  opcode(4b)  ][  target(12b)  ][  duration(16b)  ]
 * ```
 */
uint8_t cmd_fade(uint16_t target, uint16_t duration);

bool isCommandQueueEmpty_u8(CommandQueue* queue);
bool isCommandQueueEmpty_u32(CommandQueue* queue);
bool isCommandQueueFull_u8(CommandQueue* queue);
bool isCommandQueueFull_u32(CommandQueue* queue);

int commandPush(AudioChannelData* channel, ThreadQueueCommand cmd);
uint32_t commandPop(CommandQueue* queue);

#endif