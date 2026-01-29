#include "FreeRTOS.h"
#include <string.h>
#include "HUB75Task.h"
#include <stdlib.h>
#include <sys/types.h>
#include "bsp_can.h"
#include "cmsis_os2.h"
#include "Config.h"
#include "queue.h"
#include "stm32f1xx_hal_can.h"
#include "timer.h"
#include "RandomData.h"
EnergyMachine_t em = {0};
EnergyMachine_t *energy_machine = &em;
extern osThreadId_t HUB75TaskHandle;
extern osThreadId_t ErrorHandlerTaskHandle;
extern osMessageQueueId_t CANToHUBQueueHandle;
void EnergyMachine_Init(EnergyMachine_t *machine) {
	if (machine == NULL) {
		return;  // 防御性编程，防止空指针
	}
	uint8_t init_ids[5] = {3, 4, 5, 6, 7};
	machine->state = EM_STATE_INACTIVE;
	machine->activated_count = 0;
	machine->ring_sum = 0;
	memset(machine->result_leaf_ids, 0, sizeof(machine->result_leaf_ids));
	memset(machine->selected_leaf_ids, 0, sizeof(machine->selected_leaf_ids));
	memcpy(machine->unselected_leaf_ids, init_ids, sizeof(init_ids));
	memset(machine->ring, 0, sizeof(machine->ring));
	machine->timer_1s = 0;
	machine->timer_2_5s = 0;
	machine->timer_20s = 0;
}

