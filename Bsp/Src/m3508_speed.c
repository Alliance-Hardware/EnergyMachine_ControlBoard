#include "m3508_speed.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "m3508_ctrl.h"
#include "timer.h"
EnergyMechanism_Speed em_speed = {0};
static bool rng_initialized = false;
static float motor_target = 1.0f;

// 初始化随机数生成器
static void init_rng_if_needed(void) {
	if (!rng_initialized) {
		srand(get_time());
		rng_initialized = true;
	}
}

// 重置函数：在进入可激活状态时调用
void reset_energy_mechanism(void) {
	init_rng_if_needed();
	// 生成 a 和 omega
	em_speed.a = 0.780f + (1.045f - 0.780f) * ((float)rand() / (float)RAND_MAX);
	em_speed.omega = 1.884f + (2.000f - 1.884f) * ((float)rand() / (float)RAND_MAX);
	em_speed.b = 2.090f - em_speed.a;
	em_speed.start_time_ms = get_time();  // 记录激活开始时间
}

float calculate_energy_mechanism_speed(void) {
	// 计算从激活开始经过的时间（秒）
	uint32_t elapsed_ms = get_time() - em_speed.start_time_ms;
	float current_time = (float)elapsed_ms / 1000.0f;
	// 计算目标转速（rad/s）
	float spd_rad_per_sec = em_speed.a * sinf(em_speed.omega * current_time) + em_speed.b;
	// 返回为rpm
	return GEAR_RATIO * spd_rad_per_sec * (60.0f / (2.0f * M_PI));
}

void set_motor_speed(const uint8_t speed) {
	switch (speed)
	{
		case STOP :
			motor_target = 0.0f;
			break;
		case FIXED_950 :
			motor_target = 950.0f;
			break;
		case SINE_WAVE:
			motor_target = calculate_energy_mechanism_speed();
			break;
		default:
			motor_target = 0.0f;
	}
}

float get_motor_target()
{
	return motor_target;
}