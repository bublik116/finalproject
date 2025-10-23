#include "drv_relay_uln2003.h"
#include "esp_log.h"

static const char *TAG = "RELAY";

#define PIN_COMP   GPIO_NUM_25
#define PIN_FAN    GPIO_NUM_26
#define PIN_DEF    GPIO_NUM_27
#define PIN_LIGHT  GPIO_NUM_14

// Драйвер реле через ULN2003 (синк-ключи). Логический "1" включает транзистор и замыкает катушку на землю.
static inline void cfg_pin(gpio_num_t pin)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(pin, 0);
}

void drv_relay_init(void)
{
    cfg_pin(PIN_COMP);
    cfg_pin(PIN_FAN);
    cfg_pin(PIN_DEF);
    cfg_pin(PIN_LIGHT);
    ESP_LOGI(TAG, "инициализация завершена");
}

void drv_relay_set_compressor(bool on) { gpio_set_level(PIN_COMP, on); }
void drv_relay_set_fan(bool on)        { gpio_set_level(PIN_FAN, on); }
void drv_relay_set_defrost(bool on)    { gpio_set_level(PIN_DEF, on); }
void drv_relay_set_light(bool on)      { gpio_set_level(PIN_LIGHT, on); }
