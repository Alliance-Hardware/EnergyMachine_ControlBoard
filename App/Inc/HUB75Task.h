//
// Created by MECHREUO on 2026/1/22.
//

#ifndef STM32F105_FR_HUB75TASK_H
#define STM32F105_FR_HUB75TASK_H
#include <stdint.h>
#include <stdbool.h>
#define CAN_CALLBACK (1 << 0)
#define TIMER_CALLBACK (1 << 1)
#define RESET_FLAG (1 << 2)
// 状态枚举
typedef enum {
	EM_STATE_INACTIVE,           // 未激活状态
	EM_STATE_SMALL_IDLE,         // 小符待机
	EM_STATE_SMALL_ACTIVATING,   // 小符正在激活
	EM_STATE_SMALL_SUCCESS,      // 小符激活成功
	EM_STATE_BIG_IDLE,           // 大符待机
	EM_STATE_BIG_ACTIVATING_25,  // 大符正在激活 (2.5s阶段)
	EM_STATE_BIG_ACTIVATING_1,   // 大符正在激活 (1s阶段)
	EM_STATE_BIG_SUCCESS         // 大符激活成功
} EnergyMachine_State_t;

// 全局状态机结构体
typedef struct {
	EnergyMachine_State_t state;	// 激活状态
	uint8_t counter;				// 记录显示过的符叶总数
	uint8_t counter_success;		// 记录成功击打的符叶数量
	uint8_t ring_sum;			    // 记录打击环数和
	uint8_t ring[10];			    // 记录每次打击的环数
	uint8_t result_leaf_ids[2];		// 选中结果输出,并记录上次选中结果
	uint8_t selected_leaf_ids[5];   // 当前需要被击打的符叶ID
	uint8_t unselected_leaf_ids[5];	// 当前未被选中的符叶ID
	uint16_t timer_1s;			    // 1s定时器
	uint16_t timer_2_5s;		    // 2.5s定时器
	uint16_t timer_20s;				// 20s定时器
	uint16_t timer_InactiveToStart;	// 未激活时判断进入哪一个状态(小能量&大能量)
	uint16_t timer_SuccessToIdle;	// 激活后显示时间定时器
} EnergyMachine_t;

typedef struct{
	uint16_t id;
	uint8_t data[8];
}CANMessage;
// 全局变量声明
extern EnergyMachine_t *energy_machine;
// 函数声明
void EnergyMachine_Init(EnergyMachine_t *machine);
void HUB75_CAN_RxCallback(uint16_t std_id, uint8_t *data);
void HUB_TIM_CallBack(void);
void HUB75_Init(void);
void StartHUB75Task(void *argument);

#endif //STM32F105_FR_HUB75TASK_H