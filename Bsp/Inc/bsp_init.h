//
// Created by MECHREUO on 2026/1/25.
//

#ifndef STM32F105_FR_BSP_INIT_H
#define STM32F105_FR_BSP_INIT_H
#include "m3508_ctrl.h"
extern M3508_Handle_t motor1;
void can_init(void);
void ws2812_init(void);
void m3508_init(void);
void timer_init(void);
#endif //STM32F105_FR_BSP_INIT_H