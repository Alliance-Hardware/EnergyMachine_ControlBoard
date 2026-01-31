#include "FreeRTOS.h"
#include "WS2812Task.h"
#include "WS2812.h"
#include "cmsis_os2.h"

void StartWS2812Task(void *argument)
{
	for (;;) {
		osDelay(20000);
		ws2812_update();
	}
}
