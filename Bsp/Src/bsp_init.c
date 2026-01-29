#include "bsp_can.h"
#include "m3508_ctrl.h"
#include "ws2812.h"
#include "bsp_init.h"
M3508_Handle_t motor1;

void can_init(){
	BSP_CAN_Init();
}

static void m3508_can_send(uint16_t id, uint8_t *data) {
	BSP_CAN_SendMsg(&hcan2, id, data); // 注册现有CAN发送函数
}

void m3508_init()
{
	M3508_Handle_t *motors[] = {&motor1};                  // 电机数组
	uint8_t motor_count = sizeof(motors) / sizeof(motors[0]); // 电机数量
	//初始化电机管理器
	M3508_ManagerInit(motors, motor_count);
	//初始化特定电机
	M3508_Init(&motor1, 2, m3508_can_send);
	//初始化电机控制参数-速度
	M3508_PID_Params_t speed_pid = {.kp = 0.02000f,
									.ki = 0.00200f,
									.kd = 0.00800f,
									.output_limit = 10.0f,
									.integral_limit = 10.0f};
	M3508_SetPIDParams(&motor1, &speed_pid, NULL);
	//设定电机模式-速度环
	M3508_SetMode(&motor1, M3508_MODE_SPEED);
}

void timer_init()
{
	HAL_TIM_Base_Start_IT(&htim3);
}

void ws2812_init()
{
	ws2812_set_all(0X0000FF);//蓝色
	ws2812_update();//更新显示
}