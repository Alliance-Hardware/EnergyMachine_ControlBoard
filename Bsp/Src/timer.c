#include "timer.h"
#include "HUB75Task.h"
#include "stm32f1xx_hal_tim.h"

static volatile uint32_t time_1ms = 0;

uint32_t get_time()
{
	return time_1ms;
}

// 定时器中断回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM3) {
		// 更新时间（1ms触发）
		time_1ms++;
	}
	if (htim->Instance == TIM4) {
		// 更新时间（10ms触发）
	}
}