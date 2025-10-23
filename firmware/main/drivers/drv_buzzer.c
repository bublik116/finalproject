#include "drv_buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUZZ";

#define PIN_BUZZER GPIO_NUM_13

void drv_buzzer_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << PIN_BUZZER,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(PIN_BUZZER, 0);
    ESP_LOGI(TAG, "инициализация завершена");
}

void drv_buzzer_on(bool on)
{
    gpio_set_level(PIN_BUZZER, on ? 1 : 0);
}

void drv_buzzer_beep(uint16_t ms)
{
    drv_buzzer_on(true);
    vTaskDelay(pdMS_TO_TICKS(ms));
    drv_buzzer_on(false);
}

void drv_buzzer_set_freq(uint32_t hz)
{
    (void)hz; // заглушка (активный буззер не управляется частотой здесь)
}
