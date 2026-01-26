#include "FreeRTOS.h"
#include <string.h>
#include "HUB75Task.h"
EnergyMachine_t em = {0};
EnergyMachine_t *energy_machine = &em;

void EnergyMachine_Init(EnergyMachine_t *machine) {
	if (machine == NULL) {
		return;  // 防御性编程，防止空指针
	}
	machine->state = EM_STATE_INACTIVE;
	machine->activated_count = 0;
	machine->ring_sum = 0;
	memset(machine->last_leaf_ids, 0, sizeof(machine->last_leaf_ids));
	memset(machine->ring, 0, sizeof(machine->ring));
	machine->timer_1s = 0;
	machine->timer_2_5s = 0;
	machine->timer_20s = 0;
}


void StartHUB75Task(void *argument)
{

}
