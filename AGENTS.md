# AGENTS.md — ZenWare.cc

Инструкция для ИИ-агента, работающего с этим репозиторием. Только конкретика проекта.

## 1. Контекст проекта

- **Что:** чит для Left 4 Dead 2 (2009, Source engine): internal-DLL (инжект) + GUI-лоадер + external-оверлей без инжекта. Только обучение и локальный сервер с `-insecure`.
- **Стек:** C++17 (`stdcpp17`), MSBuild v143, Windows SDK `10.0.26100.0`, **только x86** (`Release | Win32`), рантайм `/MT`, `/utf-8`, Warning Level 3. Тестов нет.
- **Ключевые библиотеки:** MinHook (внутри `ZenWare.DLL/src/Util/Hook/MinHook/`, менять нельзя), FontAwesome 6 Free (иконки меню), Win32 API + GDI+ (лоадер), WinMM `Beep` (хитсаунд). DirectX вне SDK нет — весь рендер через `MatSystemSurface`.
- **Важно:** `XorString` (`_()`) — заглушка (`#define _(x) x`), строки в бинаре открытые. `l4d2_base.vcxproj` (+ `.filters`) под `src/` — мёртвое наследие v142, не в солюшене, не трогать.
- **Политика:** source-only — никаких `.exe`/`.dll` в гите, релизах и артефактах CI (см. `SECURITY.md`). Готовый билд живёт только в локальной `dist/` (git-ignored).

## 2. Структура и навигация

```text
ZenWare.sln                        3 проекта (DLL + Loader + External)
ZenWare.DLL/                       internal-модуль → bin\Release\ZenWare.dll
  ZenWare.DLL.vcxproj              ЕДИНСТВЕННОЕ место регистрации новых .cpp/.h DLL
  src/Entry/Entry.cpp              точка входа: лог → паттерны → интерфейсы → хуки
  src/DllMain.cpp                  только CreateThread(InitThread), никакой логики
  src/Features/Vars.h              ВСЕ настройки — plain inline-глобалы по namespace
  src/Features/Config/             key=value конфиг (<gamedir>\ZenWare[2|3].cfg)
  src/Features/Menu/               кастомное immediate-mode меню (320x480, табы, скролл)
  src/Features/Lang/Lang.h         RU/EN таблица (EN-строка = ключ!)
  src/Features/{Aimbot,ESP,...}/   фичи: Run/OnTick (из CreateMove) + Draw/Render (из Paint)
  src/Hooks/                       детуры: ClientMode::CreateMove, EngineVGui::Paint, WndProc, ...
  src/SDK/                         минимальный реверсинг-слой L4D2 (сущности, интерфейсы, математика)
  src/Util/Offsets|Pattern/        15 сигнатур + кэш ZenWare.offsets
  ZenWare.DLL/external/Icons/      subset FontAwesome (UTF-8!), подключён из vcxproj
ZenWare.Loader/                    GUI-лоадер → bin\Release\ZenWare.Loader.exe
  Main.cpp (~950 строк)            сплэш, кнопки, темы; версия ТОЛЬКО из resource.h
  resource.h                       ZENWARE_VER_STR — бамп версии = править здесь
ZenWare.External/src/              external: Memory/Overlay/ESP/Movement/Offsets/Resolve
Tools/SigScan/                     отдельный .vcxproj (НЕ в .sln!): проверка 15 паттернов
.github/workflows/build.yml       CI: msbuild Release|x86 + проверка размера, без выгрузки
dist/                              локальный выход Build-SingleFile.ps1 (в гите нет)
```

