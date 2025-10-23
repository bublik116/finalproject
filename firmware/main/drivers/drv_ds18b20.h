#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Запустить измерение температуры на всех датчиках на шине 1‑Wire.
// Возвращает true при успешной отправке команды.
bool ds18b20_start_conversion_all(void);

// Прочитать температуру (°C) с датчика по индексу (0..N-1) после завершения конверсии.
// out_c — указатель для записи результата. Возвращает true при успехе.
bool ds18b20_read_temp_c(uint8_t index, float *out_c);

// Просканировать шину и вернуть количество найденных датчиков (заглушка в MVP).
int  ds18b20_scan_bus(void); // возвращает число найденных датчиков (stub)

#ifdef __cplusplus
}
#endif
