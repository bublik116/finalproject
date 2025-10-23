#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

// Инициализация GPIO буззера (активный 5 В через ключ).
void drv_buzzer_init(void);

// Включить/выключить буззер (простое управление).
void drv_buzzer_on(bool on);

// Короткий звуковой сигнал заданной длительности (мс).
void drv_buzzer_beep(uint16_t ms);

// Установка частоты для пассивного буззера (заглушка для активного).
void drv_buzzer_set_freq(uint32_t hz); // опционально, заглушка

#ifdef __cplusplus
}
#endif