## 3. Сборка и проверка (команды)

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1  # всё + dist\ZenWare.exe
.\Verify-Signatures.bat              # паттерны против локальных DLL игры (0 = ок, 2 = чинить Offsets.cpp)
msbuild ZenWare.sln -p:Configuration=Release -p:Platform=x86 -m -v:m -nologo
```

- Новый `.cpp/.h` в DLL: добавить `ClCompile`/`ClInclude` в `ZenWare.DLL.vcxproj` (рядом с соседями), иначе CI упадёт с LNK, а локально всё соберётся.
- Диагностика вылета: хвост `<gamedir>\left4dead2\ZenWare.log` (не `%TEMP%` — туда только старт), строки `[!!!] EXCEPTION ... (модуль+смещение)` и `[!!!] last breadcrumb`.
- CI гоняет каждый пуш (~2 мин). Локального MSBuild может не быть — тогда только CI.

## 4. Правила кода (как здесь принято)

- **Стиль:** табы, Allman-скобки, `.editorconfig` + `.clang-format`. Русский в комментариях — ок, файл в UTF-8.
- **Fail-closed, не fail-crash:** любой отсутствующий паттерн/интерфейс/указатель — ранний `return`/`false`, никаких разыменований «авось». Образец: `Entry.cpp` (таблица `aNeed` + abort с MessageBox), `Hook.h` (`Init(nullptr) → false`).
- **Netvars вместо виртуалок:** слоты vtable в дампе не проверены — `GetEyePosition()` через `m_vecOrigin()+m_vecViewOffset()`, см. `GameUtil.cpp`. Новую виртуалку без сверки со свежим бинарником не добавлять.
- **Касты сущностей:** `As<T>()` защищает только от `nullptr`, НЕ от чужого класса. Перед виртуалкой/нетваром проверять `GetClientClass()->m_ClassID` (урок: `ESP::DrawItem` падал на таблетках — `CWeaponSpawn`-only ветка обязательна).
- **Ивентов движка нет by design:** HP/смерти — только опросом каждый кадр (образец: `Killfeed::OnTick`, `Hitmarker::OnTick`). `IGameEventManager` не подключать.
- **Конфиг:** только `bool/int/float/Color`. `float`-слайдеру нужен int-прокси (`nFOVSlider` → `flFOV`): синхронизация в `Menu::Render` + ресинк в лямбде `Load config` (`Menu.cpp`). Ключ вида `aimbot.fov`, слот конфига — сессионный, не персистить.
- **Меню:** подпись виджета = EN-строка; русскую добавить в `Lang.h`, иначе покажется английский (не ошибка). Хелп-`?` опционален. Вкладки фиксированы: Visuals/Move/View/Combat/Misc — строки добавляются в `case N:` в `Render()`, скролл и клиппинг уже есть.
- **Рисование:** только через `G::Draw` (`String/Line/Rect/OutlinedRect/OutlinedCircle`), координаты — `int`, строки формата — всегда литералы (`L"%ls"`, не переменные). `wsprintfW` запрещён, только `*printf_s`.
- **Память:** STL (`map/vector/string`) разрешён; сырой `new` — только там, где уже есть; `strncpy_s` трёхаргументный (шаблонный) — ок. `trace_t`/`player_info_t` всегда `= {}`.
- **Потоки:** хот-пути однопоточные (CreateMove/Paint). Фон — только короткие detached-треды (звук) и `CreateThread` лоадера. Никаких блокировок в детурах.
- **Лоадер:** никакого сетевого кода (автоапдейтер удалён сознательно). Кнопка «обновления» — только `ShellExecute` на страницу релизов.

## 5. Реверс-инжиниринг (практика проекта)

- **Паттерны:** IDA-стиль `55 8B EC ... ? ?`, скан только `.text` (`Pattern.cpp`). Адрес в `Offsets.cpp` = match ± константа (`+0x3`, `-0x1B`); **та же арифметика обязана совпасть в `Tools/SigScan/Main.cpp`** (`m_nAdjust`), иначе тул врёт.
- **RVA vs abs:** DLL хранит абсолютные адреса, SigScan печатает `base+RVA`, кэш `ZenWare.offsets` — RVA + fingerprint (размер+время записи) трёх DLL. После обновы игры: `Verify-Signatures.bat` → чинить только строки `NOT FOUND`/`AMBIGUOUS` (первое совпадение молча бьёт мимо — как было с `CheckForSequenceChange`, он теперь захардкоженно выключен в `Hooks.cpp`).
- **Документировать находки** в коде коротким комментом `//` с причиной (пример: «бумера нет в дампе — фолбэк по имени»), крупные изменения — строкой в `CHANGELOG.md` (Unreleased, EN+RU однострочники).
- **Инструменты реверса:** x64dbg/IDA для сверки кандидатов (особенно ambiguous-матчи), Ghidra — приемлемо. Локальные DLL игры: `<Steam>\steamapps\common\Left 4 Dead 2\left4dead2\bin\`.

## 6. Правила инструментов ИИ

- **filesystem (read/glob/grep):** дефолт для всего — сначала читать файлы самому, выводы без локальных проверок не писать. За пределы корня можно только читать: `%TEMP%\ZenWare*.log`, `left4dead2\ZenWare.log`, DLL игры (read-only). Писать в папку игры и Steam — запрещено (кроме запуска сборки).
- **context7:** почти не применим здесь (Source SDK и Win32 там нет или устарели). Вызывать только для сторонних библиотек общего назначения (н-р, STL/`std::thread` нюансы MSVC), никогда — для движка/оффсетов.
- **sequential-thinking:** обязательно перед правками, затрагивающими ≥3 файлов, новую механику прицеливания/движения, или когда причина вылета не видна сразу. Для одиночных правок (текст, конфиг, меню-строка) — не нужен.
- **memory:** сохранять только долгоживущее: архитектурные решения («ивентов нет by design»), проверенные смещения/версии SDK, грабли («первое совпадение паттерна бьёт мимо», «CWeaponSpawn-only в DrawItem»). НЕ сохранять: адреса под конкретный билд игры (протухают), содержимое логов, временные статусы.
- **task-субагенты:** слать параллельный разбор независимых подсистем (н-р, ESP-рендер и NoSpread) перед большим фичем; запрещать им править файлы (только отчёт с `file:line`).
- **bash:** только терминал (git/msbuild/gh/winget/реестр/логи). Не использовать для чтения/правок файлов и как чат с пользователем. После правок C++ — обязательно `git push` и дождаться CI (`conclusion == success`), потом пересобрать локальный `dist\ZenWare.exe`, если пользователь его запускает.
