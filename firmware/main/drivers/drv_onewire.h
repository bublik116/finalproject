#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void drv_onewire_init(gpio_num_t pin);
bool onewire_reset(void);
void onewire_write_byte(uint8_t b);
uint8_t onewire_read_byte(void);
void onewire_write_bit(uint8_t bit);
uint8_t onewire_read_bit(void);

#ifdef __cplusplus
}
#endif
