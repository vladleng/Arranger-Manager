# Arranger Manager — передача контекста

**Срез на 2026-09-28, приложение 0.1.** Начните с [README](../README.md), затем с этого файла. [CONCEPT](CONCEPT.md) фиксирует действующие решения, [ROADMAP](ROADMAP.md) — следующие этапы. Документы версий `HUB-0.0*.md` и ARA относятся к истории; изменения кода и состояние Issues/PR нужно проверить в GitHub перед началом новой работы.

## Что пользователь хочет получить

Отдельное Windows-приложение для обзора проектов Fender Studio Pro в стиле дерева задач: папки → песни → треки → клипы. Секции аранжировки окрашивают метки клипов, заметки дорожек находятся в Notes, типы audio/MIDI имеют значки. Во время работы в DAW пользователь правит клипы и заметки там; приложение читает сохранённый `.song`. Для закрытого проекта в будущем планируется безопасное обратное редактирование. Не требовать ARA-вставку на каждом событии или служебный трек.

## Текущее поведение 0.1

| Объект | Источник и способ изменения | Что показано |
| --- | --- | --- |
| Клип | Имя обычного `AudioEvent`/`MusicPart` в сохранённом `.song`; меняется в Studio Pro и становится видимым после Save | Статус из имени; текст после `|` в Notes; Type и метка пересекающей секции |
| Трек | Имя, цвет, тип, заметка и события из `.song`; статус выбирается вручную в приложении | Кружок цвета, Type, Status, `PROG = DONE / все клипы`, Notes |
| Песня | `.song`; статус выбирается вручную в приложении | Info/Notes, Status и `DONE / все клипы` |
| Папка | Локальный каталог | Группировка песен, суммарный PROG |

Статусы трека и песни **не наследуются** друг от друга и не следуют автоматически из PROG. Они вместе с папками и путями сохраняются в локальном JUCE `ApplicationProperties` каталоге, не в проекте. Песня может быть WAIT при треке WIP. Клип без распознанного статуса входит в знаменатель PROG как незавершённый. Пункты меню Status: POOL, TODO, WIP, DRAFT, WAIT, DONE, BLOCKED и Clear status. Исторический `REVIEW` распознаётся как WAIT; `P1`–`P4` в старых именах клипов игнорируются UI. Приоритет песни пока отсутствует.

Раскрытие песни, Tracks/Markers и трека в 0.1 управляется в памяти UI. После нового запуска вложенные группы закрыты; папки каталога по умолчанию открыты. Файл отсутствующего проекта остаётся в каталоге с ошибкой чтения. Обновление снимка проверяется по времени изменения и размеру файла примерно раз в секунду; это не live состояние DAW и не полноценное обнаружение смены содержимого при одинаковых времени и размере.

## Код и границы

- `src/SongSnapshot.cpp/.h`: read-only ZIP/XML reader для `metainfo.xml`, `notes.txt`, `notepad.xml`, `Song/song.xml`. Читает `MediaTrack`, обычные `AudioEvent` и `MusicPart`, `ArrangerTrack` и `MarkerTrack`. Нормализует unbound `x:id` только при чтении. XML схемы тестировались на Studio Pro 8.1.2; другие версии требуют проверки.
- `src/ArrangementTag.h`: разбор имени клипа и совместимость REVIEW/старого Pn. `src/StudioProColour.h`: конвертация сохранённого цвета Studio Pro из ABGR для JUCE.
- `src/SongCatalog.h`: локальный каталог (`version: 1` JSON), папки, песни и ручные статусы по пути песни и `trackID` (fallback: индекс трека). Старый 0.0m JSON без статусов читается. Удаление песни из каталога удаляет её локальные статусы треков, но не исходный `.song`.
- `src/HubEditor.cpp/.h`: общий JUCE UI для настольного приложения и Hub VST3; настольная версия получает каталог, VST3 работает как просмотр одного проекта. Здесь находятся PROG, цветные кружки, дерево и обновление снимков.
- `src/HubApplication.cpp`: Windows окно, сохранение каталога и выбранного проекта через `ApplicationProperties` в пользовательских настройках Moon River Studio.
- `src/HubProcessor.cpp`: обычный VST3 Hub. ARA исследование в `src/PluginProcessor.cpp`, `InspectorDocumentController.*`; по умолчанию ARA target выключен.
- `tools/read_song_metadata.py`, `tools/read_song_inventory.py`, `scripts/`: диагностические средства, не реализация записи `.song`.

Есть ограничения текущего reader: временные атрибуты берутся как сырые значения, сопоставление клипа с секцией опирается на эти значения; нестандартные takes/layers и другие версии файла не подтверждены. Совпадающий `clipID` после split/copy нельзя использовать как уникальный ключ для будущей записи. В 0.1 нет записи в `.song`, импорта `.show`, приоритета песен, поиска и анализа coverage/gaps.

## Сборка и проверки

```sh
cmake -S . -B build
cmake --build build --config Release --target ArrangerManagerDesktop ArrangerManagerHub_VST3 ArrangementTagTests SongSnapshotTests SongCatalogTests --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

CMake 3.22+, C++20, JUCE 9.0.2 через FetchContent. `.github/workflows/windows-build.yml` пакует только `.exe` и `docs/HUB-0.1.md`. Отдельный ARA Inspector включается `-DARRANGER_BUILD_ARA_INSPECTOR=ON` с ARA SDK 2.3.0, в текущем Windows CI он не собирается. [Windows Build #35](https://github.com/vladleng/Arranger-Manager/actions/runs/36397309647) для 0.1 прошёл компиляцию и все три набора тестов; ZIP `Arranger-Manager-0.1-Windows-App`. Пользовательская визуальная проверка именно 0.1 пока не зафиксирована.

## GitHub и следующие действия

- 0.1: [PR #20](https://github.com/vladleng/Arranger-Manager/pull/20), ветка `hub-stage2-manual-status`, поверх ветки `hub-catalog-folders-progress` ([PR #16](https://github.com/vladleng/Arranger-Manager/pull/16)). Предшествующие UI PR тоже могут быть draft; не считайте `main` содержащей 0.1 без проверки истории веток.
- [#17](https://github.com/vladleng/Arranger-Manager/issues/17) — пункты этапа 2 реализованы в PR #20; остаётся пользовательская проверка и решение о закрытии после merge.
- [#7](https://github.com/vladleng/Arranger-Manager/issues/7) — каталог частично реализован; поиск, приоритет песни, масштабирование и переносимость остаются.
- [#8](https://github.com/vladleng/Arranger-Manager/issues/8) — следующий технический этап: безопасная запись **закрытой копии** `.song`, проверка в Studio Pro прежде чем трогать оригинал.
- [#19](https://github.com/vladleng/Arranger-Manager/issues/19) — исследование `.show`, сначала read-only на тестовом файле.

Перед изменениями проверить актуальное состояние этих Issues, PR и CI. После пользовательского теста 0.1 устранить найденные ошибки, затем продолжать по согласованному этапу. Не предполагать наличие схемы `.show` или безопасной записи `.song` только по результатам read-only parser.
