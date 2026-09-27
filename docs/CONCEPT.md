# Arranger Manager — концепт и архитектура

## 1. Product Vision

**Arranger Manager** — не «Notion внутри DAW», а **project-aware органайзер аранжировки**, который понимает музыкальный timeline проекта и помогает отвечать на вопросы:

- какие участки аранжировки уже сделаны;
- какие секции ещё пустые или требуют доработки;
- насколько заполнены партии и группы инструментов;
- где находятся `TODO / DRAFT / REVIEW / DONE` области;
- какие задачи и заметки относятся к конкретному месту timeline;
- что изменилось между рабочими сессиями;
- какие области требуют внимания прямо сейчас.

Ключевой принцип:

> DAW timeline является источником структуры проекта, а Arranger Manager превращает эту структуру в понятную карту прогресса.

Цель — не заставить аранжировщика вручную дублировать состояние проекта в отдельном менеджере задач, а максимально использовать уже существующую разметку внутри DAW.

---

## 2. Базовая идея Arrangement Map

Для MVP не требуется автоматически читать все MIDI-клипы проекта.

В Studio Pro создаётся один или несколько служебных **audio tracks** с настоящими тихими audio events. Эти events используются как визуальные и машинно-читаемые блоки прогресса.

Пример:

```text
ARRANGER — Brass
[ BRASS:DONE ]             [ BRASS:DRAFT ]

ARRANGER — Strings
        [ STRINGS:REVIEW ]

ARRANGER — Bass
[──────────── BASS:DONE ────────────]
```

Каждый служебный блок должен логически иметь:

- положение на timeline;
- длительность;
- имя;
- статус;
- при наличии надёжной поддержки host — цвет;
- принадлежность к категории/группе инструментов.

### Почему именно silent audio event

Не следует строить архитектуру на предположении, что полностью пустой event без `AudioSource` обязательно будет представлен в ARA как usable region.

Для надёжного первого прототипа лучше использовать настоящий silent audio source/event. Он визуально выполняет роль служебного блока, а технически остаётся обычным audio event.

В тестовом проекте пользователь подтвердил, что ARA-разметка остаётся доступной после сохранения/повторного открытия и при mute. Отдельно подтвердить поведение именно служебного silent audio source/event перед использованием его как основы Arrangement Map.

### Stage 0 observation — Event FX placement (2026-09-27)

В тестовом проекте Studio Pro ARA Inspector находится **на каждом audio event** как Event FX. При копировании или разрезании event Studio Pro копирует вставку вместе с ним, что позволяет быстро размечать трек. Пользователь отключил обработку этих вставок; открытый Inspector при этом продолжил получать изменения ARA-модели. Это наблюдение относится к текущей сессии, а сохранение/повторное открытие и mute трека ещё требуют проверки.

На скриншоте восемь Event FX относятся к восьми events на двух tracks (`Drums`, `Bass`), но Inspector показывает один ARA document controller, две `RegionSequence` и восемь `PlaybackRegion`. Поэтому для MVP рассматриваем **одну логическую карту проекта**, собираемую из регионов всех доступных последовательностей, при обязательной ARA-вставке на размечаемых событиях. Не считать наличие одной вставки достаточным для чтения произвольных events без неё. На скриншотах использованы звучащие events с waveform; пригодность настоящего silent audio event пока остаётся отдельной проверкой.

---

## 3. Статусы

Базовый набор:

```text
TODO
DRAFT
REVIEW
DONE
```

Возможные схемы кодирования имени:

```text
BRASS:DONE
STRINGS:DRAFT
BASS:REVIEW
```

или

```text
DONE | Brass
DRAFT | Strings
REVIEW | Bass
```

### Цвет как вспомогательный статус

Если Studio Pro стабильно передаёт color region/event через доступный ARA scope, цвет можно использовать как визуальное дублирование:

```text
green  → DONE
yellow → DRAFT
blue   → REVIEW
red    → TODO
```

Но цвет не должен быть единственным источником истины. Основной статус надёжнее хранить в имени или собственном metadata/plugin state.

---

## 4. Coverage / Progress Engine

Прогресс нужно считать не как сумму длин отдельных events, а как **union занятых интервалов** внутри рабочего диапазона.

Пример:

```text
Project range: 120 bars
Brass DONE union: 72 bars
Coverage: 60%
```

Перекрывающиеся events не должны приводить к значению выше 100%.

