#include "FreeRTOS.h"
#include <string.h>
#include "HUB75Task.h"
#include <stdlib.h>
#include <sys/types.h>
#include "bsp_can.h"
#include "cmsis_os2.h"
#include "Config.h"
#include "queue.h"
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
	machine->counter = 0;
	machine->counter_success = 0;
	machine->ring_sum = 0;
	machine->timer_1s = 0;
	machine->timer_2_5s = 0;
	machine->timer_20s = 0;
	machine->timer_InactiveToStart = 0;
	machine->timer_SuccessToIdle = 0;
	memset(machine->result_leaf_ids, 0, sizeof(machine->result_leaf_ids));
	memset(machine->selected_leaf_ids, 0, sizeof(machine->selected_leaf_ids));
	memcpy(machine->unselected_leaf_ids, init_ids, sizeof(init_ids));
	memset(machine->ring, 0, sizeof(machine->ring));
}

void HUB75_CAN_RxCallback(uint16_t std_id, uint8_t *data)
{
	CANMessage* can_message = pvPortMalloc(sizeof(CANMessage));
	if (can_message == NULL) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		vTaskNotifyGiveFromISR(ErrorHandlerTaskHandle, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		return;
	}
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
	uint8_t index = can_message->id - CAN_RECEIVE_BASE_ID;
	// 防御性检查
	if (index >= 3 && index <= 7) {
		if (index == energy_machine->selected_leaf_ids[index -3]) {
			energy_machine->selected_leaf_ids[index -3] = 0;
			return 1;
		}
	}
	return 0;
}



void GetGainTime(uint16_t *gain_time) {
	// 大能量机关增益时间
	if (energy_machine->state == EM_STATE_BIG_SUCCESS) {
		*gain_time = energy_machine->counter_success * 1000;
	}
	// 小能量机关增益时间
	else if (energy_machine->state == EM_STATE_SMALL_SUCCESS) {
		*gain_time = 5000;
	}
}

void Init_CANSend() {
	uint16_t id = CAN_ID;
	uint8_t tx_data[8] = {0};
	tx_data[0] = DISABLE;
	BSP_CAN_SendMsg(&hcan1, id, tx_data);
}

uint8_t Small_EM_CANSend() {
	uint8_t tx_data[8] = {0};
	tx_data[0] = SMALL_EM;
	if (GetRandomData(energy_machine->unselected_leaf_ids, energy_machine->result_leaf_ids, 1) == 1) {
		uint16_t id = CAN_ID;
		energy_machine->selected_leaf_ids[energy_machine->result_leaf_ids[0] - 3] = energy_machine->result_leaf_ids[0];
		energy_machine->unselected_leaf_ids[energy_machine->result_leaf_ids[0] - 3] = 0;
		energy_machine->counter++;
		tx_data[energy_machine->result_leaf_ids[0]] = 0xFF;
		BSP_CAN_SendMsg(&hcan1, id, tx_data);
		return 1;
	}
	return 0;
}

uint8_t Big_EM_CANSend() {
	uint8_t tx_data[8] = {0};
	uint16_t id = CAN_ID;
	tx_data[0] = BIG_EM;
	// 这里不是显示平均环数,而是显示对应增益的平均环数区间的向上取值(也就是说会比打出的平均环数更大)
	if (energy_machine->state == EM_STATE_BIG_SUCCESS) {
		float average_ring = (float)energy_machine->ring_sum / (float)energy_machine->counter_success;
		// 定义(攻击增益, 防御增益, 热量冷却增益)
		if (average_ring >= 1 && average_ring <= 3) {
			// (150%, 25%, 0)
			memset(&tx_data[3], Ring_3, 5);
		}
		else if (average_ring > 3 && average_ring <= 7) {
			// (150%, 25%, 200%)
			memset(&tx_data[3], Ring_7, 5);
		}
		else if (average_ring > 7 && average_ring <= 8) {
			// (200%, 25%, 200%)
			memset(&tx_data[3], Ring_8, 5);
		}
		else if (average_ring > 8 && average_ring <= 9) {
			// (200%, 25%, 300%)
			memset(&tx_data[3], Ring_9, 5);
		}
		else if (average_ring > 9 && average_ring <= 10) {
			// (300%, 50%, 500%)
			memset(&tx_data[3], Ring_10, 5);
		}
		if (tx_data[7] != 0) {
			BSP_CAN_SendMsg(&hcan1, id, tx_data);
		}
		return 1;
	}
	if (GetRandomData(energy_machine->unselected_leaf_ids, energy_machine->result_leaf_ids, 2) == 2) {
		energy_machine->selected_leaf_ids[energy_machine->result_leaf_ids[0] - 3] = energy_machine->result_leaf_ids[0];
		energy_machine->selected_leaf_ids[energy_machine->result_leaf_ids[1] - 3] = energy_machine->result_leaf_ids[1];
		energy_machine->counter = energy_machine->counter + 2;

		tx_data[energy_machine->result_leaf_ids[0]] = 0xFF;
		tx_data[energy_machine->result_leaf_ids[1]] = 0xFF;
		BSP_CAN_SendMsg(&hcan1, id, tx_data);
		return 1;
	}
	return 0;
}

