#include "tasks_comms.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "COMMS";

// Задача связи: Wi‑Fi/MQTT/HTTP/NTP (в этой версии — заглушка для компиляции)
static void comms_task(void *arg)
{
    (void)arg;
    int cnt = 0;
    for (;;) {
        if ((cnt++ % 50) == 0) {
            ESP_LOGI(TAG, "Коммуникации работают (заглушка)");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void comms_task_start(void)
{
    xTaskCreate(comms_task, "comms", 6144, NULL, 5, NULL);
}
