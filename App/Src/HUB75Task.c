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
		xTaskNotifyFromISR(HUB75TaskHandle,  CAN_CALLBACK, eSetValueWithOverwrite, &xHigherPriorityTaskWoken_HUB75);
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
		xTaskNotifyWait(0, 0, &pulNotificationValue, portMAX_DELAY);
		switch (pulNotificationValue) {
			case TIMER_CALLBACK:
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
				break;
			case TIME_SUCCESS_TO_IDLE:
				break;
			default: ;
		}
		if (energy_machine->timer_20s >= TIME_20S_MS){

		}
		else if (energy_machine->timer_2_5s >= TIME_2_5S_MS){

		}
		else if (energy_machine->timer_1s >= TIME_1S_MS) {

		}
		else if (energy_machine->timer_SuccessToIdle >= TIME_SUCCESS_TO_IDLE) {

		}
	}
}
