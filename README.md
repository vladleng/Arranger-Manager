# Arranger Manager

Самостоятельное рабочее пространство экосистемы MR: страницы и задачи, план дня, календарь, Timeline, аналитика и GPT. **Решение 2026-10-28: связь со Studio Pro упразднена; сначала интеграция с MR Live, затем MR Studio.** Название продукта пока сохраняется.

[Решение и изученные ассеты MR](docs/MR-ECOSYSTEM.md) · [ROADMAP](docs/ROADMAP.md) · [HANDOFF](docs/HANDOFF.md) · [Модель](docs/WORKSPACE-ARCHITECTURE.md)

## Состояние

- Принятые 0.1e–0.1g находятся в main: устойчивые ID, хранилище и миграция, страницы/навигация/архив, блочный редактор.
- 0.1h — кандидат [PR #46](https://github.com/vladleng/Arranger-Manager/pull/46): общие задачи и подробные описания, раскрытие задач/чек-поинтов и меню по ПКМ. [Build #74](https://github.com/vladleng/Arranger-Manager/actions/runs/37249113209) ранее прошла 8/8 наборов; пользовательская приёмка ожидается. Кандидат ещё не слит.
- MR-дизайн/адаптеры сейчас **планируются**, не реализованы. Старый Studio Pro reader/Hub/Inspector остаётся в коде до отдельного вывода зависимости #7; `.show` исследование #19 отменено. Смена плана не удаляет старые задачи, Notes, backups или пользовательские исходники.

## Будущая работа

| Направление | Задача |
| --- | --- |
| Общее пространство и приёмка 0.2 | [#37](https://github.com/vladleng/Arranger-Manager/issues/37) |
| Дизайн MR Live / MR UDS Manager Kit | [#47](https://github.com/vladleng/Arranger-Manager/issues/47) |
| Первый контракт и адаптер MR Live (новый 0.1i) | [#48](https://github.com/vladleng/Arranger-Manager/issues/48) |
| Затем MR Studio на том же слое источников | [#49](https://github.com/vladleng/Arranger-Manager/issues/49) |
| План дня 0.3, календарь 0.4, Timeline 0.5 | [#38](https://github.com/vladleng/Arranger-Manager/issues/38), [#39](https://github.com/vladleng/Arranger-Manager/issues/39), [#40](https://github.com/vladleng/Arranger-Manager/issues/40) |
| Аналитика 0.6 и GPT 0.7 | [#41](https://github.com/vladleng/Arranger-Manager/issues/41), [#42](https://github.com/vladleng/Arranger-Manager/issues/42) |

[Общий план #36](https://github.com/vladleng/Arranger-Manager/issues/36). Обычные задачи работают без Live/Studio. Связи с музыкальными источниками используют стабильные ID и явное владение, без копий задач. Проценты, статусы и сохранение различаются у источников; контракт выбирается до записи данных. Read-only snapshot не равен двусторонней интеграции.

## Ассеты и дизайн

Общая библиотека — `assets/ui/mr-uds/` в [MR-Studio](https://github.com/vladleng/MR-Studio), в том числе Manager SVG Kit v1: 46 SVG, семантические токены и каталог. [MR Live reference](https://vladleng.github.io/MR-Studio/tools/design-lab/live/) задаёт графитовый компактный стиль и workflow. Kit и native Live есть в тематических ветках, не полностью в main; точные sources и границы описаны в [MR-ECOSYSTEM](docs/MR-ECOSYSTEM.md). Наличие SVG не означает внедрение в приложение.

## Сборка существующей базы

CMake 3.22+, C++20, JUCE 9.0.2 через FetchContent. Main содержит принятую 0.1g; функциональный кандидат 0.1h — в отдельной ветке. Для main:

```sh
cmake -S . -B build
cmake --build build --config Release --target ArrangerManagerDesktop ArrangementTagTests SongSnapshotTests SongCatalogTests WorkspaceTests WorkspaceNavigationTests BlockEditorTests --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Существующий Hub VST3 и опциональный ARA Inspector — legacy цели, не обязательная архитектура будущей MR-связи. Docs-only актуализация 2026-10-28 не требует сборки.

Документы `HYBRID-*`, `HUB-*`, `CONTEXT-BRIDGE-*` и ранние ARA checks описывают исторические версии Studio Pro направления. Они не являются действующим заданием на развитие `.song`/`.show`/ARA интеграции.
