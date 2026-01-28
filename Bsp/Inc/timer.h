#ifndef STM32F105_FR_TIMER_H
#define TIME_1S_MS 1000				//1s
#define TIME_2_5S_MS 2500			//2.5s
#define TIME_20S_MS 20000			//20s
#define TIME_SUCCESS_TO_IDLE 10000	//10s
#define TIMER_CALLBACK (1 < 0)
#include <stdint.h>
#include "stm32f1xx_hal.h"
uint32_t get_time();
void TIM_Callback(TIM_HandleTypeDef *htim);
#define STM32F105_FR_TIMER_H
#endif