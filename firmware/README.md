# Прошивка esp32_fridge (MVP)

Требования:
- ESP-IDF v5.x (проверено с 5.2.x)

Сборка:
- `idf.py set-target esp32`
- `idf.py build`
- `idf.py -p COMx flash monitor` (укажите ваш COM‑порт)

Назначение GPIO по умолчанию (можно изменить в коде):
- 1‑Wire (DS18B20): `GPIO23`
- TM1637: `GPIO16` (CLK), `GPIO17` (DIO)
- Реле: `GPIO25` (COMP), `GPIO26` (FAN), `GPIO27` (DEF), `GPIO14` (LIGHT)
- Дверь: `GPIO32`
- Кнопки: `GPIO34` (Menu), `GPIO35` (Up), `GPIO36` (Down), `GPIO39` (OK)
- Буззер: `GPIO13`
- Светодиод: `GPIO2`

Примечания:
- В текущей версии часть драйверов реализована как заглушки для упрощения сборки (TM1637, 1‑Wire/DS18B20).
- По мере готовности железа заменим заглушки на полноценные реализации с таймингами и CRC.

## Диаграммы архитектуры

- PlantUML: [../docs/queues-architecture.puml](../docs/queues-architecture.puml)
- Mermaid: [../docs/queues-architecture.mmd](../docs/queues-architecture.mmd)

Подсказки по просмотру:
- PlantUML: используйте плагин VS Code “PlantUML” или офлайн рендер `plantuml.jar`:
  - `java -jar plantuml.jar ../docs/queues-architecture.puml`
- Mermaid: GitHub/VS Code умеют рендерить блоки Mermaid. Можно открыть файл `.mmd` прямо в IDE с соответствующим расширением.
