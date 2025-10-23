#include "core/event_bus.h"
#include "esp_log.h"

static const char *TAG = "BUS";

QueueHandle_t sensor_data_q = NULL;
QueueHandle_t control_state_q = NULL;
QueueHandle_t mqtt_tx_q = NULL;
QueueHandle_t mqtt_cmd_q = NULL;
QueueHandle_t ui_event_q = NULL;
QueueHandle_t ui_cmd_q = NULL;
QueueHandle_t control_cfg_q = NULL;

EventGroupHandle_t sys_eg = NULL;

TimerHandle_t t_sensor = NULL;
TimerHandle_t t_control = NULL;
TimerHandle_t t_telemetry = NULL;
TimerHandle_t t_state = NULL;
TimerHandle_t t_watchdog = NULL;

static void timer_noop_cb(TimerHandle_t xTimer) {
    (void)xTimer;
}

void event_bus_init(void)
{
    sensor_data_q   = xQueueCreate(8, sizeof(sensor_frame_t));
    control_state_q = xQueueCreate(8, sizeof(control_state_t));
    mqtt_tx_q       = xQueueCreate(16, 64); // заглушка: буфер под полезную нагрузку/индекс
    mqtt_cmd_q      = xQueueCreate(8, 64);  // заглушка: структура команд
    ui_event_q      = xQueueCreate(16, 16);
    ui_cmd_q        = xQueueCreate(8, 16);
    control_cfg_q   = xQueueCreate(4, 16);

    sys_eg = xEventGroupCreate();

    t_sensor    = xTimerCreate("t_sens", pdMS_TO_TICKS(1000), pdTRUE, 0, timer_noop_cb);
    t_control   = xTimerCreate("t_ctl",  pdMS_TO_TICKS(100),  pdTRUE, 0, timer_noop_cb);
    t_telemetry = xTimerCreate("t_tlm",  pdMS_TO_TICKS(10000),pdTRUE, 0, timer_noop_cb);
    t_state     = xTimerCreate("t_state",pdMS_TO_TICKS(60000),pdTRUE, 0, timer_noop_cb);
    t_watchdog  = xTimerCreate("t_wd",   pdMS_TO_TICKS(1000), pdTRUE, 0, timer_noop_cb);

    xTimerStart(t_sensor, 0);
    xTimerStart(t_control, 0);
    xTimerStart(t_telemetry, 0);
    xTimerStart(t_state, 0);
    xTimerStart(t_watchdog, 0);

    ESP_LOGI(TAG, "шина событий и очереди инициализированы");
}
