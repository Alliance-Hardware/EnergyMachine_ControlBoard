#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "cmsis_os2.h"
#include "HUB75Task.h"
#include "timer.h"
extern osThreadId_t HUB75TaskHandle;
extern osThreadId_t M3508TaskHandle;
static volatile uint32_t time_1ms = 0;
uint32_t get_time()
{
	return time_1ms;
}
// 定时器中断回调函数
void TIM_Callback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM3) {
		// 更新时间（1ms触发）
		time_1ms++;
		switch (energy_machine->state)
		{
			case EM_STATE_INACTIVE:// 未激活状态
				break;
			case EM_STATE_SMALL_IDLE:         // 小符待机
			case EM_STATE_BIG_IDLE:           // 大符待机
			case EM_STATE_BIG_ACTIVATING_25:  // 大符正在激活 (2.5s阶段)
			case EM_STATE_SMALL_ACTIVATING:   // 小符正在激活
				energy_machine->timer_2_5s++;
				energy_machine->timer_20s++;
				break;
			case EM_STATE_BIG_ACTIVATING_1:   // 大符正在激活 (1s阶段)
				energy_machine->timer_1s++;
				energy_machine->timer_20s++;
			case EM_STATE_SMALL_SUCCESS:      // 小符激活成功
			case EM_STATE_BIG_SUCCESS:		 // 大符激活成功
				energy_machine->timer_SuccessToIdle++;
				break;
			default:
				break;
		}
		// 创建优先级判断量
		BaseType_t xHigherPriorityTaskWoken_M3508 = pdFALSE;
		BaseType_t xHigherPriorityTaskWoken_HUB75 = pdFALSE;
		vTaskNotifyGiveFromISR((TaskHandle_t)M3508TaskHandle, &xHigherPriorityTaskWoken_M3508);
		if (energy_machine->timer_20s >= TIME_20S_MS || energy_machine->timer_2_5s >= TIME_2_5S_MS ||
			energy_machine->timer_1s >= TIME_1S_MS || energy_machine->timer_SuccessToIdle >= TIME_SUCCESS_TO_IDLE)
		{
			vTaskNotifyGiveFromISR((TaskHandle_t)HUB75TaskHandle, &xHigherPriorityTaskWoken_HUB75);
		}
		//判断是否抢占
		if (xHigherPriorityTaskWoken_M3508 == pdTRUE || xHigherPriorityTaskWoken_HUB75 == pdTRUE)
		{
			portYIELD_FROM_ISR(pdTRUE);
		}
		else
		{
			portYIELD_FROM_ISR(pdFALSE);
		}
	}
	if (htim->Instance == TIM4) {
		// 更新时间（10ms触发）
	}
}