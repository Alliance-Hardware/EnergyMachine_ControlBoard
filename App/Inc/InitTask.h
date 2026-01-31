#ifndef STM32F105_FR_INITTASK_H
#define STM32F105_FR_INITTASK_H
#include "CyclicBuffer.h"
CYCLICBUFFER_DECLARE(can_message_buffer, CANMessage_t, 32);
CYCLICBUFFER_EXTERN_DECLARE(can_message_buffer);
void StartInitTask(void *argument);
#endif //STM32F105_FR_INITTASK_H