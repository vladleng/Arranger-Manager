# Arranger Manager

**Arranger Manager** — project-aware органайзер аранжировки для DAW.

Идея проекта: не переносить обычный todo-list внутрь DAW, а создать систему, которая понимает музыкальный timeline проекта, показывает степень готовности аранжировки по секциям и группам инструментов, хранит заметки в музыкальном контексте и помогает быстро увидеть незавершённые области.

Целевая DAW для первого прототипа — **Studio Pro**. Текущий Hub работает как обычный VST3 и читает сохранённый проект `.song`. Эксперимент с ARA 2 отложен до появления конкретной потребности.

## Главная идея

DAW timeline используется как источник структуры проекта.

Текущий выбор для MVP: **сам проект Studio Pro является источником данных**. Arranger Manager читает уже существующие дорожки и аудио/MIDI-клипы, а статус и приоритет извлекает из имени обычного события, например `POOL | P2`. Отдельная служебная дорожка не требуется. Чтение сохранённого `.song` подтверждено для Info, Notes, заметок дорожек, аудиособытий и MIDI-партий тестового проекта.

Пример:

```text
Drums
[ TODO | P2 ]          [ WIP ]       [ WAIT | P3 ]

Bass
[ Intro ]               [ Chorus ]
```

Arranger Manager должен превращать такую разметку в:

- coverage по партиям, группам и секциям;
- gaps / незаполненные участки;
- статусы `POOL / TODO / WIP / DRAFT / WAIT / DONE / BLOCKED`;
- project dashboard;
- список областей, требующих внимания;
- заметки и задачи, привязанные к timeline;
- в будущем — автоматический анализ активности партий через Track Agents.

## Документация

- [Полный концепт и архитектура](docs/CONCEPT.md)
- [Roadmap разработки](docs/ROADMAP.md)

## Принцип работы с Issues

**Документация хранит концепцию и архитектурные решения. Issues используются только для конкретных этапов разработки, тестов, багов и технических задач.**

Текущий технический этап — Hub: импорт событий из сохранённого проекта и проверка обновления после сохранения в Studio Pro.

## Статус

**Hub 0.0f — первый прототип чтения проекта.**

Hub показывает Info, Notes и основные аудио/MIDI события из сохранённого `.song` без ARA. Изменения, сделанные в Studio Pro, становятся видны после сохранения проекта. В открытом проекте клипы и заметки редактируются непосредственно в Studio Pro; будущий отдельный каталог проектов сможет редактировать закрытый `.song` после проверки безопасной записи. См. [инструкцию к текущей сборке Hub](docs/HUB-0.0i.md) и [исследование моста](docs/CONTEXT-BRIDGE-PROBE.md).

В [Hub 0.0g](docs/HUB-0.0g.md) добавлены раскрывающиеся уровни проекта, дорожек и клипов, а также секции аранжировки и маркеры. Приоритеты: P1 красный (самый высокий), P2 оранжевый, P3 жёлтый, P4 серый (самый низкий). Старое имя `REVIEW` читается как `WAIT` для совместимости с уже размеченными клипами.

В [Hub 0.0h](docs/HUB-0.0h.md) область аранжировки показывается цветной меткой `[Название]` прямо у пересекающихся клипов вместо отдельного раздела. Колонка Notes после Priority показывает заметки соответствующего трека из `notepad.xml`.

В [Hub 0.0i](docs/HUB-0.0i.md) текст клипа очищен от уже выведенных в отдельных колонках статуса и приоритета; цветной кружок у дорожки читает её цвет из проекта. Цвета областей и дорожек отражают последний сохранённый `.song`.

### Stage 0 prototype

`Arranger Manager Inspector 0.0a` is a separate, read-only ARA VST3. It lists the region sequences and playback regions visible to its ARA document controller, including explicit names/colors, position, duration and live update counter. It reuses the JUCE 9.0.2 + ARA SDK 2.3.0 integration pattern proven in Smart Voicing without carrying over the harmonizer or its shared-memory bridge.

The Windows package is produced by GitHub Actions. See [Studio Pro test checklist](docs/STAGE0-TEST.md). Host behavior remains unverified until the test is run in Studio Pro.

The [Map Preview 0.0b](docs/MAP-PREVIEW-0.0b.md) branch adds status and priority parsing with separate color badges while retaining the raw ARA report. It is an early Stage 1 interface experiment, not the coverage engine.