void HUB75_CAN_RxCallback(uint16_t std_id, uint8_t *data)
{
	CANMessage* can_message = pvPortMalloc(sizeof(CANMessage));
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

uint8_t IsRightTarget(CANMessage* can_message) {
	if (can_message->id - CAN_RECEIVE_BASE_ID ==
								energy_machine->selected_leaf_ids[can_message->id - CAN_RECEIVE_BASE_ID -3]) {
		energy_machine->selected_leaf_ids[can_message->id - CAN_RECEIVE_BASE_ID -3] = 0;
		return 1;
	}
	return 0;
}

void ResetToIdle() {

}

void ResetToInactive() {
	EnergyMachine_Init(energy_machine);
}

void Init_CANSend() {
	uint16_t id = CAN_ID;
	uint8_t tx_data[8] = {0};
	tx_data[0] = DISABLE;
	BSP_CAN_SendMsg(&hcan1, id, tx_data);
}

uint8_t Small_EM_CANSend() {
	uint16_t id = CAN_ID;
	uint8_t tx_data[8] = {0};
	if (GetRandomData(energy_machine->unselected_leaf_ids, energy_machine->result_leaf_ids, 1) == 1) {
		energy_machine->selected_leaf_ids[energy_machine->result_leaf_ids[0] - 3] = energy_machine->result_leaf_ids[0];
		energy_machine->unselected_leaf_ids[energy_machine->result_leaf_ids[0] - 3] = 0;
		tx_data[0] = SMALL_EM;
		tx_data[energy_machine->result_leaf_ids[0]] = 0xFF;
		BSP_CAN_SendMsg(&hcan1, id, tx_data);
		return 1;
	}
	for (uint8_t i = 0; i < 5; i++) {
		if (energy_machine->unselected_leaf_ids[i] != 0) {
			return 0xFF;
		}
	}
	return 0;
}

uint8_t Big_EM_CANSend() {
	uint16_t id = CAN_ID;
	uint8_t tx_data[8] = {0};
	if (GetRandomData(energy_machine->unselected_leaf_ids, energy_machine->result_leaf_ids, 2) == 2) {
		energy_machine->selected_leaf_ids[energy_machine->result_leaf_ids[0] - 3] = energy_machine->result_leaf_ids[0];
		energy_machine->selected_leaf_ids[energy_machine->result_leaf_ids[1] - 3] = energy_machine->result_leaf_ids[1];
		tx_data[0] = BIG_EM;
		tx_data[energy_machine->result_leaf_ids[0]] = 0xFF;
		tx_data[energy_machine->result_leaf_ids[1]] = 0xFF;
		BSP_CAN_SendMsg(&hcan1, id, tx_data);
		return 1;
	}
	return 0;
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
				if (energy_machine->timer_InactiveToStart != 0) {
					energy_machine->timer_InactiveToStart++;
				}
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
			// 检查定时器超时问题
			if (energy_machine->timer_20s >= TIME_20S_MS){
				energy_machine->timer_20s = 0;
			}
			else if (energy_machine->timer_2_5s >= TIME_2_5S_MS){
				energy_machine->timer_2_5s = 0;
			}
			else if (energy_machine->timer_1s >= TIME_1S_MS) {
				energy_machine->timer_1s = 0;
				energy_machine->state = EM_STATE_BIG_ACTIVATING_25;
				if (Big_EM_CANSend() != 1) {
					xTaskNotifyGive(ErrorHandlerTaskHandle);
				}
			}
			else if (energy_machine->timer_InactiveToStart >=  TIME_INACTIVE_TO_START) {
				energy_machine->timer_InactiveToStart = 0;
				energy_machine->state = EM_STATE_SMALL_IDLE;
				if (Small_EM_CANSend() != 1) {
					xTaskNotifyGive(ErrorHandlerTaskHandle);
				}
			}
			else if (energy_machine->timer_SuccessToIdle >= TIME_SUCCESS_TO_IDLE) {
				energy_machine->timer_SuccessToIdle = 0;
			}
		}
		// 检查是否触发CAN中断
		if ((pulNotificationValue & CAN_CALLBACK) != 0){
			CANMessage* can_message;
			while (xQueueReceive(CANToHUBQueueHandle, &can_message, 0) == pdPASS) {
				if (can_message->id >= 0x403 && can_message->id <= 0x407) {
					switch (energy_machine->state)
					{
						case EM_STATE_INACTIVE:// 未激活状态
							if (energy_machine->timer_InactiveToStart == 0) {
								energy_machine->timer_InactiveToStart = 1;
							}
							if (energy_machine->timer_InactiveToStart >= 1000) {
								energy_machine->timer_InactiveToStart = 0;
								energy_machine->state = EM_STATE_BIG_ACTIVATING_25;
								if (Big_EM_CANSend() != 1) {
									xTaskNotifyGive(ErrorHandlerTaskHandle);
								}
							}
							break;
						case EM_STATE_SMALL_IDLE:         // 小符待机
						case EM_STATE_SMALL_ACTIVATING:   // 小符正在激活
							if (IsRightTarget(can_message)) {
								energy_machine->timer_2_5s = 0;
								energy_machine->state = EM_STATE_SMALL_ACTIVATING;
								if (Small_EM_CANSend() == 0) {
									energy_machine->state = EM_STATE_SMALL_SUCCESS;
								}
								if (Small_EM_CANSend() == 0xFF) {
									xTaskNotifyGive(ErrorHandlerTaskHandle);
								}
							}
							else {
								ResetToIdle();
							}
							break;
						case EM_STATE_BIG_IDLE:           // 大符待机
						case EM_STATE_BIG_ACTIVATING_25:  // 大符正在激活 (2.5s阶段)
							if (IsRightTarget(can_message)) {
								energy_machine->timer_2_5s = 0;
								energy_machine->state = EM_STATE_BIG_ACTIVATING_1;
							}
							else {
								ResetToIdle();
							}
							break;
						case EM_STATE_BIG_ACTIVATING_1:   // 大符正在激活 (1s阶段)
							if (IsRightTarget(can_message)) {
								energy_machine->timer_1s = 0;
								energy_machine->state = EM_STATE_BIG_ACTIVATING_25;
								if (Big_EM_CANSend() != 1) {
									xTaskNotifyGive(ErrorHandlerTaskHandle);
								}
							}
							else {
								energy_machine->timer_1s = 0;
								energy_machine->state = EM_STATE_BIG_ACTIVATING_25;
								if (Big_EM_CANSend() != 1) {
									xTaskNotifyGive(ErrorHandlerTaskHandle);
								}
							}
							break;
						case EM_STATE_SMALL_SUCCESS:      // 小符激活成功
						case EM_STATE_BIG_SUCCESS:		 // 大符激活成功
						default:
							break;
					}
				}
				vPortFree(can_message);
			}
		}
	}
}
