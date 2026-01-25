//
// Created by MECHREUO on 2026/1/22.
//

#ifndef STM32F105_FR_HUB75TASK_H
#define STM32F105_FR_HUB75TASK_H
#include <stdint.h>
#include <stdbool.h>

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
	EnergyMachine_State_t state;
	uint8_t activated_count;       // 已成功激活次数
	uint8_t last_leaf_ids[5];      // 记录点亮的符叶ID
	bool is_big;                   // 标志当前是大符还是小符模式
} EnergyMachine_t;

typedef struct {
	uint16_t timer_1s;     // 1s定时器
	uint16_t timer_2_5s;   // 2.5s定时器
	uint16_t timer_20s;    // 20s定时器
} EnergyMachine_timer;

// 全局变量声明
extern EnergyMachine_t em;
extern EnergyMachine_timer em_timer;

// 函数声明
void HUB75_CAN_RxCallback(uint16_t std_id, uint8_t *data);
void HUB_TIM_CallBack(void);
void HUB75_Init(void);

#endif //STM32F105_FR_HUB75TASK_H