void ResetToIdle(EnergyMachine_t *machine) {
	switch (machine->state){
		case EM_STATE_SMALL_IDLE:         // 小符待机
		case EM_STATE_SMALL_ACTIVATING:   // 小符正在激活
		case EM_STATE_BIG_IDLE:           // 大符待机
		case EM_STATE_BIG_ACTIVATING_25:  // 大符正在激活 (2.5s阶段)
			break;
		default:
			return;
	}
	uint8_t init_ids[5] = {3, 4, 5, 6, 7};
	machine->counter = 0;
	machine->counter_success = 0;
	machine->ring_sum = 0;
	machine->timer_1s = 0;
	machine->timer_2_5s = 0;
	machine->timer_InactiveToStart = 0;
	machine->timer_SuccessToIdle = 0;
	memset(machine->selected_leaf_ids, 0, sizeof(machine->selected_leaf_ids));
	memcpy(machine->unselected_leaf_ids, init_ids, sizeof(init_ids));
	memset(machine->ring, 0, sizeof(machine->ring));
	switch (machine->state) {
		case EM_STATE_SMALL_IDLE:         // 小符待机
		case EM_STATE_SMALL_ACTIVATING:   // 小符正在激活
			machine->state = EM_STATE_SMALL_IDLE;
			if (Small_EM_CANSend() != 1) {
				xTaskNotifyGive(ErrorHandlerTaskHandle);
			}
			break;
		case EM_STATE_BIG_IDLE:           // 大符待机
		case EM_STATE_BIG_ACTIVATING_25:  // 大符正在激活 (2.5s阶段)
			machine->state = EM_STATE_BIG_IDLE;
			if (Big_EM_CANSend() != 1) {
				xTaskNotifyGive(ErrorHandlerTaskHandle);
			}
			break;
		default:
			break;
	}
}

void ResetToInactive() {
	// 进入临界区
	taskENTER_CRITICAL();
	xTaskNotify(HUB75TaskHandle, RESET_FLAG, eSetBits);
	CANMessage* can_message;
	// 清空队列消息
	while (xQueueReceive(CANToHUBQueueHandle, &can_message, 0) == pdPASS) {
		if (can_message != NULL) {
			vPortFree(can_message);
		}
	}
	// 退出临界区
	taskEXIT_CRITICAL();
}

