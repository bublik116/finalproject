#include "tasks_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "OTA";

static void ota_task(void *arg)
{
    (void)arg;
    for(;;){
        // Заглушка: ожидать флаг запуска OTA и обрабатывать обновление позднее
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void ota_task_start(void)
{
    xTaskCreate(ota_task, "ota", 6144, NULL, 5, NULL);
}
