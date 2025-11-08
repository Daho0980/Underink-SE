#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <stdbool.h>

#include "audioThreadStruct.h"


uint8_t cmd_setVolume();
uint8_t cmd_continue();
uint8_t cmd_pause();
uint8_t cmd_stop();
uint8_t cmd_fadeOut(uint8_t level);

bool isCommandQueueEmpty_u8(CommandQueue* queue);
bool isCommandQueueEmpty_u32(CommandQueue* queue);
bool isCommandQueueFull_u8(CommandQueue* queue);
bool isCommandQueueFull_u32(CommandQueue* queue);

int commandPush(CommandQueue* queue, uint8_t cmd);
uint32_t commandPop(CommandQueue* queue);

#endif