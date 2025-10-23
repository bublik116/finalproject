#include "drv_onewire.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

static const char *TAG = "1W";
static gpio_num_t s_pin = GPIO_NUM_NC;

static inline void ow_low(void)   { gpio_set_level(s_pin, 0); }
static inline void ow_release(void){ gpio_set_level(s_pin, 1); }
static inline int  ow_read(void)  { return gpio_get_level(s_pin); }

void drv_onewire_init(gpio_num_t pin)
{
    s_pin = pin;
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << s_pin,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    ow_release();
    ESP_LOGI(TAG, "инициализация на GPIO %d (bit-bang)", (int)s_pin);
}

bool onewire_reset(void)
{
    if (s_pin == GPIO_NUM_NC) return false;
    // Импульс сброса ~480 мкс в нуле
    ow_low();
    esp_rom_delay_us(480);
    ow_release();
    // Подождать ~70 мкс и считать присутствие (низкий уровень от датчика)
    esp_rom_delay_us(70);
    bool presence = (ow_read() == 0);
    // Дождаться окончания слота присутствия
    esp_rom_delay_us(410);
    return presence;
}

void onewire_write_byte(uint8_t b)
{
    for (int i = 0; i < 8; ++i) {
        uint8_t bit = (b >> i) & 0x01;
        // Слот ~60 мкс. Для "1": коротко вниз (~6 мкс), затем отпустить.
        // Для "0": держать низко ~60 мкс
        ow_low();
        if (bit) {
            esp_rom_delay_us(6);
            ow_release();
            esp_rom_delay_us(64);
        } else {
            esp_rom_delay_us(60);
            ow_release();
            esp_rom_delay_us(10);
        }
    }
}

uint8_t onewire_read_byte(void)
{
    uint8_t v = 0;
    for (int i = 0; i < 8; ++i) {
        // Начало слота чтения: кратковременно в ноль, затем отпуск и чтение через ~10 мкс
        ow_low();
        esp_rom_delay_us(3);
        ow_release();
        esp_rom_delay_us(10);
        int bit = ow_read();
        v |= (bit ? 1 : 0) << i;
        // Дождаться конца слота
        esp_rom_delay_us(50);
    }
    return v;
}

void onewire_write_bit(uint8_t bit)
{
    ow_low();
    if (bit) {
        esp_rom_delay_us(6);
        ow_release();
        esp_rom_delay_us(64);
    } else {
        esp_rom_delay_us(60);
        ow_release();
        esp_rom_delay_us(10);
    }
}

uint8_t onewire_read_bit(void)
{
    ow_low();
    esp_rom_delay_us(3);
    ow_release();
    esp_rom_delay_us(10);
    int bit = ow_read();
    esp_rom_delay_us(50);
    return (uint8_t)(bit & 1);
}
