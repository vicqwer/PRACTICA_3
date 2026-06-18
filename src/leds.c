#include "leds.h"
#include "system_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

void led_task(void *pvParameters)
{
    LedTaskParams_t *cfg = (LedTaskParams_t *)pvParameters;

    gpio_reset_pin((gpio_num_t)cfg->gpio);
    gpio_set_direction((gpio_num_t)cfg->gpio, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)cfg->gpio, 0);

    while (1)
    {
        uint8_t bit_value;

		
        bit_value = (g_system.value >> cfg->bit_position) & 0x01;

		/*TODO --> Después de actualizar el valor de bit_value, enviar a gpio*/
		gpio_set_level((gpio_num_t)cfg->gpio, bit_value);
        

        /*
           Tarea ligera: actualiza LED periódicamente.
           Cuando no hay cambios, queda BLOCKED.
        */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}