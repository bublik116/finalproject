#pragma once
#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void drv_relay_init(void);
void drv_relay_set_compressor(bool on);
void drv_relay_set_fan(bool on);
void drv_relay_set_defrost(bool on);
void drv_relay_set_light(bool on);

#ifdef __cplusplus
}
#endif
