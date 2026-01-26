#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include "M3508ControlTask.h"
#include "HUB75Task.h"
#include "m3508_ctrl.h"
#include "m3508_speed.h"
extern M3508_Handle_t motor1;

void StartM3508ControlTask(void *argument){
	for (;;)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		switch (energy_machine->state)
		{
			case EM_STATE_SMALL_IDLE:
			case EM_STATE_SMALL_ACTIVATING:
				set_motor_speed(FIXED_950);
				break;
			case EM_STATE_BIG_IDLE:
			case EM_STATE_BIG_ACTIVATING_1:
			case EM_STATE_BIG_ACTIVATING_25:
				set_motor_speed(SINE_WAVE);
				break;
			case EM_STATE_INACTIVE:
			case EM_STATE_SMALL_SUCCESS:
			case EM_STATE_BIG_SUCCESS:
				set_motor_speed(STOP);
				break;
			default:
				set_motor_speed(STOP);
		}
		M3508_SetTarget(&motor1, get_motor_target());
		M3508_PIDCalculate();
		M3508_SendAll();
	}
}




