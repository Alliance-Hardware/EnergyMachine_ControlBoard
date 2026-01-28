#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "main.h"
#include "task.h"
#include "projdefs.h"
#include "HUB75Task.h"
#include "timer.h"
extern osThreadId_t HUB75TaskHandle;
extern osThreadId_t M3508TaskHandle;
// 定时器中断回调函数
void TIM_Callback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM3) {
		// 更新时间（1ms触发）
		// 创建优先级判断信号量
		BaseType_t xHigherPriorityTaskWoken_M3508 = pdFALSE;
		BaseType_t xHigherPriorityTaskWoken_HUB75 = pdFALSE;
		// M3508定时器任务通知
		vTaskNotifyGiveFromISR((TaskHandle_t)M3508TaskHandle, &xHigherPriorityTaskWoken_M3508);
		// HUB75定时器任务通知
		xTaskNotifyFromISR((TaskHandle_t)HUB75TaskHandle, TIMER_CALLBACK,
			eSetBits, &xHigherPriorityTaskWoken_HUB75);
		//判断是否抢占
		if (xHigherPriorityTaskWoken_M3508 == pdTRUE ||
			xHigherPriorityTaskWoken_HUB75 == pdTRUE){
			portYIELD_FROM_ISR(pdTRUE);
		}else{
			portYIELD_FROM_ISR(pdFALSE);
		}
	}
}