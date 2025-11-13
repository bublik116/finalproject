---
description: Architecture template for ESP32 temperature and relay system
---

# Project Architecture Template

Fill placeholders, then use this document to scaffold the project.

## 1. Overview

- **Project name**: <PROJECT_NAME>
- **Device**: ESP32 on FreeRTOS
- **Sensors and actuators**:
  - **LCD1602**. 4-bit GPIO
  - **Buttons**. GPIO inputs with internal pull-ups. active low
  - **DS18B20**. single sensor on 1-Wire
  - **Relay**. 3.3 V logic control
- **Transport**: WebSocket only. TLS required
- **Backend**: FastAPI ASGI
- **Database**: SQLite with WAL
- **Frontend**: SPA with real-time charts
- **Hosting**: PythonAnywhere. verify WebSocket support on the plan

## 2. Hardware and Pin Map

Adjust pins to match your board and wiring. Avoid strapping pins.

- **LCD1602 4-bit GPIO**
  - RS: GPIO18
  - E: GPIO19
  - D4: GPIO21
  - D5: GPIO22
  - D6: GPIO23
  - D7: GPIO17
  - RW: GND
- **Buttons with pull-ups**
  - BTN1: GPIO32
  - BTN2: GPIO33
  - BTN3: GPIO25
  - Mode: INPUT_PULLUP. active low
- **DS18B20**
  - DATA: GPIO27 with 4.7 kΩ pull-up to 3.3 V
- **Relay**
  - RELAY_OUT: GPIO26. default LOW at boot

## 3. Firmware Architecture (ESP-IDF + FreeRTOS)

- **Tasks and priorities**
  - **Watchdog/Health**. prio 5. monitors queues and restart conditions
  - **NetTask**. prio 4. Wi-Fi station. SNTP time sync
  - **WsClientTask**. prio 4. maintains WebSocket. TX and RX queues. heartbeat
  - **SensorTask**. prio 3. DS18B20 non-blocking convert and read. CRC. filter
  - **RelayTask**. prio 3. applies commands. ensures safe transitions
  - **UiTask**. prio 2. button scan at 100 Hz with debounce. LCD updates at 5 to 10 Hz
- **IPC**
  - Queue telemetry_t from SensorTask to WsClientTask
  - Queue relay_cmd_t from UiTask and WsClientTask to RelayTask
  - Queue event_t from RelayTask to WsClientTask
  - EventGroup for WIFI_UP. WS_UP. TIME_VALID
  - Mutex for LCD writes if needed
- **Data structures**
  - telemetry_t { ts_ms. temp_c. relay. quality }
  - relay_cmd_t { id. desired. origin. ts_ms }
- **Timing**
  - DS18B20 resolution 12-bit. conversion about 750 ms. use state machine. not blocking
  - Publish telemetry at 1 Hz. include device ts_ms
- **Resilience**
  - Ring buffer for last 600 samples offline
  - Heartbeat every 10 s. use TCP keepalive if available
  - Config and secrets in NVS
- **LCD UX**
  - Line 1: Temp and Relay state
  - Line 2: Wi-Fi and WS status or menu
  - Long press on BTN to toggle relay for safety

## 4. WebSocket Protocol

- **Endpoints**
  - Device. wss://<DOMAIN>/ws/device?device_id=<DEVICE_ID>&token=<DEVICE_TOKEN>
  - UI. wss://<DOMAIN>/ws/ui?token=<USER_JWT>
- **Frames**
  - Telemetry from device
    {
      "type":"telemetry",
      "ts": 1731504000123,
      "device_id":"<DEVICE_ID>",
      "temp_c": 22.31,
      "relay": true,
      "fw":"1.0.0"
    }
  - Relay command to device
    {
      "type":"cmd",
      "id":"<UUID>",
      "action":"relay",
      "value": true,
      "requested_by":"user"
    }
  - Ack from device
    {
      "type":"ack",
      "cmd_id":"<UUID>",
      "status":"ok",
      "reason":""
    }
  - Relay event from device
    {
      "type":"relay_event",
      "ts": 1731504000456,
      "device_id":"<DEVICE_ID>",
      "state": true,
      "origin":"LOCAL" | "REMOTE"
    }
- **Reliability**
  - Commands require ack within timeout. backend updates command status. retry policy optional
  - Telemetry backpressure. drop oldest buffered items first
  - Heartbeats with ping and pong or TCP keepalive

## 5. Backend Architecture (FastAPI + SQLite)

