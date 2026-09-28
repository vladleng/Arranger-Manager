# Arranger Manager — Roadmap

## Общий принцип

Roadmap хранит общую последовательность развития проекта. Конкретная реализация каждого этапа разбивается на GitHub Issues.

**Решение 2026-09-28:** разметка строится из существующих событий и метаданных самого проекта. Упоминания служебных silent-audio дорожек в Stage 0 ниже относятся к выполненному исследованию ARA, а не к обязательному пользовательскому сценарию. Чтение аудио- и MIDI-событий из сохранённого тестового `.song` подтверждено. Для Stage 1 проверить доступный способ переименовывать обычные события из Hub через Studio Pro.

```text
Stage 0  Feasibility / ARA Inspector
Stage 1  Arrangement Map Parser
Stage 2  Coverage + Gap Engine
Stage 3  Dashboard MVP
Stage 4  Notes / Tasks
Stage 5  Section-aware workflow
Stage 6  Track Agents
Stage 7  Advanced progress analysis
Stage 8  Session History
Stage 9  Harma Waves integration
Stage 10 UX / Stability / 1.0
```

---

## Stage 0 — ARA Inspector / Feasibility

Цель: экспериментально подтвердить, какие region-level данные Studio Pro реально предоставляет ARA-плагину.

Проверить:

- `RegionSequence`;
- `PlaybackRegion` для настоящего silent audio event;
- start position;
- duration;
- имя region/event;
- color;
- live updates после rename;
- move;
- resize;
- recolor;
- copy / duplicate;
- split;
- overlapping events;
- scope нескольких служебных tracks;
- muted track/event;
- поведение silent audio event, который используется только как служебный маркер.

Acceptance criteria:

- стабильно читаются start + duration;
- можно различить несколько служебных regions;
- найден надёжный способ идентифицировать логический тип/status region;
- изменение region на timeline отражается в Inspector;
- понятна модель scope: какие RegionSequence доступны данной ARA-инстанции.

Color желателен, но не обязателен для MVP.

### Ветвление после теста

Если central ARA instance видит все нужные служебные tracks:

```text
central ARA reader
→ Arrangement Map
```

Если scope ограничен:

```text
ARA reader/agent per service track
→ shared project state
→ Arranger Manager Hub
```

Если region metadata недостаточно:

```text
minimal Track/Map Agents
+ own plugin state
+ ARA only for Chord / Key / Tempo / Meter
```

---

## Stage 1 — Arrangement Map Parser

Цель: превратить существующие audio events и MIDI/instrument parts в внутреннюю модель проекта без служебных дорожек. Оба типа подтверждены на сохранённом тестовом `.song`.

Минимальная модель:

```text
ArrangementRegion
├ id
├ category / group
├ status
├ start
├ duration
├ end
└ optional color
```

Поддержать базовые статусы:

```text
TODO
DRAFT
REVIEW
DONE
```

Поддержать выбранный формат имени, например:

```text
BRASS:DONE
STRINGS:DRAFT
```

---

## Stage 2 — Coverage + Gap Engine

Цель: вычислять степень заполнения рабочего диапазона.

Нужно:

- union overlapping intervals;
- coverage 0–100%;
- gaps;
- расчёт отдельно по group/status;
- защита от двойного счёта overlapping regions;
- определение рабочего диапазона через `ARRANGEMENT RANGE` или manual Start/End.

Пример результата:

```text
Brass: 72%
Missing: bars 41–48, 73–80
```

---

## Stage 3 — Dashboard MVP

Первый действительно полезный пользовательский интерфейс.

Показывать:

- общий progress;
- progress по группам;
- gaps;
- Draft / Review / Done;
- Attention list;
- timeline-style overview.

Главный принцип UI:

> Пользователь должен за несколько секунд понять, что в аранжировке ещё не закончено.

---

## Stage 4 — Notes / Tasks

Добавить ручные задачи и заметки, привязанные к DAW-контексту.

Возможные связи:

- Project;
- group / track;
- section;
- time position / range;
- chord;
- instrument;
- status.

Это превращает Arranger Manager в музыкальный issue tracker внутри DAW.

---

## Stage 5 — Section-aware workflow

Добавить логические секции композиции:

```text
Intro
Verse
Chorus
Bridge
Solo
Outro
```

Источник section boundaries должен определяться отдельно и не должен предполагать наличие стандартного ARA marker API.

Возможные варианты:

- служебные section regions;
- собственная разметка Arranger Manager;
- host-specific интеграция в будущем.

Dashboard сможет показывать матрицу:

```text
              Verse  Chorus  Bridge  Outro
Brass          72%    100%     38%     0%
Strings         0%     68%     22%     0%
```

---

## Stage 6 — Track Agents

Опциональный более глубокий слой.

Маленькие VST3 Track Agents смогут сообщать Hub:

- MIDI activity;
- фактически звучащие области;
- CC1 / CC11;
- articulation / keyswitch activity;
- другие доступные track-level признаки.

Цель — добавить `Content Coverage`, не ломая простоту базовой Arrangement Map.

---

## Stage 7 — Advanced Progress Analysis

Разделить стадии готовности партии:

```text
Notes
Voicing
Voice Leading
Articulations
Dynamics
Humanization
Final Review
```

Часть статусов manual, часть может определяться автоматически.

---

## Stage 8 — Session History

Сохранять snapshots состояния Arranger Manager и показывать изменения между рабочими сессиями.

Пример:

```text
+ Brass added to Chorus 2
+ Strings: Draft → Review
+ arrangement extended by 8 bars
+ overall progress 64% → 71%
```

История принадлежит Arranger Manager и не должна зависеть от наличия DAW change log.

---

## Stage 9 — Harma Waves Integration

Опциональная интеграция через shared project context.

Потенциально передавать:

- voicing type;
- range warnings;
- voice-leading warnings;
- harmonic/tension warnings;
- musical processing status.

Arranger Manager должен оставаться полезным и без Harma Waves.

---

## Stage 10 — UX / Stability / 1.0

- сохранение project state;
- устойчивость shared state;
- обработка edge cases;
- оптимизация;
- финальный UI;
- regression tests;
- Studio Pro workflow validation;
- документация пользователя.

---

## Не включать в ранний MVP

- AI planner;
- cloud sync;
- автоматический анализ всех MIDI tracks;
- сложную совместную работу;
- обязательную интеграцию с Harma Waves;
- сложную аналитику expression/articulation до доказательства базовой модели.

Главное раннее доказательство идеи:

```text
service regions
→ reliable timeline map
→ coverage / gaps
→ useful Dashboard
```
