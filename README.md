# Arranger Manager

## Актуальное направление на 2026-10-02

Arranger Manager развивается в собственное рабочее пространство по принципу Notion с DAW: страницы задач, план дня, общий календарь, Timeline, аналитика соцсетей и интеграция с ChatGPT. Новое направление **запланировано**, рабочая реализованная версия — принятая **0.1d**.

Код и актуальные документы находятся в ветке [`hybrid-0-1d-folders-menu-song-notes`](https://github.com/vladleng/Arranger-Manager/tree/hybrid-0-1d-folders-menu-song-notes), [PR #34](https://github.com/vladleng/Arranger-Manager/pull/34); стек PR ещё не слит в main. Для нового чата начните с [HANDOFF](https://github.com/vladleng/Arranger-Manager/blob/hybrid-0-1d-folders-menu-song-notes/docs/HANDOFF.md), затем прочитайте [CONCEPT](https://github.com/vladleng/Arranger-Manager/blob/hybrid-0-1d-folders-menu-song-notes/docs/CONCEPT.md), [ROADMAP](https://github.com/vladleng/Arranger-Manager/blob/hybrid-0-1d-folders-menu-song-notes/docs/ROADMAP.md) и [WORKSPACE-ARCHITECTURE](https://github.com/vladleng/Arranger-Manager/blob/hybrid-0-1d-folders-menu-song-notes/docs/WORKSPACE-ARCHITECTURE.md).

| Плановый выпуск | Этап | Issue |
| --- | --- | --- |
| 0.2 | Рабочее пространство, страницы и единая модель задач | [#37](https://github.com/vladleng/Arranger-Manager/issues/37) |
| 0.3 | План дня из общего списка задач | [#38](https://github.com/vladleng/Arranger-Manager/issues/38) |
| 0.4 | Общий календарь всех страниц | [#39](https://github.com/vladleng/Arranger-Manager/issues/39) |
| 0.5 | Timeline для планирования проектов | [#40](https://github.com/vladleng/Arranger-Manager/issues/40) |
| 0.6 | Сбор и обзор аналитики соцсетей | [#41](https://github.com/vladleng/Arranger-Manager/issues/41) |
| 0.7 | Интеграция с ChatGPT и помощник по рабочему пространству | [#42](https://github.com/vladleng/Arranger-Manager/issues/42) |

Общий план: [#36](https://github.com/vladleng/Arranger-Manager/issues/36). Следующий подэтап — 0.1e: ID, хранилище и миграция данных 0.1d. Все новые этапы ещё не реализованы. .song остаётся только для чтения, телефон и синхронизация устройств — отложенная задача #35.

## Историческая концепция main

Текст ниже относится к первоначальному ARA-исследованию и **не задаёт действующие требования** (служебные дорожки, silent events и ARA не требуются для рабочего приложения).

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