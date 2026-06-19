#include "app_tasks.h"

#include "app_config.h"
#include "system_state.h"
#include "leds.h"
#include "buttons.h"
#include "counter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

// Etiqueta para los mensajes de log del TaskManager
static const char *TAG = "MANAGER";

// ==========================================================================
// DECLARACIÓN DE LOS HANDLES (TaskHandle_t) DE TODAS LAS TAREAS
// ==========================================================================
// Estos handles se guardan al crear las tareas con xTaskCreate.
// El TaskManager los usará para suspender o reanudar tareas según el estado.
static TaskHandle_t h_leds[4];
static TaskHandle_t h_btn_start;
static TaskHandle_t h_btn_dir;
static TaskHandle_t h_btn_speed;
static TaskHandle_t h_counter;
static TaskHandle_t h_manager;

// ==========================================================================
// CONFIGURACIONES DE LAS TAREAS (Estructuras para pvParameters)
// ==========================================================================
// Estas estructuras se pasan como pvParameters a las tareas de LEDs
static LedTaskParams_t led_params[4] =
{
    { .gpio = LED_B0, .bit_position = 0, .name = "LED_B0" },
    { .gpio = LED_B1, .bit_position = 1, .name = "LED_B1" },
    { .gpio = LED_B2, .bit_position = 2, .name = "LED_B2" },
    { .gpio = LED_B3, .bit_position = 3, .name = "LED_B3" }
};

// Configuración para la tarea del botón de Inicio/Pausa
static ButtonTaskParams_t btn_start =
{
    .gpio = BTN_START,
    .name = "BTN_START",
    .type = BUTTON_START_PAUSE
};

// Configuración para la tarea del botón de Dirección (Subir/Bajar)
static ButtonTaskParams_t btn_dir =
{
    .gpio = BTN_DIR,
    .name = "BTN_DIR",
    .type = BUTTON_DIRECTION
};

// Configuración para la tarea del botón de Velocidad (Lento/Rápido)
static ButtonTaskParams_t btn_speed =
{
    .gpio = BTN_SPEED,
    .name = "BTN_SPEED",
    .type = BUTTON_SPEED
};

// ==========================================================================
// FUNCIONES AUXILIARES
// ==========================================================================

// Convierte el estado interno de una tarea de FreeRTOS a un string legible
static const char *state_to_string(eTaskState state)
{
    switch (state)
    {
        case eRunning:   return "Running";
        case eReady:     return "READY";
        case eBlocked:   return "BLOCKED";
        case eSuspended: return "SUSPENDED";
        case eDeleted:   return "DELETED";
        default:         return "UNKNOWN";
    }
}

// ==========================================================================
// FUNCIONES DE CAMBIO DE ESTADO (Controladas por el TaskManager)
// ==========================================================================

/**
 * @brief Pausa el sistema completo.
 *        Suspende el contador y los botones de control, dejando solo activo el botón Start.
 */
static void manager_pause_system(void)
{
    g_system.mode = SYSTEM_PAUSED;

    /*
       Se suspende el contador:
       - conserva su valor actual
       - no ejecuta más pasos
       - NO regresa solo; necesita vTaskResume()
    */
    vTaskSuspend(h_counter);

    /*
       Se suspenden botones que NO deben funcionar en pausa.
       Solo queda activo el botón START/PAUSE.
    */
    vTaskSuspend(h_btn_dir);
    vTaskSuspend(h_btn_speed);

    ESP_LOGW(TAG, "Sistema PAUSADO");
}

/**
 * @brief Reanuda el sistema completo.
 *        Reactiva el contador y los botones de control.
 */
static void manager_run_system(void)
{
    g_system.mode = SYSTEM_RUNNING;

    /*
       Primero reanudamos botones de control.
    */
    vTaskResume(h_btn_dir);
    vTaskResume(h_btn_speed);

    /*
       Luego reanudamos contador.
    */
    vTaskResume(h_counter);

    ESP_LOGW(TAG, "Sistema RUNNING");
}

/**
 * @brief Cambia la dirección de conteo (UP a DOWN, o DOWN a UP).
 */
static void manager_toggle_direction(void)
{
    if (g_system.direction == COUNT_UP)
    {
        g_system.direction = COUNT_DOWN;
    }
    else
    {
        g_system.direction = COUNT_UP;
    }

    ESP_LOGI(TAG,"Nueva direccion: %s", g_system.direction == COUNT_UP ? "UP" : "DOWN");
}

/**
 * @brief Cambia la velocidad de conteo alternando entre LENTO y RÁPIDO.
 */
static void manager_toggle_speed(void)
{
    if (g_system.period_ms == SPEED_SLOW_MS)
    {
        g_system.period_ms = SPEED_FAST_MS;
    }
    else
    {
        g_system.period_ms = SPEED_SLOW_MS;
    }

    ESP_LOGI(TAG,"Nueva velocidad: %lu ms", (unsigned long)g_system.period_ms);
}

/**
 * @brief Imprime en el puerto serie el estado actual de todas las tareas y variables del sistema.
 */
