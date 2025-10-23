#include "tasks_sensors.h"
#include "core/types.h"
#include "core/event_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "drivers/drv_ds18b20.h"

#define DOOR_GPIO   GPIO_NUM_32

static void sensors_task(void *arg)
{
    (void)arg;
    sensor_frame_t fr = {0};
    for (;;) {
        int64_t now_ms = esp_timer_get_time() / 1000; // мс
        fr.ts_ms = now_ms;

        // 1) Запуск конверсии на всех датчиках и ожидание завершения (12 бит ~750 мс)
        ds18b20_start_conversion_all();
        vTaskDelay(pdMS_TO_TICKS(800));

        // 2) Чтение датчиков: 0 — камера, 1 — испаритель (если присутствуют)
        float t0 = 0.0f;
        if (ds18b20_read_temp_c(0, &t0)) {
            fr.t_room = t0;
            fr.valid_mask |= 0x01; // бит 0 — камера
        }

        float t1 = 0.0f;
        if (ds18b20_read_temp_c(1, &t1)) {
            fr.t_evap = t1;
            fr.valid_mask |= 0x02; // бит 1 — испаритель
        } else {
            fr.t_evap = 0.0f; // если второго нет — оставим 0 и бит валидности не ставим
        }

        // 3) Дверь: активный низкий (геркон на землю, включена подтяжка)
        fr.door = (gpio_get_level(DOOR_GPIO) == 0);
        xQueueSend(sensor_data_q, &fr, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void sensors_task_start(void)
{
    xTaskCreate(sensors_task, "sensors", 3072, NULL, 3, NULL);
}
