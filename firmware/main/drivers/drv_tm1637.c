#include "drv_tm1637.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static const char *TAG = "TM1637";

#define TM1637_CLK GPIO_NUM_16
#define TM1637_DIO GPIO_NUM_17

// Коды сегментов для 0..9 и пусто/минус
static const uint8_t DIGITS[12] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x00, // blank
    0x40  // '-'
};

static inline void clk_high(void){ gpio_set_level(TM1637_CLK, 1); }
static inline void clk_low(void) { gpio_set_level(TM1637_CLK, 0); }
static inline void dio_high(void){ gpio_set_level(TM1637_DIO, 1); }
static inline void dio_low(void) { gpio_set_level(TM1637_DIO, 0); }

static void dio_out(void)
{
    gpio_set_direction(TM1637_DIO, GPIO_MODE_OUTPUT_OD);
}
static void dio_in(void)
{
    gpio_set_direction(TM1637_DIO, GPIO_MODE_INPUT);
}

static inline void delay_us(int us){ esp_rom_delay_us(us); }

static void start(void)
{
    dio_out();
    clk_high(); dio_high(); delay_us(2);
    dio_low(); delay_us(2);
    clk_low(); delay_us(2);
}

static void stop(void)
{
    dio_out();
    clk_low(); delay_us(2);
    dio_low(); delay_us(2);
    clk_high(); delay_us(2);
    dio_high(); delay_us(2);
}

static void write_byte(uint8_t b)
{
    for (int i = 0; i < 8; ++i) {
        // бит LSB первым
        clk_low(); delay_us(2);
        if (b & 0x01) dio_high(); else dio_low();
        delay_us(2);
        clk_high(); delay_us(4);
        b >>= 1;
    }
    // ACK
    clk_low(); dio_in(); dio_high(); delay_us(2);
    clk_high(); delay_us(4); // игнорируем уровень ACK
    clk_low(); dio_out();
}

// Выставить 4 байта сегментов на индексы 0..3
void drv_tm1637_set_segments(const uint8_t segs[4])
{
    // 0x40 — автоинкремент адреса
    start(); write_byte(0x40); stop();
    // 0xC0 — адрес 0
    start(); write_byte(0xC0);
    for (int i = 0; i < 4; ++i) write_byte(segs[i]);
    stop();
    // 0x88 | brightness (0..7)
    start(); write_byte(0x88 | 0x07); stop();
}

void drv_tm1637_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << TM1637_CLK) | (1ULL << TM1637_DIO),
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE, // подтяжки обязательны (на 5 В стороне стоят резисторы)
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    clk_high(); dio_high();
    ESP_LOGI(TAG, "инициализация завершена");
}

static void encode_int_to_segs(int value, uint8_t out[4])
{
    // Диапазон отображения: -999..9999 (упрощенно)
    bool negative = (value < 0);
    int v = negative ? -value : value;
    int d0 = v % 10; v /= 10;
    int d1 = v % 10; v /= 10;
    int d2 = v % 10; v /= 10;
    int d3 = v % 10;
    out[0] = (d3 ? DIGITS[d3] : (negative ? DIGITS[11] : DIGITS[10]));
    out[1] = (d3 || d2) ? DIGITS[d2] : DIGITS[10];
    out[2] = (d3 || d2 || d1) ? DIGITS[d1] : DIGITS[10];
    out[3] = DIGITS[d0];
}

void drv_tm1637_display_int(int value)
{
    uint8_t segs[4];
    encode_int_to_segs(value, segs);
    drv_tm1637_set_segments(segs);
}

// Отобразить температуру с одной цифрой после запятой для диапазона [-9.9 .. 99.9].
// Вне диапазона — показываем целое значение без дробной части.
// Используем десятичную точку (DP = 0x80) у предыдущего разряда.
void drv_tm1637_display_temp(float value)
{
    uint8_t segs[4] = { DIGITS[10], DIGITS[10], DIGITS[10], DIGITS[10] }; // по умолчанию пусто
    const uint8_t DP = 0x80;

    if (value >= -9.9f && value <= 99.9f) {
        // Округление до 0.1°C
        int scaled = (int)(value * 10.0f + (value >= 0 ? 0.5f : -0.5f));
        bool neg = (scaled < 0);
        if (neg) scaled = -scaled;
        int ip = scaled / 10;   // целая часть
        int dpd = scaled % 10;  // десятая

        if (neg) {
            // Диапазон -9.9 .. -0.1  => "-X.Y"
            // [-] [X.] [Y] [ ]
            segs[0] = DIGITS[11];
            if (ip >= 0 && ip <= 9) {
                segs[1] = DIGITS[ip] | DP;
                segs[2] = DIGITS[dpd];
                segs[3] = DIGITS[10];
            } else {
                // На всякий случай, если ip вышла за пределы (не должно быть)
                encode_int_to_segs((int)value, segs);
            }
        } else {
            // 0.0 .. 99.9
            if (ip <= 9) {
                // [ ] [X.] [Y] [ ]
                segs[0] = DIGITS[10];
                segs[1] = DIGITS[ip] | DP;
                segs[2] = DIGITS[dpd];
                segs[3] = DIGITS[10];
            } else {
                // 10.0 .. 99.9  => "XY.Z"
                // [X] [Y.] [Z] [ ]
                int tens = ip / 10;
                int ones = ip % 10;
                segs[0] = DIGITS[tens];
                segs[1] = DIGITS[ones] | DP;
                segs[2] = DIGITS[dpd];
                segs[3] = DIGITS[10];
            }
        }
        drv_tm1637_set_segments(segs);
        return;
    }

    // Вне диапазона — показываем целое число (откажемся от дополнительной точности)
    int iv = (int)(value + (value >= 0 ? 0.5f : -0.5f));
    encode_int_to_segs(iv, segs);
    drv_tm1637_set_segments(segs);
}
