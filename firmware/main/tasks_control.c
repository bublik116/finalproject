#include "tasks_control.h"
#include "core/types.h"
#include "core/event_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include "drivers/drv_relay_uln2003.h"
#include "drivers/drv_tm1637.h"

static const char *TAG = "CTRL";

// Простейшая конфигурация по умолчанию (до появления хранилища NVS)
static control_cfg_t g_cfg = {
    .setpoint = -3.0f,
    .hysteresis = 1.0f,
    .ton_min_s = 120,
    .toff_min_s = 180,
    .start_delay_s = 60,
    .defrost = {"time", 360, 20},
    .fan = {"with_compressor", 30},
    .door = {120, true},
    .calib_room = 0, .calib_evap = 0, .calib_aux1 = 0, .calib_aux2 = 0,
};

static bool s_comp_on = false;
static bool s_fan_on = false;
static int64_t s_last_on_ms = 0;
static int64_t s_last_off_ms = 0;
static int64_t s_boot_ms = 0;

static void control_task(void *arg)
{
    (void)arg;
    sensor_frame_t fr = {0};
    control_state_t st = {0};
    s_boot_ms = esp_timer_get_time() / 1000;
    for (;;) {
        if (xQueueReceive(sensor_data_q, &fr, pdMS_TO_TICKS(100))) {
            // Применим калибровку (если нужна)
            float t_room = fr.t_room + g_cfg.calib_room;

            // Гистерезисный контроль
            float up = g_cfg.setpoint + g_cfg.hysteresis * 0.5f;
            float dn = g_cfg.setpoint - g_cfg.hysteresis * 0.5f;
            int64_t now_ms = esp_timer_get_time() / 1000;

            if (!s_comp_on) {
                bool start_delay_ok = (now_ms - s_boot_ms) >= (int64_t)g_cfg.start_delay_s * 1000;
                bool toff_ok = (now_ms - s_last_off_ms) >= (int64_t)g_cfg.toff_min_s * 1000;
                if (t_room > up && start_delay_ok && toff_ok) {
                    s_comp_on = true;
                    s_last_on_ms = now_ms;
                }
            } else {
                bool ton_ok = (now_ms - s_last_on_ms) >= (int64_t)g_cfg.ton_min_s * 1000;
                if (t_room < dn && ton_ok) {
                    s_comp_on = false;
                    s_last_off_ms = now_ms;
                }
            }

            // Вентилятор: режим «с компрессором» (без пост‑задержки для простоты в MVP)
            s_fan_on = s_comp_on;

            // Управление реле
            drv_relay_set_compressor(s_comp_on);
            drv_relay_set_fan(s_fan_on);
            drv_relay_set_defrost(false);
            // Свет не трогаем здесь (отдельная логика/UI), пока держим выключенным
            drv_relay_set_light(false);

            // Отображение температуры на TM1637 с точностью 0.1°C
            drv_tm1637_display_temp(t_room);

            // Состояние для других задач
            memset(&st, 0, sizeof(st));
            strncpy(st.mode, s_comp_on ? "COOL" : "IDLE", sizeof(st.mode)-1);
            st.relays.compressor = s_comp_on;
            st.relays.fan = s_fan_on;
            st.relays.defrost = false;
            st.relays.light = false;
            st.alarm_active = false;
            st.defrost.active = false;
            st.defrost.since_s = 0;
            st.t_room = t_room;
            xQueueSend(control_state_q, &st, 0);

            ESP_LOGI(TAG, "T=%.2f SP=%.2f H=%.2f COMP=%d FAN=%d", t_room, g_cfg.setpoint, g_cfg.hysteresis, (int)s_comp_on, (int)s_fan_on);
        }
    }
}

void control_task_start(void)
{
    xTaskCreate(control_task, "control", 4096, NULL, 4, NULL);
}
