#include "buttons.h"
#include "system_state.h"
#include "app_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

// Etiqueta para los mensajes de log de la tarea de botones
static const char *TAG = "BUTTON";

// Estructura para mantener el estado del filtro de rebote (debounce) de un botón
typedef struct
{
    int stable_state;   // Estado lógico estable confirmado (1 = suelto, 0 = presionado)
    int last_raw;       // Último valor leído directamente del GPIO
    int count;          // Contador de veces que el valor raw se ha mantenido igual

} Debounce_t;

// Número de lecturas consecutivas iguales necesarias para confirmar un cambio (filtro de ruido)
#define DEBOUNCE_COUNT 3

/**
 * @brief Función de filtrado por rebote para evitar lecturas falsas debido al rebote mecánico del botón.
 * 
 * @param gpio El número del pin GPIO a leer.
 * @param db Puntero a la estructura de estado del debounce.
 * @return true Si se detectó una pulsación confirmada (presionado).
 * @return false Si no hubo cambio o fue ruido.
 */
static bool debounce_update(uint8_t gpio, Debounce_t *db)
{
    int raw;
    bool pressed_event;

    // 1. Leer el estado actual del pin (0 o 1)
    raw = gpio_get_level((gpio_num_t)gpio);
    pressed_event = false;

    // 2. Verificar si la lectura es igual a la lectura anterior
    if (raw == db->last_raw)
    {
        // Si es igual, aumentamos el contador (la señal se está estabilizando)
        if (db->count < DEBOUNCE_COUNT)
        {
            db->count++;
        }
    }
    else
    {
        // Si es diferente, hubo ruido; reiniciamos el contador y actualizamos el último valor conocido
        db->count = 0;
        db->last_raw = raw;
    }

    // 3. Si el contador llegó al límite (se mantuvo estable) y es diferente al estado estable...
    if (db->count >= DEBOUNCE_COUNT)
    {
        if (raw != db->stable_state)
        {
            db->stable_state = raw; // Confirmamos el nuevo estado estable

            /*
               Botón normalmente abierto con pull-up:
               - Nivel ALTO (1) = Botón SUELTO (No presionado)
               - Nivel BAJO (0) = Botón PRESIONADO
            */
            if (db->stable_state == 1)
            {
                pressed_event = false; // Liberación del botón
            }
            if (db->stable_state == 0)
            {
                pressed_event = true;  // Pulsación confirmada
            }
        }
    }

    return pressed_event; // Retorna true solo cuando el botón es presionado y confirmado
}

/**
 * @brief Función principal de la tarea del botón.
 *        Lee el GPIO periódicamente, aplica el filtro de rebote y notifica al sistema mediante una variable global.
 */
void button_task(void *pvParameters)
{
    // 1. Recibir la configuración a través del puntero pvParameters
    ButtonTaskParams_t *cfg = (ButtonTaskParams_t *)pvParameters;

    // 2. Inicializar la estructura de debounce (suponemos que inicia suelto = 1)
    Debounce_t db =
    {
        .stable_state = 1,
        .last_raw = 1,
        .count = 0
    };

    // 3. Configurar el hardware del GPIO (Pull-up interno activado)
    gpio_reset_pin((gpio_num_t)cfg->gpio);
    gpio_set_direction((gpio_num_t)cfg->gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)cfg->gpio, GPIO_PULLUP_ONLY);

    // 4. Bucle infinito de la tarea
    while (1)
    {
        // Leer el botón con el filtro de rebote. Si devuelve true, hubo una pulsación válida.
        if (debounce_update(cfg->gpio, &db))
        {
            /*
               NO modificamos directamente el contador aquí.
               Solo pedimos al Task_Manager que procese el evento.
               La comunicación se realiza mediante una variable global (g_system.pending_event).
            */
            switch (cfg->type)
            {
                case BUTTON_START_PAUSE:
                    // El TaskManager cambiará a MANAGER_EVENT_START_PAUSE
                    g_system.pending_event = MANAGER_EVENT_START_PAUSE;
                    ESP_LOGI(TAG, "%s presionado", cfg->name);
                    break;

                case BUTTON_DIRECTION:
                    // El TaskManager cambiará a MANAGER_EVENT_DIRECTION
                    g_system.pending_event = MANAGER_EVENT_DIRECTION;
                    ESP_LOGI(TAG, "%s presionado", cfg->name);
                    break;

                case BUTTON_SPEED:
                    // El TaskManager cambiará a MANAGER_EVENT_SPEED
                    g_system.pending_event = MANAGER_EVENT_SPEED;
                    ESP_LOGI(TAG, "%s presionado", cfg->name);
                    break;

                default:
                    break;
            }
        }

        // Esperar el tiempo de sondeo configurado (ej: 30 ms) antes de la siguiente lectura
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}