static void manager_print_states(void)
{
    ESP_LOGI(TAG, "------ ESTADOS ------");

    ESP_LOGI(TAG,"COUNTER: %s", state_to_string(eTaskGetState(h_counter)));
    ESP_LOGI(TAG,"BTN_START: %s", state_to_string(eTaskGetState(h_btn_start)));
    ESP_LOGI(TAG,"BTN_DIR: %s", state_to_string(eTaskGetState(h_btn_dir)));
    ESP_LOGI(TAG,"BTN_SPEED: %s", state_to_string(eTaskGetState(h_btn_speed)));

    for (int i = 0; i < 4; i++)
    {
        ESP_LOGI(TAG,"%s: %s", led_params[i].name, state_to_string(eTaskGetState(h_leds[i])));
    }

    ESP_LOGI(TAG,"Valor=%u | Modo=%s | Direccion=%s | Periodo=%lu ms",
             g_system.value,
             g_system.mode == SYSTEM_RUNNING ? "RUNNING" : "PAUSED",
             g_system.direction == COUNT_UP ? "UP" : "DOWN",
             (unsigned long)g_system.period_ms);
}

// ==========================================================================
// TAREA PRINCIPAL DEL ADMINISTRADOR (TaskManager)
// ==========================================================================

/**
 * @brief Tarea del Administrador (TaskManager).
 *        Es el cerebro del sistema: procesa eventos de los botones,
 *        cambia estados y controla las demás tareas mediante sus TaskHandle_t.
 */
static void task_manager(void *pvParameters)
{
    (void)pvParameters; // No se usa en esta práctica, pero se recibe por xTaskCreate

    TickType_t last_print;
    last_print = xTaskGetTickCount(); // Inicializa el timer para el log periódico

    while (1) // Bucle infinito
    {
        ManagerEvent_t events;

        // Leer el evento pendiente en la variable global del sistema
        events = g_system.pending_event;

        // Si hay un evento por procesar...
        if (events != MANAGER_EVENT_NONE)
        {
            /*
               Consumimos el evento (lo borramos).
               Nota: todavía no usamos mutex; en esta práctica aceptamos
               esta variable simple para enseñar handles/estados.
            */
            g_system.pending_event = MANAGER_EVENT_NONE;

            // Decidir qué hacer según el tipo de evento
            switch (events)
            {
                case MANAGER_EVENT_START_PAUSE:
                    if (g_system.mode == SYSTEM_RUNNING)
                    {
                        manager_pause_system(); // Si está corriendo, lo pausa
                    }
                    else
                    {
                        manager_run_system();   // Si está pausado, lo reanuda
                    }
                    break;

                case MANAGER_EVENT_DIRECTION:
                    if (g_system.mode == SYSTEM_RUNNING)
                    {
                        manager_toggle_direction(); // Cambia dirección solo si está en marcha
                    }
                    else
                    {
                        ESP_LOGW(TAG,"Direccion ignorada: sistema pausado");
                    }
                    break;

                case MANAGER_EVENT_SPEED:
                    if (g_system.mode == SYSTEM_RUNNING)
                    {
                        manager_toggle_speed(); // Cambia velocidad solo si está en marcha
                    }
                    else
                    {
                        ESP_LOGW(TAG,"Velocidad ignorada: sistema pausado");
                    }
                    break;

                default:
                    break;
            }
        }

        /*
           Log periódico de estados cada 2 segundos.
        */
        if ((xTaskGetTickCount() - last_print) >= pdMS_TO_TICKS(2000))
        {
            last_print = xTaskGetTickCount();
            manager_print_states();
        }

        // Pequeño delay para no saturar el CPU (100ms)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==========================================================================
// CREACIÓN DE TAREAS (app_tasks_create)
// ==========================================================================

/**
 * @brief Crea todas las tareas del sistema y las coloca en su estado inicial.
 *        Se ejecuta una sola vez al arrancar el microcontrolador.
 */
void app_tasks_create(void)
{
    /* Creación de las tareas LED (4 tareas independientes con la misma función) */
    xTaskCreate(led_task, "LED_B0", 2048, &led_params[0], 1, &h_leds[0]);
    xTaskCreate(led_task, "LED_B1", 2048, &led_params[1], 1, &h_leds[1]);
    xTaskCreate(led_task, "LED_B2", 2048, &led_params[2], 1, &h_leds[2]);
    xTaskCreate(led_task, "LED_B3", 2048, &led_params[3], 1, &h_leds[3]);

    /* Creación de las tareas de los botones */
    xTaskCreate(button_task, "BTN_START", 2048, &btn_start, 2, &h_btn_start);
    xTaskCreate(button_task, "BTN_DIR", 2048, &btn_dir, 2, &h_btn_dir);
    xTaskCreate(button_task, "BTN_SPEED", 2048, &btn_speed, 2, &h_btn_speed);

    /* Creación de la tarea del contador y del TaskManager */
    xTaskCreate(counter_task, "COUNTER", 2048, NULL, 2, &h_counter);
    xTaskCreate(task_manager, "MANAGER", 4096, NULL, 3, &h_manager);

    /*
       Estado inicial del sistema (al arrancar el ESP32):
       - contador pausado
       - dirección y velocidad deshabilitados
       - start activo
       - leds activos mostrando valor inicial
    */
    manager_pause_system();
}