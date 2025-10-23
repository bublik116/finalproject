#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern QueueHandle_t sensor_data_q;
extern QueueHandle_t control_state_q;
extern QueueHandle_t mqtt_tx_q;
extern QueueHandle_t mqtt_cmd_q;
extern QueueHandle_t ui_event_q;
extern QueueHandle_t ui_cmd_q;
extern QueueHandle_t control_cfg_q;

extern EventGroupHandle_t sys_eg;

// Дескрипторы таймеров (опционально для использования из других модулей)
extern TimerHandle_t t_sensor;
extern TimerHandle_t t_control;
extern TimerHandle_t t_telemetry;
extern TimerHandle_t t_state;
extern TimerHandle_t t_watchdog;

void event_bus_init(void);

#ifdef __cplusplus
}
#endif
