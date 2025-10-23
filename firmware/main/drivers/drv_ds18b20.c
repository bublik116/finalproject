#include "drv_ds18b20.h"
#include "esp_log.h"
#include "drv_onewire.h"

// Драйвер DS18B20 (MVP-версия, заглушки):
// - В этой версии функции возвращают тестовые значения, чтобы
//   проект успешно собирался и работал без реального железа.
// - Позже будет добавлена полноценная реализация через 1‑Wire
//   (поиск ROM, запуск конверсии, чтение scratchpad, CRC).

static const char *TAG = "DS18B20";

// Хранилище обнаруженных ROM‑кодов (поддержим до 2 датчиков)
#define DS18B20_MAX 2
static uint8_t s_roms[DS18B20_MAX][8];
static int s_count = 0;

// Dallas/Maxim CRC8 (полином 0x31, init 0x00)
static uint8_t crc8_maxim(const uint8_t *data, int len)
{
    uint8_t crc = 0;
    for (int i = 0; i < len; ++i) {
        uint8_t inbyte = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

// Алгоритм поиска 1‑Wire (SEARCH ROM, 0xF0), находит следующий ROM в шине.
// Упрощённая реализация для небольшого числа устройств.
static bool onewire_search_next(uint8_t rom_out[8], int *last_discrepancy, bool *last_device_flag)
{
    int id_bit_number = 1;
    int last_zero = 0;
    int rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    uint8_t rom_no[8] = {0};

    if (*last_device_flag) return false; // больше нет устройств
    if (!onewire_reset()) return false;  // нет присутствия

    // SEARCH ROM
    onewire_write_byte(0xF0);

    while (rom_byte_number < 8) {
        // Считываем бит и его инверс
        uint8_t id_bit = onewire_read_bit();
        uint8_t cmp_id_bit = onewire_read_bit();

        if (id_bit == 1 && cmp_id_bit == 1) {
            // Нет устройств в этой ветви
            return false;
        }

        uint8_t bit_to_write;
        if (id_bit != cmp_id_bit) {
            // Нет коллизии — все устройства имеют одинаковый бит
            bit_to_write = id_bit;
        } else {
            // Коллизия (оба 0) — выбираем направление
            if (id_bit_number < *last_discrepancy) {
                bit_to_write = (rom_out[rom_byte_number] & rom_byte_mask) ? 1 : 0;
            } else {
                bit_to_write = (id_bit_number == *last_discrepancy);
            }
            if (bit_to_write == 0) {
                last_zero = id_bit_number;
            }
        }

        // Записываем выбранный бит и строим ROM
        onewire_write_bit(bit_to_write);
        if (bit_to_write) rom_no[rom_byte_number] |= rom_byte_mask;

        id_bit_number++;
        rom_byte_mask <<= 1;
        if (rom_byte_mask == 0) {
            // Переход к следующему байту
            rom_out[rom_byte_number] = rom_no[rom_byte_number];
            rom_byte_number++;
            rom_byte_mask = 1;
        }
    }

    *last_discrepancy = last_zero;
    if (*last_discrepancy == 0) *last_device_flag = true;

    // Проверка CRC ROM
    if (crc8_maxim(rom_out, 7) != rom_out[7]) return false;
    return true;
}

bool ds18b20_start_conversion_all(void)
{
    if (!onewire_reset()) {
        ESP_LOGW(TAG, "нет присутствия на 1‑Wire");
        return false;
    }
    // Проще всего запустить конверсию на всех: SKIP ROM (0xCC) + CONVERT T (0x44)
    // Это сработает и для 1, и для 2 датчиков.
    onewire_write_byte(0xCC);
    onewire_write_byte(0x44); // CONVERT T
    return true; // конверсия асинхронная; подождать нужно снаружи
}

bool ds18b20_read_temp_c(uint8_t index, float *out_c)
{
    if (!out_c) return false;
    if (s_count == 0) return false;
    if (index >= (uint8_t)s_count) return false;

    if (!onewire_reset()) return false;
    // MATCH ROM выбранного датчика
    onewire_write_byte(0x55);
    for (int i = 0; i < 8; ++i) onewire_write_byte(s_roms[index][i]);
    // READ SCRATCHPAD
    onewire_write_byte(0xBE);
    uint8_t sp[9];
    for (int i = 0; i < 9; ++i) sp[i] = onewire_read_byte();
    // CRC считается по первым 8 байтам и сравнивается с девятым
    if (crc8_maxim(sp, 8) != sp[8]) {
        ESP_LOGW(TAG, "CRC ошибка scratchpad");
        return false;
    }
    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    // Разрешение по умолчанию 12 бит: шаг 0.0625°C
    *out_c = raw / 16.0f;
    return true;
}

int ds18b20_scan_bus(void)
{
    // Поиск до 2 устройств через SEARCH ROM
    s_count = 0;
    int last_discrepancy = 0;
    bool last_device_flag = false;
    uint8_t rom[8] = {0};

    while (s_count < DS18B20_MAX) {
        if (!onewire_search_next(rom, &last_discrepancy, &last_device_flag)) break;
        // Проверим, что это устройство семейства DS18B20 (0x28)
        if (rom[0] == 0x28) {
            for (int i = 0; i < 8; ++i) s_roms[s_count][i] = rom[i];
            ESP_LOGI(TAG, "обнаружен DS18B20[%d]: ROM=%02X%02X%02X%02X%02X%02X%02X%02X",
                     s_count, rom[0],rom[1],rom[2],rom[3],rom[4],rom[5],rom[6],rom[7]);
            s_count++;
        } else {
            ESP_LOGW(TAG, "найдено неизвестное устройство семейства 0x%02X — пропускаем", rom[0]);
        }
        if (last_device_flag) break;
    }

    ESP_LOGI(TAG, "итого на шине найдено датчиков: %d", s_count);
    return s_count;
}