void StartHUB75Task(void *argument)
{
	for (;;) {
		uint32_t pulNotificationValue = 0;
		// 等待任务通知
		xTaskNotifyWait(0, TIMER_CALLBACK | CAN_CALLBACK | RESET_FLAG,
			&pulNotificationValue, portMAX_DELAY);
		// 检查是否触发复位中的
		if ((pulNotificationValue & RESET_FLAG) != 0) {
			// 丢弃当前处理的所有消息
			EnergyMachine_Init(energy_machine);
			continue;  // 跳过本次循环
		}
		// 检查是否触发定时器中断
		if ((pulNotificationValue & TIMER_CALLBACK) != 0){
			// 判断当前状态,决定定时器自增情况
			switch (energy_machine->state){
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
				if (energy_machine->timer_SuccessToIdle) {
					energy_machine->timer_SuccessToIdle--;
				}
				break;
			default:
				break;
			}
			// 检查定时器超时问题
			if (energy_machine->timer_20s >= TIME_20S_MS){
				energy_machine->timer_20s = 0;
				ResetToInactive();
				continue;
			}
			if (energy_machine->timer_2_5s >= TIME_2_5S_MS){
				energy_machine->timer_2_5s = 0;
				ResetToIdle(energy_machine);
			}
			else if (energy_machine->timer_1s >= TIME_1S_MS) {
				energy_machine->timer_1s = 0;
				if (energy_machine->counter == 10) {
					energy_machine->state = EM_STATE_BIG_SUCCESS;
					GetGainTime(&energy_machine->timer_SuccessToIdle);
					Big_EM_CANSend();
				}
				else {
					energy_machine->state = EM_STATE_BIG_ACTIVATING_25;
					if (Big_EM_CANSend() != 1) {
						xTaskNotifyGive(ErrorHandlerTaskHandle);
					}
				}
			}
			else if (energy_machine->timer_InactiveToStart >=  TIME_INACTIVE_TO_START) {
				if (energy_machine->state == EM_STATE_INACTIVE ) {
					energy_machine->timer_InactiveToStart = 0;
					energy_machine->state = EM_STATE_SMALL_IDLE;
					if (Small_EM_CANSend() != 1) {
						xTaskNotifyGive(ErrorHandlerTaskHandle);
					}
				}
			}
			if (energy_machine->timer_SuccessToIdle == 0) {
				if (energy_machine->state == EM_STATE_SMALL_SUCCESS || energy_machine->state == EM_STATE_BIG_SUCCESS) {
					ResetToInactive();
					continue;
				}
			}
		}
		// 检查是否触发CAN中断
		if ((pulNotificationValue & CAN_CALLBACK) != 0){
			CANMessage* can_message;
			while (xQueueReceive(CANToHUBQueueHandle, &can_message, 0) == pdPASS) {
				if (can_message->id >= 0x403 && can_message->id <= 0x407) {
					switch (energy_machine->state)
					{
						case EM_STATE_INACTIVE:				// 未激活状态
							//初次击打,启动定时器
							if (energy_machine->timer_InactiveToStart == 0) {
								energy_machine->timer_InactiveToStart = 1;
							}
							// 延时1s滤波,若1s后再次击打,进入大能量机关状态.
							if (energy_machine->timer_InactiveToStart >= 1001) {
								energy_machine->timer_InactiveToStart = 0;
								energy_machine->state = EM_STATE_BIG_IDLE;
								if (Big_EM_CANSend() != 1) {
									xTaskNotifyGive(ErrorHandlerTaskHandle);
								}
							}
							break;
						case EM_STATE_SMALL_IDLE:			// 小符待机
						case EM_STATE_SMALL_ACTIVATING:		// 小符正在激活
							// 是否正确击打
							if (IsRightTarget(can_message)) {
								energy_machine->timer_2_5s = 0;
								energy_machine->state = EM_STATE_SMALL_ACTIVATING;
								// 记录环数总和和各个标靶的环数
								energy_machine->ring[energy_machine->counter - 1] = can_message->data[0];
								energy_machine->ring_sum += can_message->data[0];
								// 判断是否满足退出条件
								if (energy_machine->counter == 5) {
									GetGainTime(&energy_machine->timer_SuccessToIdle);
									energy_machine->state = EM_STATE_SMALL_SUCCESS;
								}
								else if (Small_EM_CANSend() == 0) {
									xTaskNotifyGive(ErrorHandlerTaskHandle);
								}
							}
							// 错误击打,回到IDLE状态
							else {
								ResetToIdle(energy_machine);
							}
							break;
						case EM_STATE_BIG_IDLE:           // 大符待机
						case EM_STATE_BIG_ACTIVATING_25:  // 大符正在激活 (2.5s阶段)
							// 是否正确击打
							if (IsRightTarget(can_message)) {
								energy_machine->timer_2_5s = 0;
								energy_machine->state = EM_STATE_BIG_ACTIVATING_1;
								// 记录环数总和和各个标靶的环数
								energy_machine->ring[energy_machine->counter - 2] = can_message->data[0];
								energy_machine->ring_sum += can_message->data[0];
								energy_machine->counter_success++;
							}
							// 错误击打,回到IDLE状态
							else {
								ResetToIdle(energy_machine);
							}
							break;
						case EM_STATE_BIG_ACTIVATING_1:   // 大符正在激活 (1s阶段)
							// 是否正确击打
							if (IsRightTarget(can_message)) {
								energy_machine->timer_1s = 0;
								energy_machine->state = EM_STATE_BIG_ACTIVATING_25;
								energy_machine->ring[energy_machine->counter - 1] = can_message->data[0];
								energy_machine->ring_sum += can_message->data[0];
								energy_machine->counter_success++;
							}
							// 错误击打,回到2_5s状态
							else {
								energy_machine->timer_1s = 0;
								energy_machine->state = EM_STATE_BIG_ACTIVATING_25;
								energy_machine->ring[energy_machine->counter - 1] = 0;
								// 无需重置counter_success
							}
							// 判断是否满足退出条件
							if (energy_machine->counter == 10) {
								energy_machine->state = EM_STATE_BIG_SUCCESS;
								GetGainTime(&energy_machine->timer_SuccessToIdle);
								Big_EM_CANSend();
							}
							else if (Big_EM_CANSend() != 1) {
								xTaskNotifyGive(ErrorHandlerTaskHandle);
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
