#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "core/event_bus.h"
#include "tasks_control.h"
#include "tasks_sensors.h"
#include "tasks_ui.h"
#include "tasks_comms.h"
#include "tasks_storage.h"
#include "tasks_ota.h"
#include "drivers/drv_relay_uln2003.h"
#include "drivers/drv_buzzer.h"
#include "drivers/drv_tm1637.h"
#include "drivers/drv_keys.h"
#include "drivers/drv_onewire.h"
#include "drivers/drv_ds18b20.h"

// Основная точка входа прошивки esp32_fridge (MVP).
// Инициализирует периферию, объекты ОС (очереди/таймеры) и запускает задачи.
static const char *TAG = "APP";

#define LED_GPIO    GPIO_NUM_2
#define DOOR_GPIO   GPIO_NUM_32

static void init_gpio_basic(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(LED_GPIO, 0);

    // Вход двери: GPIO32, подтяжка вверх, без прерываний
    gpio_config_t door = {
        .pin_bit_mask = 1ULL << DOOR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&door);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "esp32_fridge starting...");

    init_gpio_basic();

    event_bus_init();

    drv_relay_init();
    drv_buzzer_init();
    drv_tm1637_init();
    drv_keys_init();

    // 1‑Wire + DS18B20: инициализация и первичное сканирование
    drv_onewire_init(GPIO_NUM_23);
    ds18b20_scan_bus();

    sensors_task_start();
    control_task_start();
    ui_task_start();
    storage_task_start();
    comms_task_start();
    ota_task_start();

    // Мигаем светодиодом в главном потоке как «пульс» системы
    while (true) {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}
