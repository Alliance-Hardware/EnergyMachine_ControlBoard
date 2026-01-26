#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include "ErrorHandlerTask.h"
void StartErrorHandlerTask(void *argument)
{
	ulTaskNotifyTake(pdTRUE ,portMAX_DELAY);
	for (;;)
	{
		osDelay(1000);
	}
}
