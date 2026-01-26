#include <math.h>
#include <string.h>
#include "m3508_ctrl.h"
// 内部全局变量
static M3508_Handle_t **g_motors = NULL;
static uint8_t g_motor_count = 0;

// 硬件参数定义
#define M3508_CTRL_ID1 0x200      // 对应电调接收指令标识符0x200
#define M3508_CTRL_ID2 0x1FF      // 对应电调接收指令标识符0x1FF
#define M3508_FEEDBACK_BASE 0x200 // 反馈基地址：0x200 + ID（ID=1~8）
#define ANGLE_MAX_MECH 8191       // 机械角度最大值
#define ANGLE_TO_DEG (360.0f / ANGLE_MAX_MECH) // 角度转换系数（机械值->度）

/**
 * @brief 初始化电机管理器
 */
void M3508_ManagerInit(M3508_Handle_t *motors[], uint8_t count)
{
	if (motors == NULL || count == 0 || count > 8) return;
	g_motors = motors;
	g_motor_count = count;
}

/**
 * @brief 初始化单个电机（默认电流环模式）
 */
void M3508_Init(M3508_Handle_t *motor, uint8_t id, void (*can_send)(uint16_t, uint8_t *))
{
	if (motor == NULL || can_send == NULL || id < 1 || id > 8) return;

	motor->id = id;
	motor->mode = M3508_MODE_CURRENT;
	motor->can_send = can_send;

	// 初始化目标值
	motor->target_pos = 0.0f;
	motor->target_speed = 0.0f;
	motor->target_current = 0.0f;
	motor->pos_set = 0.0f;
	motor->speed_set = 0.0f;

	// 初始化PID参数（M3508默认值，需根据实际调试修改）
	motor->speed_pid = (M3508_PID_Params_t){
		.kp = 0.3f, .ki = 2.0f, .kd = 0.005f, .output_limit = 2.0f, // 速度环输出限幅（对应电流A）
		.integral_limit = 0.5f // 速度环积分限幅
	};

	motor->pos_pid = (M3508_PID_Params_t){
		.kp = 1.5f, .ki = 0.0f, .kd = 0.05f, .output_limit = 300.0f, // 位置环输出限幅（对应转速rpm）
		.integral_limit = 50.0f // 位置环积分限幅
	};

	// 初始化PID内部变量
	memset(&motor->feedback, 0, sizeof(M3508_Feedback_t));
	motor->pos_err = 0.0f;
	motor->pos_err_last = 0.0f;
	motor->pos_integral = 0.0f;
	motor->speed_err = 0.0f;
	motor->speed_err_last = 0.0f;
	motor->speed_integral = 0.0f;
}

/**
 * @brief 在运行时更改电机句柄对应的电机ID。
 *
 * 注意：此函数仅更新软件结构体中的ID，不发送任何CAN指令。
 * 新ID必须在1到8的有效范围内。
 *
 * @param motor 要更改ID的电机句柄。
 * @param new_id 新的电机ID (1-8)。
 * @return int8_t 0: 成功 (Success), -1: 失败 (Failure - Invalid parameter)。
 */
uint8_t M3508_ChangeMotorId(M3508_Handle_t *motor, uint8_t new_id)
{
	// 检查电机句柄是否有效以及新ID是否在1到8的范围内
	if (motor == NULL || new_id < 1 || new_id > 8)
	{
		return -1; // 参数无效
	}

	// 更新电机句柄中的ID
	motor->id = new_id;

	return 0; // 成功
}

/**
 * @brief 设置控制模式
 */
void M3508_SetMode(M3508_Handle_t *motor, M3508_Mode_t mode)
{
	if (motor == NULL) return;
	motor->mode = mode;
	// 切换模式时重置PID积分项，防止冲击
	motor->pos_integral = 0.0f;
	motor->speed_integral = 0.0f;
}

/**
 * @brief 设置PID参数
 */
void M3508_SetPIDParams(M3508_Handle_t *motor, M3508_PID_Params_t *speed_params, M3508_PID_Params_t *pos_params)
{
	if (motor == NULL) return;
	if (speed_params != NULL) motor->speed_pid = *speed_params;
	if (pos_params != NULL) motor->pos_pid = *pos_params;
}

/**
 * @brief 设置目标值（根据当前模式解析）
 */
void M3508_SetTarget(M3508_Handle_t *motor, float target)
{
	if (motor == NULL) return;
	switch (motor->mode)
	{
		case M3508_MODE_POSITION:
			// 位置目标限幅在0~360度
			motor->target_pos = fmodf(target, 360.0f);
			if (motor->target_pos < 0) motor->target_pos += 360.0f;
			break;
		case M3508_MODE_SPEED:
			motor->target_speed = target; // 转速无硬性限制，由PID输出限幅
			break;
		case M3508_MODE_CURRENT:
			motor->target_current = target; // 电流会在SetCurrent中限幅
			break;
	}
}

/**
 * @brief PID计算（位置环→速度环→电流环 级联控制）
 */
