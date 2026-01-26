#include "FreeRTOS.h"
#include "task.h"
#include "bsp_init.h"
#include "cmsis_os2.h"
#include "HUB75Task.h"
#include "InitTask.h"
void StartInitTask(void *argument){
	osDelay(50);
	ws2812_init();
	can_init();
	m3508_init();
	timer_init();
	EnergyMachine_Init(energy_machine);
	vTaskDelete(NULL);
}
