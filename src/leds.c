#include "leds.h"
#include "system_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

/**
 * @brief Función principal de la tarea de cada LED.
 *        Su función es leer el valor global del contador y reflejar el estado de 
 *        su bit correspondiente en el GPIO físico.
 * 
 *        Esta tarea se crea 4 veces (una por cada LED) usando la MISMA función,
 *        pero recibiendo diferentes parámetros (GPIO y posición del bit) a través
 *        de la estructura LedTaskParams_t.
 */
void led_task(void *pvParameters)
{
    // 1. Recibir la configuración a través del puntero pvParameters
    //    Esto permite que la misma función sirva para los 4 LEDs.
    LedTaskParams_t *cfg = (LedTaskParams_t *)pvParameters;

    // 2. Configurar el hardware del GPIO como salida digital
    gpio_reset_pin((gpio_num_t)cfg->gpio);
    gpio_set_direction((gpio_num_t)cfg->gpio, GPIO_MODE_OUTPUT);
    
    // Asegurar que el LED inicia apagado al arrancar el sistema
    gpio_set_level((gpio_num_t)cfg->gpio, 0);

    // 3. Bucle infinito de la tarea
    while (1)
    {
        uint8_t bit_value;

        /* 
           Operación de desplazamiento de bits (Bit Shifting):
           - g_system.value es el número decimal actual (ej: 5 = 0101 binario).
           - cfg->bit_position indica qué LED somos (0 para el bit menos significativo, 3 para el más significativo).
           - El operador '>>' desplaza los bits a la derecha.
           - El operador '& 0x01' (máscara) aísla solo el bit menos significativo (0 o 1).
           
           Ejemplo: Si value = 5 (0101) y somos bit_position = 1:
           1. Desplazamos 1: 0101 >> 1 = 0010 (2 en decimal).
           2. Hacemos AND con 1: 0010 & 0001 = 0 (Apagado).
        */
        bit_value = (g_system.value >> cfg->bit_position) & 0x01;

        // Escribir el valor calculado (0 o 1) en el GPIO físico
        gpio_set_level((gpio_num_t)cfg->gpio, bit_value);
        
        /*
           Tarea ligera: actualiza LED periódicamente.
           Al usar vTaskDelay, la tarea se BLOQUEA (eBlocked) durante 20 ms.
           Esto libera el procesador para que otras tareas (TaskManager, Botones, Contador) puedan ejecutarse.
           Cuando no hay cambios en el valor del contador, el LED simplemente mantiene su estado.
        */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}