#include "drv_keys.h"
#include "driver/gpio.h"

#define PIN_KEY_MENU  GPIO_NUM_34
#define PIN_KEY_UP    GPIO_NUM_35
#define PIN_KEY_DOWN  GPIO_NUM_36
#define PIN_KEY_OK    GPIO_NUM_39

static inline void cfg_in(gpio_num_t pin)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
}

void drv_keys_init(void)
{
    cfg_in(PIN_KEY_MENU);
    cfg_in(PIN_KEY_UP);
    cfg_in(PIN_KEY_DOWN);
    cfg_in(PIN_KEY_OK);
}

void drv_keys_read(keys_state_t *out)
{
    if (!out) return;
    // Активный уровень низкий (кнопка на землю), т.к. включена подтяжка к 3.3 В
    out->menu = gpio_get_level(PIN_KEY_MENU) == 0;
    out->up   = gpio_get_level(PIN_KEY_UP)   == 0;
    out->down = gpio_get_level(PIN_KEY_DOWN) == 0;
    out->ok   = gpio_get_level(PIN_KEY_OK)   == 0;
}