void M3508_PIDCalculate(void)
{
	if (g_motors == NULL || g_motor_count == 0) return;

	for (uint8_t i = 0; i < g_motor_count; i++)
	{
		M3508_Handle_t *motor = g_motors[i];
		if (motor == NULL) continue;

		// 1. 位置环计算（仅在位置模式生效）
		if (motor->mode == M3508_MODE_POSITION)
		{
			// 机械角度转换为度（0~360）
			float current_pos = motor->feedback.angle * ANGLE_TO_DEG;

			// 计算角度误差（处理360度环回）
			motor->pos_err = motor->target_pos - current_pos;
			if (motor->pos_err > 180.0f)
			{
				motor->pos_err -= 360.0f;
			}
			else if (motor->pos_err < -180.0f)
			{
				motor->pos_err += 360.0f;
			}

			// 积分项计算与限幅
			motor->pos_integral += motor->pos_err * motor->pos_pid.ki;
			if (motor->pos_integral > motor->pos_pid.integral_limit)
			{
				motor->pos_integral = motor->pos_pid.integral_limit;
			}
			else if (motor->pos_integral < -motor->pos_pid.integral_limit)
			{
				motor->pos_integral = -motor->pos_pid.integral_limit;
			}

			// 位置环输出（作为速度环目标）
			motor->pos_set = motor->pos_err * motor->pos_pid.kp + motor->pos_integral + (
								 motor->pos_err - motor->pos_err_last) * motor->pos_pid.kd;

			// 位置环输出限幅
			if (motor->pos_set > motor->pos_pid.output_limit)
			{
				motor->pos_set = motor->pos_pid.output_limit;
			}
			else if (motor->pos_set < -motor->pos_pid.output_limit)
			{
				motor->pos_set = -motor->pos_pid.output_limit;
			}

			// 更新上一次误差
			motor->pos_err_last = motor->pos_err;
		}

		// 2. 速度环计算（位置模式下使用位置环输出，速度模式下使用目标速度）
		float speed_target = (motor->mode == M3508_MODE_POSITION) ? motor->pos_set : motor->target_speed;

		if (motor->mode != M3508_MODE_CURRENT)
		{
			// 电流模式跳过速度环
			// 速度误差（反馈转速单位：rpm）
			motor->speed_err = speed_target - motor->feedback.speed;

			// 积分项计算与限幅
			motor->speed_integral += motor->speed_err * motor->speed_pid.ki;
			if (motor->speed_integral > motor->speed_pid.integral_limit)
			{
				motor->speed_integral = motor->speed_pid.integral_limit;
			}
			else if (motor->speed_integral < -motor->speed_pid.integral_limit)
			{
				motor->speed_integral = -motor->speed_pid.integral_limit;
			}

			// 速度环输出（作为电流环目标，单位：A）
			motor->speed_set = motor->speed_err * motor->speed_pid.kp + motor->speed_integral + (
								   motor->speed_err - motor->speed_err_last) * motor->speed_pid.kd;

			// 速度环输出限幅
			if (motor->speed_set > motor->speed_pid.output_limit)
			{
				motor->speed_set = motor->speed_pid.output_limit;
			}
			else if (motor->speed_set < -motor->speed_pid.output_limit)
			{
				motor->speed_set = -motor->speed_pid.output_limit;
			}

			// 更新上一次误差
			motor->speed_err_last = motor->speed_err;
		}

		// 3. 电流环设置（根据模式选择电流来源）
		float current_target = (motor->mode == M3508_MODE_CURRENT) ? motor->target_current : motor->speed_set;

		// 转换为内部电流指令（限幅在±20A，M3508最大电流范围）
		const float MAX_CURRENT = 20.0f;
		const float MIN_CURRENT = -20.0f;
		if (current_target > MAX_CURRENT) current_target = MAX_CURRENT;
		else if (current_target < MIN_CURRENT) current_target = MIN_CURRENT;

		motor->target_current = current_target;
	}
}

/**
 * @brief 批量发送电流指令
 */
void M3508_SendAll(void)
{
	if (g_motors == NULL || g_motor_count == 0) return;

	uint8_t data1[8] = {0};
	uint8_t data2[8] = {0};

	for (uint8_t i = 0; i < g_motor_count; i++)
	{
		M3508_Handle_t *motor = g_motors[i];
		if (motor == NULL) continue;

		// 真实电流（A）转换为指令值（16384对应20A）
		int16_t curr_cmd = (int16_t) (motor->target_current * 16384.0f / 20.0f);
		uint8_t idx = motor->id - 1;

		if (idx < 4)
		{
			data1[idx * 2] = (curr_cmd >> 8) & 0xFF;
			data1[idx * 2 + 1] = curr_cmd & 0xFF;
		}
		else
		{
			data2[(idx - 4) * 2] = (curr_cmd >> 8) & 0xFF;
			data2[(idx - 4) * 2 + 1] = curr_cmd & 0xFF;
		}
	}

	if (g_motors[0] != NULL && g_motors[0]->can_send != NULL)
	{
		g_motors[0]->can_send(M3508_CTRL_ID1, data1);
		g_motors[0]->can_send(M3508_CTRL_ID2, data2);
	}
}

/**
 * @brief CAN接收回调（更新反馈数据）
 */
void M3508_CAN_RxCallback(uint16_t std_id, uint8_t *data)
{
	if (g_motors == NULL || g_motor_count == 0 || data == NULL) return;

	if (std_id < M3508_FEEDBACK_BASE + 1 || std_id > M3508_FEEDBACK_BASE + 8)
	{
		return;
	}

	uint8_t motor_id = std_id - M3508_FEEDBACK_BASE;
	for (uint8_t i = 0; i < g_motor_count; i++)
	{
		M3508_Handle_t *motor = g_motors[i];
		if (motor != NULL && motor->id == motor_id)
		{
			motor->feedback.angle = (data[0] << 8) | data[1];
			motor->feedback.speed = (int16_t) ((data[2] << 8) | data[3]);
			motor->feedback.current = (int16_t) ((data[4] << 8) | data[5]);
			motor->feedback.temp = data[6];
			motor->feedback.err_code = data[7];
			break;
		}
	}
}
