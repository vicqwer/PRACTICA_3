#include "buttons.h"
#include "system_state.h"
#include "app_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

typedef struct
{
    int stable_state;
    int last_raw;
    int count;

} Debounce_t;

#define DEBOUNCE_COUNT 3

static bool debounce_update(uint8_t gpio, Debounce_t *db)
{
    int raw;
    bool pressed_event;

    raw = gpio_get_level((gpio_num_t)gpio);
    pressed_event = false;

    if (raw == db->last_raw)
    {
        if (db->count < DEBOUNCE_COUNT)
        {
            db->count++;
        }
    }
    else
    {
        db->count = 0;
        db->last_raw = raw;
    }

    if (db->count >= DEBOUNCE_COUNT)
    {
        if (raw != db->stable_state)
        {
            db->stable_state = raw;

            /*
               Botón normalmente abierto con pull-up:
               no presionado = 1
               presionado    = 0
            */
            if (db->stable_state == 1)
            {
                pressed_event = false;
            }
            if (db->stable_state == 0)
            {
                pressed_event = true;
            }
        }
    }

    return pressed_event;
}

void button_task(void *pvParameters)
{
    ButtonTaskParams_t *cfg = (ButtonTaskParams_t *)pvParameters;

    Debounce_t db =
    {
        .stable_state = 1,
        .last_raw = 1,
        .count = 0
    };

    gpio_reset_pin((gpio_num_t)cfg->gpio);
    gpio_set_direction((gpio_num_t)cfg->gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)cfg->gpio, GPIO_PULLUP_ONLY);

    while (1)
    {
        if (debounce_update(cfg->gpio, &db))
        {
            /*
               NO modificamos directamente el contador aquí.
               Solo pedimos al Task_Manager que procese el evento.
            */
            switch (cfg->type)
            {
                case BUTTON_START_PAUSE:
					/* El manager cambia a MANAGER_EVENT_START_PAUSE*/
                    g_system.pending_event = MANAGER_EVENT_START_PAUSE;
                    ESP_LOGI(TAG, "%s presionado", cfg->name);
                    break;

                case BUTTON_DIRECTION:
                    /*El manager cambia a MANAGER_EVENT_DIRECTION */
					g_system.pending_event = MANAGER_EVENT_DIRECTION;
                    ESP_LOGI(TAG, "%s presionado", cfg->name);
                    break;

                case BUTTON_SPEED:
					/*El manager cambia a MANAGER_EVENT_SPEED */
                    g_system.pending_event = MANAGER_EVENT_SPEED;
                    ESP_LOGI(TAG, "%s presionado", cfg->name);
                    break;

                default:
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}