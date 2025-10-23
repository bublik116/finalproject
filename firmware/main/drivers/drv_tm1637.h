#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Инициализация TM1637 (предполагается согласование уровней 3.3В↔5В)
void drv_tm1637_init(void);

// Отобразить целое число (упрощённый интерфейс)
void drv_tm1637_display_int(int value);

// Установить сырые сегменты для 4 разрядов (a..g + dp)
void drv_tm1637_set_segments(const uint8_t segs[4]);

// Отобразить температуру с точностью 0.1°C на 4‑разрядном индикаторе.
// Правила форматирования:
//  - От −9.9 до 99.9 показываем одну цифру после запятой (пример: 36.6, -3.5).
//  - Меньше −9.9 показываем целые без десятичной точки (пример: -12).
void drv_tm1637_display_temp(float value);

#ifdef __cplusplus
}
#endif