- **Responsibilities**
  - WebSocket device endpoint. validate token. ingest telemetry and relay events. send commands to device
  - WebSocket UI endpoint. broadcast live telemetry and events
  - REST for history and command trigger
- **Endpoints**
  - WS
    - /ws/device
    - /ws/ui
  - REST
    - GET /api/telemetry?device_id=&from=&to=&bucket=1m
    - GET /api/relay-events?device_id=&from=&to=
    - POST /api/relay { device_id. value }
- **Processing**
  - Ingest writes to SQLite. publishes to in-process pubsub for live fanout
  - Command lifecycle. created. sent. acked or errored. expose status
- **SQLite configuration**
  - Enable WAL and set synchronous to NORMAL
  - Indices on (device_id. ts)
  - Retention job to delete old rows. consider downsampling for long range charts
- **Deployment on PythonAnywhere**
  - Confirm WebSocket availability on selected plan
  - Use ASGI server. Uvicorn or Hypercorn. single or few workers that support WS
  - Serve SPA static files via the same app or PythonAnywhere static mapping

## 6. Database Schema (SQLite)

- devices
  - id TEXT PRIMARY KEY. name TEXT. fw TEXT. created_at DATETIME. last_seen_at DATETIME
- telemetry
  - device_id TEXT. ts DATETIME. temp_c REAL. relay INTEGER. quality INTEGER
  - index (device_id. ts)
- relay_events
  - device_id TEXT. ts DATETIME. state INTEGER. origin TEXT
  - index (device_id. ts)
- commands
  - id TEXT PRIMARY KEY. device_id TEXT. ts_requested DATETIME. requested_by TEXT. payload TEXT. status TEXT. ts_ack DATETIME. error TEXT

## 7. Frontend

- **Views**
  - Dashboard with current temp and relay state
  - Live temp chart and relay state timeline
  - Toggle relay with confirmation. disabled if device offline
- **Tech**
  - React with Vite. Chart.js or ECharts
  - WebSocket client with reconnect backoff
  - REST client for history queries

## 8. Configuration

- **Device config**
  - WIFI_SSID. WIFI_PASS. DEVICE_ID. DEVICE_TOKEN. WS_URL
- **Backend config**
  - SECRET_KEY. JWT settings. SQLITE_PATH. RETENTION_DAYS. HOST. PORT
- **Frontend config**
  - API_BASE_URL. WS_UI_URL

## 9. Directory Structure

- firmware/
  - main/
    - app_main.c
    - tasks/
      - net_task.c. ws_task.c. sensor_task.c. relay_task.c. ui_task.c
    - drivers/
      - lcd1602_gpio.c. buttons.c. ds18b20.c. relay.c
    - proto/
      - ws_client.c. msg_codec.c
    - utils/
      - ring_buffer.c. time_sync.c
  - sdkconfig
- backend/
  - src/
    - main.py
    - ws/ device.py. ui.py. pubsub.py
    - api/ telemetry.py. relay.py
    - db/ schema.sql. dao.py
    - auth/ device.py. user.py
  - requirements.txt. tests/
- web/
  - src/
    - pages/Dashboard.tsx
    - components/TempChart.tsx. RelayTimeline.tsx
    - api/rest.ts. api/ws.ts

## 10. Security

- Device auth with per-device token. stored in NVS
- User auth with JWT. short TTL
- TLS only. wss and https
- Rate limiting for WS messages per device and per user
- Audit relay commands in commands table

## 11. Testing

- **Firmware**. unit tests for DS18B20 CRC. debounce. ring buffer. integration test with WS echo server
- **Backend**. tests for WS ingestion. REST history queries. command and ack flow
- **E2E**. simulated device at 1 Hz. UI toggles relay. assert ack and persistence

## 12. Milestones

- **M1**. Firmware skeleton. pin init. tasks. button debounce. LCD hello
- **M2**. DS18B20 driver and filtered telemetry. 1 Hz local log
- **M3**. WebSocket client. backend WS device endpoint. telemetry ingest
- **M4**. UI WebSocket. live dashboard
- **M5**. Relay command and ack. end to end toggle
- **M6**. History REST and charts. retention job
- **M7**. Deployment on PythonAnywhere. TLS and domain

## 13. Open Questions checklist

- Does PythonAnywhere plan support WebSocket for this app. yes or no
- Retention window in days. default 30
- Downsampling rules for long range charts. optional
- Exact LCD pin mapping on your board. confirm

## 14. References

- ESP-IDF WebSocket client. DS18B20 timing. LCD1602 4-bit protocol
- FastAPI WebSocket. SQLite WAL. PythonAnywhere ASGI
