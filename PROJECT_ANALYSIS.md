# Обзор проекта ESP32 Fridge

- **Аппаратная часть (ESP32 прошивка)**: каталог `firmware/`
  - Задачи FreeRTOS: сенсоры, контроль холода, UI, OTA.
  - Индикация: `TM1637`; реле через ULN2003; клавиатура.
  - Меню уставок на индикаторе, антидребезг кнопок, подавление перерисовки дисплея задачей контроля во время работы UI.

- **Веб‑фронтенд**: каталог `web/`
  - Одностраничная панель: вкладки Обзор/История/Управление, журнал, настройки.
  - Визуализация истории на `<canvas>`, мок‑API (`window.FridgeAPI`), хранение базового URL в `localStorage`.

- **Облачный бэкенд**: каталог `smart-life-cloud/backend/`
  - FastAPI + SQLModel (SQLite). Домены: организации, устройства, телеметрия, события, конфиги. JWT‑аутентификация.
  - Точка входа: `app/main.py`; CORS (пока «всё разрешено»);
    auto‑create схемы БД (`SQLModel.metadata.create_all`).

---

# Ключевые находки

- **[Firmware/UI]**
  - Реализовано меню уставок на `TM1637` с антидребезгом кнопок.
    Файлы: `firmware/main/tasks_ui.c`, `firmware/main/tasks_control.c`, `firmware/main/drivers/drv_keys.c`.
  - Кнопки переназначены на GPIO с внутренней подтяжкой: `GPIO33/21/22/19`.
  - В `tasks_control.c` добавлена обработка очереди `control_cfg_q` и валидация уставок;
    в `control_state_t` — снимок уставок для UI.

- **[Firmware/индикация]**
  - UI «захватывает» дисплей; `tasks_control.c` проверяет `ui_overlay_active()`
    и не перерисовывает экран, пока активен оверлей/меню.

- **[Frontend]**
  - Файлы: `web/index.html`, `web/styles.css`, `web/api.js`, `web/app.js`.
  - Красивый интерфейс с табами, графиком, журналом, настройками и управляющими кнопками.
  - Мок‑API с локальным состоянием; планируется замена на реальные REST/WebSocket вызовы.

- **[Backend/архитектура]**
  - `app/main.py` — CORS «*», автосоздание схемы БД на старте.
  - Модели `app/models.py`: `Organization`, `User`, `Membership` (роль `Role`), `Site`, `Device`,
    `DeviceConfig` (JSON), `Telemetry` (JSON), `Event`.
  - Схемы: `app/schemas.py` — покрывают CRUD и телеметрию.
  - Безопасность: `app/security.py` — `JWT_SECRET` захардкожен, HS256, 8 часов.
  - Роутеры: `app/routers/*.py` (auth, organizations, devices, telemetry, events).
  - Зависимости: `app/deps.py` — `require_roles()` проверяет первую найденную запись `Membership`
    без привязки к `organization_id` (упрощённо, рискованно).

---

# Риски и пробелы

- **[Security/CORS и секреты]**
  - `JWT_SECRET` в коде (`app/security.py`), CORS `allow_origins=["*"]` (`app/main.py`).
  - В продакшене нужно вынести в окружение и ограничить источники.

- **[Авторизация/мульти‑тенантность]**
  - `require_roles()` (`app/deps.py`) игнорирует контекст организации; выбирает первую запись `Membership`.
  - Риск: пользователь с ролью в одной организации может доступаться к ресурсам другой,
    если роуты не фильтруют явно по `organization_id`.

- **[Целостность данных]**
  - `Device.device_uid` не уникален; минимум нужен `UNIQUE(organization_id, device_uid)`.

- **[Миграции]**
  - Используется `create_all()` без Alembic. Эволюция схемы будет болезненной.

- **[Производительность/масштаб]**
  - Телеметрия растёт. Нужны пагинация, индексы, ретеншн/TTL.
  - Проверить `telemetry.py`/`events.py` на `limit/offset` и сортировку по `ts DESC`.

- **[Frontend интеграция]**
  - Пока мок‑API. Нет привязки к реальным эндпоинтам бэкенда/устройства.

---

# Рекомендации

- **[Env‑конфигурация и CORS]**
  - Вынести настройки в переменные окружения: `JWT_SECRET`, `ACCESS_TOKEN_EXPIRE_MINUTES`, `DATABASE_URL`,
    `CORS_ALLOWED_ORIGINS`.
  - В `app/main.py` ограничить CORS — список доменов фронтенда из env (через запятую).

- **[Роли и границы организаций]**
  - Обновить `require_roles()` в `app/deps.py` для проверки членства в целевой организации:
    - Извлекать `org_id` из пути (`/orgs/{org_id}/...`) через зависимость.
    - Проверять `Membership` по `user_id` и `organization_id`.
  - В роутерах (`app/routers/*.py`) все запросы фильтровать по `organization_id` (и `site_id` при необходимости).

- **[Ограничения целостности]**
  - В `app/models.py` добавить уникальность `UNIQUE(organization_id, device_uid)` для `Device`.
  - Индексы: `Device.last_seen_at`, `Event.ts`, составной по `Telemetry(device_id, ts)`.

- **[Миграции]**
  - Добавить Alembic, сгенерировать первую миграцию, отключить `create_all()` для prod.

- **[Телеметрия: пагинация и защита]**
  - В `app/routers/telemetry.py` и `events.py`:
    - Параметры `limit`/`offset` с дефолтом и верхней границей.
    - Сортировка `ORDER BY ts DESC`.
    - Лимит размера payload и числа точек при ingest.

- **[Фронтенд: привязка к бэкенду]**
  - В `web/api.js` заменить моки на `fetch` к FastAPI (`/auth`, `/orgs/{org_id}/devices`, `/telemetry/...`).
  - Хранить JWT в `localStorage`, добавлять `Authorization: Bearer <token>`.
  - Для live‑обновлений — WebSocket/SSE на бэкенде.

- **[Прошивка]**
  - Сохранение уставок в NVS, восстановление при старте.
  - Удержание кнопок (автоинкремент), визуальные подсказки на `TM1637`.

---

# Быстрый план работ

- **[1]** Исправить `require_roles()` и провести ревизию фильтров по `organization_id` в роутерах.
- **[2]** Перевести секреты/URL/CORS на переменные окружения.
- **[3]** Добавить уникальность `device_uid` (per org) + Alembic миграции.
- **[4]** Пагинация и лимиты в `telemetry.py`/`events.py`.
- **[5]** Фронтенд: авторизация (JWT) и реальные запросы вместо моков.
- **[6]** Прошивка: NVS для уставок и UX улучшения меню.
