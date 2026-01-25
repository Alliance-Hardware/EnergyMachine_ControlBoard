#include "InitTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_init.h"
void StartInitTask(void){
	ws2812_init();
	can_init();
	m3508_init();
	timer_init();
	vTaskDelete(NULL);
}
