#include "FreeRTOS.h"
#include "task.h"
#include "WS2812Task.h"
#include "WS2812.h"
#include "cmsis_os2.h"
#include "Config.h"

void StartWS2812Task(void *argument)
{
	for (;;) {
		uint32_t pulNotificationValue = 0;
		xTaskNotifyWait(0, 0,
			&pulNotificationValue, portMAX_DELAY);
		switch (pulNotificationValue) {
			case COLOR_RED:
				ws2812_set_all(0XAA0000);
				break;
			case COLOR_GREEN:
				ws2812_set_all(0X00AA00);
				break;
			case COLOR_BLUE:
				ws2812_set_all(0X0000AA);
				break;
			default:
				break;
		}
		taskENTER_CRITICAL();
		ws2812_update();
		taskEXIT_CRITICAL();
	}
}
