#include "system_state.h"
#include "app_config.h"

SystemState_t g_system;

void system_state_init(void)
{
	/*TODO: 
		1. Establecer el valor de value a cero, 
		2. Establecer periodo_ms a una velocidad,
		3. Establecer dirección,
		4. Establecer modo,
		5. Establecer el estado del manager. :)*/

g_system.value = 0;
g_system.period_ms = SPEED_SLOW_MS;
g_system.direction = COUNT_UP;
g_system.mode = SYSTEM_PAUSED;
g_system.pending_event = MANAGER_EVENT_NONE;

}