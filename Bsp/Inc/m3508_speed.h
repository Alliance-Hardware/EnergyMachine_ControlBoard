#ifndef STM32F105_FR_M3508_SPEED_H
#define STM32F105_FR_M3508_SPEED_H
#include <stdint.h>
#include <stdbool.h>
#define STOP 0
#define FIXED_950 1
#define SINE_WAVE 2
#define GEAR_RATIO 95.0f
// 状态结构体
typedef struct {
	float a;      // 振幅 [0.780, 1.045]
	float omega;  // 角频率 [1.884, 2.000]
	float b;      // 偏置 b = 2.090 - a
	uint32_t start_time_ms;  // 激活开始时间（毫秒）
} EnergyMechanism_Speed;

extern EnergyMechanism_Speed em_speed;

// 重置能量机关参数,重新激活时候使用
void reset_energy_mechanism(void);

// 速度设定函数,用于设定目标速度
void set_motor_speed(uint8_t speed);

//获得目标速度的外部接口函数
float get_motor_target();

#endif