Кроме процента нужно показывать gaps:

```text
Brass  72%
██████████████░░░░██████████░░░░

Missing:
41–48
73–80
```

### Нужно различать три разных понятия

```text
CLIP COVERAGE
есть ли служебная разметка области

CONTENT COVERAGE
есть ли фактический музыкальный материал

ARRANGEMENT STATUS
TODO / DRAFT / REVIEW / DONE
```

Для MVP достаточно:

```text
Clip Coverage + Arrangement Status
```

Content Coverage можно добавить позже через Track Agents или другие источники.

---

## 5. Рабочий диапазон проекта

Не следует считать стандартный ARA универсальным API для DAW markers.

Поэтому MVP должен иметь host-neutral способ определить область расчёта прогресса:

1. специальный служебный region `PROJECT` / `ARRANGEMENT RANGE`;
2. ручные Start / End в Arranger Manager;
3. host-specific интеграция позже, если Studio Pro позволит надёжно читать markers или аналогичную структуру.

Пример:

```text
ARRANGEMENT RANGE
[────────────────────────────────────────]
```

Coverage всех категорий рассчитывается только внутри этого диапазона.

---

## 6. Project Dashboard

Главный экран должен показывать состояние аранжировки, а не обычный todo-list.

Пример:

```text
Human Nature
Arrangement: 68%

                 Verse   Chorus   Bridge   Outro
Harmony           100%     100%     100%    100%
Rhythm Section    100%     100%      92%     74%
Brass              72%     100%      38%      0%
Strings             0%      68%      22%      0%

Attention:
- Brass / Bridge incomplete
- Strings / Outro not started
- Harmony gap: bars 65–68
```

Дополнительный вид по одной группе:

```text
BRASS
Intro       ██████████ 100%
Verse A     ████████░░  80%
Chorus      ██████████ 100%
Bridge      ███░░░░░░░  30%
Outro       ░░░░░░░░░░   0%
```

Главный вопрос, на который должен отвечать Dashboard:

> Что в этой аранжировке ещё не закончено?

---

## 7. Attention / автоматически формируемый список проблем

Arranger Manager должен формировать список незавершённых или подозрительных областей.

Базовый уровень:

```text
ATTENTION
- Bridge: Brass incomplete
- Outro: Bass not started
- Chorus 2: Strings status = Draft
- Bars 65–68: no arrangement marker
```

В будущем сюда можно добавлять предупреждения от Track Agents и других инструментов:

- нет MIDI activity в ожидаемой области;
- нет expression CC;
- отсутствуют articulation/keyswitch события;
- range warning;
- voice-leading warning;
- harmonic warning.

---

## 8. Tasks / Notes

Поверх автоматической карты нужны обычные ручные задачи и заметки, но привязанные к музыкальному контексту DAW.

Возможные привязки:

```text
Project
Track / Group
Section
Time position / range
Chord
Instrument
Status
```

Пример:

```text
Bar 42 / D7alt / Brass
"Попробовать UST Eb и оставить верхнюю трубу на 13."
```

Такой слой превращает Arranger Manager в музыкальный issue tracker прямо внутри проекта.

---

## 9. Потенциально полезный ARA-контекст

Для Arranger Manager интересны две группы данных.

### Musical Context

```text
Chord
Key
Tempo
Time Signature
```

### Region-level structure

Потенциально интересны:

```text
RegionSequence
PlaybackRegion
start
 duration
name
color
```

Но архитектурно обязательно соблюдать правило:

```text
ARA specification capability
!=
Studio Pro actually exposes it to our plugin
```

Поэтому region-based часть проекта должна начинаться не с production-кода, а с **ARA Inspector / feasibility test**.

Также не следует исходить из того, что стандартный ARA предоставляет универсальный список MIDI Parts проекта. Для MIDI потребуется отдельный механизм или host-specific путь.

---

## 10. Optional Track Agents — future

Если в будущем потребуется более глубокий анализ MIDI/instrument tracks, можно добавить маленькие служебные VST3 `Track Agent`.

Предварительная архитектура:

```text
                Arranger Manager
                       │
          ┌────────────┼────────────┐
          ↓            ↓            ↓
        ARA         Track Agent   Track Agent
   Project Map         Brass        Strings
          │             │             │
  Chords/Key       MIDI/CC state  MIDI/CC state
  Tempo/Meter      progress       progress
          └─────────────┴─────────────┘
                       ↓
                Project Database
```

