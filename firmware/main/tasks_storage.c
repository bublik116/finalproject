#include "tasks_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "STOR";

// Задача хранения: конфигурация в NVS, снапшоты, журналы (пока заглушка)
static void storage_task(void *arg)
{
    (void)arg;
    int cnt = 0;
    for(;;) {
        if ((cnt++ % 100) == 0) {
            ESP_LOGI(TAG, "Хранилище работает (заглушка)");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void storage_task_start(void)
{
    xTaskCreate(storage_task, "storage", 3072, NULL, 2, NULL);
}
