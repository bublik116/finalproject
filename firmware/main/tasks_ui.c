#include "tasks_ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "UI";

// Задача пользовательского интерфейса: отображение на TM1637 и обработка кнопок.
static void ui_task(void *arg)
{
    (void)arg;
    int cnt = 0;
    for (;;) {
        if ((cnt++ % 50) == 0) {
            ESP_LOGI(TAG, "UI работает (заглушка)");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ui_task_start(void)
{
    xTaskCreate(ui_task, "ui", 3072, NULL, 2, NULL);
}
