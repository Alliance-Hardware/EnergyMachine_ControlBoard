#include "FreeRTOS.h"
#include <string.h>
#include "HUB75Task.h"
#include <stdlib.h>
#include "cmsis_os2.h"
#include "queue.h"
#include "stm32f1xx_hal_can.h"
#include "timer.h"
EnergyMachine_t em = {0};
EnergyMachine_t *energy_machine = &em;
extern osThreadId_t HUB75TaskHandle;
extern osThreadId_t ErrorHandlerTaskHandle;
extern osMessageQueueId_t CANToHUBQueueHandle;
void EnergyMachine_Init(EnergyMachine_t *machine) {
	if (machine == NULL) {
		return;  // 防御性编程，防止空指针
	}
	machine->state = EM_STATE_INACTIVE;
	machine->activated_count = 0;
	machine->ring_sum = 0;
	memset(machine->current_leaf_ids, 0, sizeof(machine->current_leaf_ids));
	memset(machine->ring, 0, sizeof(machine->ring));
	machine->timer_1s = 0;
	machine->timer_2_5s = 0;
	machine->timer_20s = 0;
}

void HUB75_CAN_RxCallback(uint16_t std_id, uint8_t *data)
{
	CANCallBack* can_message = pvPortMalloc(sizeof(CANCallBack));
	// 拷贝数据
	can_message->id = std_id;
	memcpy(can_message->data, data, 8);
	// 队列发送与任务通知
	BaseType_t xHigherPriorityTaskWoken_HUB75 = pdFALSE;
	if (xQueueSendFromISR(CANToHUBQueueHandle, &can_message, &xHigherPriorityTaskWoken_HUB75) == pdPASS) {
		xTaskNotifyFromISR(HUB75TaskHandle, CAN_CALLBACK, eSetBits, &xHigherPriorityTaskWoken_HUB75);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken_HUB75);
	}
	else {
		BaseType_t xHigherPriorityTaskWoken_ERROR_HANDLER = pdFALSE;
		vTaskNotifyGiveFromISR(ErrorHandlerTaskHandle,&xHigherPriorityTaskWoken_ERROR_HANDLER);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken_ERROR_HANDLER);
	}
}

void StartHUB75Task(void *argument)
{
	for (;;) {
		uint32_t pulNotificationValue = 0;
		// 等待任务通知
		xTaskNotifyWait(0, TIMER_CALLBACK | CAN_CALLBACK, &pulNotificationValue, portMAX_DELAY);
		// 检查是否触发定时器中断
		if ((pulNotificationValue & TIMER_CALLBACK) != 0){
			// 判断当前状态,决定定时器自增情况
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
				break;
			case EM_STATE_SMALL_SUCCESS:      // 小符激活成功
			case EM_STATE_BIG_SUCCESS:		 // 大符激活成功
				energy_machine->timer_SuccessToIdle++;
				break;
			default:
				break;
			}
		}
		// 检查定时器超时问题
		if (energy_machine->timer_20s >= TIME_20S_MS){

		}
		else if (energy_machine->timer_2_5s >= TIME_2_5S_MS){

		}
		else if (energy_machine->timer_1s >= TIME_1S_MS) {

		}
		else if (energy_machine->timer_SuccessToIdle >= TIME_SUCCESS_TO_IDLE) {

		}
		// 检查是否触发CAN中断
		if ((pulNotificationValue & CAN_CALLBACK) != 0){
			CANCallBack* can_message;
			while (xQueueReceive(CANToHUBQueueHandle, &can_message, 0) == pdPASS) {

				vPortFree(can_message);
			}
		}
	}
}
