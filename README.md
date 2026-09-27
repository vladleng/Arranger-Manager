# Arranger Manager

**Arranger Manager** — project-aware органайзер аранжировки для DAW.

Идея проекта: не переносить обычный todo-list внутрь DAW, а создать систему, которая понимает музыкальный timeline проекта, показывает степень готовности аранжировки по секциям и группам инструментов, хранит заметки в музыкальном контексте и помогает быстро увидеть незавершённые области.

Целевая DAW для первого прототипа — **Studio Pro**. Базовая технология исследования — **ARA 2** в связке с обычным VST3/shared state там, где ARA недостаточно.

## Главная идея

DAW timeline используется как источник структуры проекта.

Для MVP предлагается создать один или несколько служебных audio tracks и размечать готовность аранжировки настоящими silent audio events. Положение и длина event задают область, а имя/статус — смысл.

Пример:

```text
ARRANGER — Brass
[ BRASS:DONE ]             [ BRASS:DRAFT ]

ARRANGER — Strings
        [ STRINGS:REVIEW ]

ARRANGER — Bass
[──────────── BASS:DONE ────────────]
```

Arranger Manager должен превращать такую разметку в:

- coverage по партиям, группам и секциям;
- gaps / незаполненные участки;
- статусы `TODO / DRAFT / REVIEW / DONE`;
- project dashboard;
- список областей, требующих внимания;
- заметки и задачи, привязанные к timeline;
- в будущем — автоматический анализ активности партий через Track Agents.

## Документация

- [Полный концепт и архитектура](docs/CONCEPT.md)
- [Roadmap разработки](docs/ROADMAP.md)

## Принцип работы с Issues

**Документация хранит концепцию и архитектурные решения. Issues используются только для конкретных этапов разработки, тестов, багов и технических задач.**

Текущий первый технический этап — ARA Inspector / feasibility test: нужно экспериментально подтвердить, какие region-level данные Studio Pro реально предоставляет ARA-плагину.

## Статус

**Concept / pre-development.**

До начала полноценной реализации необходимо подтвердить ARA-возможности Studio Pro на служебных audio regions.

### Stage 0 prototype

`Arranger Manager Inspector 0.0a` is a separate, read-only ARA VST3. It lists the region sequences and playback regions visible to its ARA document controller, including explicit names/colors, position, duration and live update counter. It reuses the JUCE 9.0.2 + ARA SDK 2.3.0 integration pattern proven in Smart Voicing without carrying over the harmonizer or its shared-memory bridge.

The Windows package is produced by GitHub Actions. See [Studio Pro test checklist](docs/STAGE0-TEST.md). Host behavior remains unverified until the test is run in Studio Pro.

The [Map Preview 0.0b](docs/MAP-PREVIEW-0.0b.md) branch adds status and priority parsing with separate color badges while retaining the raw ARA report. It is an early Stage 1 interface experiment, not the coverage engine.
