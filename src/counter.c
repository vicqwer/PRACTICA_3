#include "counter.h"
#include "system_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

// Etiqueta para los mensajes de log de la tarea del contador
static const char *TAG = "COUNTER";

/**
 * @brief Función auxiliar que ejecuta un paso del contador.
 *        Incrementa o decrementa el valor según la dirección actual.
 *        Cuando llega a 9, vuelve a 0 (y viceversa al bajar).
 */
static void counter_step(void)
{
    // Si la dirección configurada es SUBIR (COUNT_UP)
    if (g_system.direction == COUNT_UP)
    {
        // Si el valor llega a 9, reiniciar a 0 (cuenta módulo 10)
        if (g_system.value >= 9)
        {
            g_system.value = 0;
        }
        else
        {
            g_system.value++; // Incrementar en 1
        }
    }
    else // Si la dirección configurada es BAJAR (COUNT_DOWN)
    {
        // Si el valor es 0, saltar a 9 (cuenta regresiva módulo 10)
        if (g_system.value == 0)
        {
            g_system.value = 9;
        }
        else
        {
            g_system.value--; // Decrementar en 1
        }
    }
}

/**
 * @brief Función principal de la tarea del contador.
 *        Su función es ejecutar un paso cada vez que transcurre el periodo configurado.
 *        Es la tarea que el TaskManager suspende o reanuda para pausar/iniciar el sistema.
 */
void counter_task(void *pvParameters)
{
    (void)pvParameters; // No se usa en esta práctica, pero se recibe por xTaskCreate

    // Bucle infinito de la tarea
    while (1)
    {
        /*
           Si esta tarea está SUSPENDIDA (vTaskSuspend), NO entra aquí.
           Cuando está corriendo (vTaskResume), ejecuta el ciclo normalmente.
        */

        // Log del estado actual del contador antes de cada paso
        ESP_LOGI(TAG,"Valor=%u | Direccion=%s | Periodo=%lu ms",
                 g_system.value,
                 g_system.direction == COUNT_UP ? "UP" : "DOWN",
                 (unsigned long)g_system.period_ms);

        // Esperar (bloquearse) el tiempo establecido por el sistema (lento o rápido)
        // El TaskManager actualiza g_system.period_ms cuando se presiona el botón de velocidad
        vTaskDelay(pdMS_TO_TICKS(g_system.period_ms));

        // Una vez transcurrido el tiempo, ejecutar un paso (incremento o decremento)
        counter_step();
    }
}