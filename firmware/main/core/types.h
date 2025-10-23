#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int64_t ts_ms;
    float t_room;
    float t_evap;
    float t_aux1;
    float t_aux2;
    bool door;
    uint8_t valid_mask; // битовая маска валидности датчиков
} sensor_frame_t;

typedef struct {
    bool compressor;
    bool fan;
    bool defrost;
    bool light;
} relays_t;

typedef struct {
    float setpoint;
    float hysteresis;
    uint16_t ton_min_s;
    uint16_t toff_min_s;
    uint16_t start_delay_s;
    struct { char type[8]; uint16_t interval_min; uint16_t max_min; } defrost;
    struct { char mode[16]; uint16_t post_delay_s; } fan;
    struct { uint16_t door_alarm_s; bool fan_off_on_door; } door;
    float calib_room, calib_evap, calib_aux1, calib_aux2;
    char device_id[32];
    char site_id[32];
    char mqtt_uri[96];
} control_cfg_t;

typedef struct {
    char mode[12]; // IDLE/COOL/DEFROST/DRIP/ALARM/SERVICE (состояние контроллера)
    relays_t relays;
    bool alarm_active;
    struct { bool active; uint32_t since_s; } defrost;
    float t_room; // текущая температура камеры для UI/телеметрии
} control_state_t;
