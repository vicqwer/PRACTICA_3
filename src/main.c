#include "system_state.h"
#include "app_tasks.h"

void app_main(void)
{
	
	/*TODO --> Inicializar el sistema y crear tareas*/
system_state_init();
app_tasks_create();

}