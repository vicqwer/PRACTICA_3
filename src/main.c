/**
 * =============================================================================
 * CONCLUSION DEL EQUIPO
 *
 * Integrantes: Victor Hugo Barrera Garcia, Sergio Garcia Hernandez
 *
 * En esta practica se implemento un contador BCD ascendente y descendente usando
 * FreeRTOS sobre una ESP32. El sistema se dividio en varias tareas para separar
 * la lectura de botones, el control de LEDs, la logica del contador y la
 * administracion general mediante un Task Manager. Esta organizacion permitio
 * observar de forma mas clara como el planificador ejecuta tareas independientes
 * segun su estado, prioridad y tiempo de bloqueo.
 *
 * Se comprobo la diferencia entre una tarea en estado BLOCKED y una tarea en
 * estado SUSPENDED. Cuando una tarea utiliza vTaskDelay(), pasa temporalmente a
 * BLOCKED y el planificador puede ejecutar otras tareas mientras termina el
 * tiempo de espera. En cambio, cuando el Task Manager usa vTaskSuspend(), la
 * tarea queda detenida indefinidamente hasta que sea reanudada explicitamente con
 * vTaskResume(). Esto se aplico para pausar el contador sin perder el ultimo
 * valor mostrado en los LEDs.
 *
 * El uso de pvParameters permitio reutilizar funciones de tarea con diferentes
 * configuraciones, por ejemplo para asociar cada LED o boton con su GPIO y su
 * nombre. Esto evita duplicar codigo y hace que el sistema sea mas facil de
 * ampliar. Por otro lado, TaskHandle_t permitio guardar referencias a tareas
 * especificas para que el Task Manager pudiera suspenderlas, reanudarlas y
 * consultar su estado mediante eTaskGetState().
 *
 * Tambien se observo que el Idle Task se ejecuta cuando no hay tareas listas y
 * que FreeRTOS puede repartir el tiempo de CPU entre tareas de la misma prioridad
 * mediante Round Robin. Si el contador se hubiera implementado solo con variables
 * globales y sin tareas especializadas, el programa seria menos modular, mas
 * dificil de depurar y no permitiria observar claramente los estados internos del
 * planificador.
 * =============================================================================
 */
#include "system_state.h"
#include "app_tasks.h"

void app_main(void)
{
	
	/* Inicializar el sistema y crear tareas*/
system_state_init();
app_tasks_create();

}