Track Agent может в будущем отслеживать:

- наличие MIDI activity;
- фактически звучащие участки;
- CC1 / CC11;
- pitch bend / automation, если доступно через соответствующий plugin path;
- keyswitch/articulation activity;
- собственный status/progress;
- другие track-level признаки.

Это **не требуется для MVP**.

---

## 11. Уровни готовности партии — future

В перспективе полезно уйти от одного процента и различать стадии работы:

```text
Notes           ✓
Voicing         ✓
Voice leading   ✓
Articulations   ✓
Dynamics        ~
Humanization    –
Final review    –
```

Некоторые признаки можно будет выставлять вручную, некоторые — определять автоматически.

Пример будущего summary:

```text
TENOR SAX — Chorus

Clip coverage          100%
Musical content         92%
Articulation coverage   68%
Expression coverage     41%

Status: Draft
```

---

## 12. Session History — future

Arranger Manager может сохранять snapshot собственного состояния и сравнивать его между рабочими сессиями.

Пример:

```text
Since previous session:
+ Brass added to Chorus 2
+ Strings Bridge: Draft → Review
+ arrangement extended by 8 bars
+ overall progress 64% → 71%
```

Это должна быть собственная история Arranger Manager, а не предположение, что ARA предоставляет готовый журнал изменений DAW.

---

## 13. Integration with Harma Waves — future

В будущем возможна интеграция с Harma Waves:

```text
Harma Waves
Harmony / Voicing / Voice Leading
        ↕ shared project context
Arranger Manager
Progress / Tasks / Sections / Notes
        ↕
DAW tools
Recording / Capture / Production
```

Потенциальные данные от Harma Waves:

- тип voicing по области;
- range warnings;
- voice-leading warnings;
- harmonic/tension warnings;
- статус музыкальной обработки партии.

Интеграция не должна быть обязательной для базовой работы Arranger Manager.

---

## 14. Предварительная архитектура

```text
Studio Pro
   │
   ├─ ARA Musical Context
   │   ├ Chord
   │   ├ Key
   │   ├ Tempo
   │   └ Time Signature
   │
   ├─ Arrangement Map audio regions
   │   ├ logical name/status
   │   ├ start
   │   ├ duration
   │   └ optional color
   │
   └─ optional Track Agents
       ├ MIDI activity
       ├ CC / articulation activity
       └ track-level progress
            ↓
      Arranger Manager Core
            ↓
      Coverage / Status Engine
            ↓
      Dashboard / Tasks / Notes
```

---

## 15. MVP Direction

MVP должен быть максимально маленьким:

```text
ARA Inspector
    ↓
read service audio regions
    ↓
parse name/status
    ↓
calculate coverage + gaps
    ↓
show simple Dashboard
```

Не начинать с:

- автоматического анализа всех MIDI tracks;
- AI-планировщика;
- сложной базы задач;
- интеграции с Harma Waves;
- session history;
- cloud sync.

Сначала нужно доказать, что Studio Pro стабильно отдаёт нужную region-разметку через ARA или определить рабочий fallback.

---

## 16. Главные технические неопределённости

До начала основной разработки необходимо экспериментально проверить:

- видит ли ARA-инстанция нужные `RegionSequence`;
- создаётся ли usable `PlaybackRegion` для silent audio event;
- доступны ли start и duration;
- можно ли стабильно идентифицировать имя region/event;
- доступен ли color;
- приходят ли live updates после rename/move/resize/recolor;
- видит ли одна центральная ARA-инстанция несколько служебных tracks или потребуется reader/agent на каждом;
- что происходит с muted track/event;
- как ведут себя copy, duplicate, split и overlapping events.

Эти вопросы не являются частью продуктовой гипотезы — это отдельный feasibility layer, который должен быть закрыт Stage 0.

---

## 17. Принцип документации и Issues

Репозиторий должен разделять долговременную документацию и рабочие задачи:

```text
README / docs
→ Product Vision
→ Architecture
→ Accepted design decisions
→ Roadmap

GitHub Issues
→ конкретный Stage
→ конкретный implementation task
→ тест
→ bug / fix
```

Концептуальные идеи не должны жить только в Issues.

---

## Status

**Concept / pre-development.**

Следующий технический шаг: Stage 0 — ARA Inspector / feasibility test для Studio Pro.
