# AGENTS.md — ZenWare.cc

Чит для Left 4 Dead 2 (Source, 2009): internal-DLL + GUI-лоадер + external-оверлей. Только обучение и локальный сервер с `-insecure`.

## Стек и сборка

- C++17 (`stdcpp17`), MSBuild v143, SDK `10.0.26100.0`, **только `Release | Win32`**, `/MT`, `/utf-8`, W3. Тестов нет.
- `ZenWare.sln` = 3 проекта (DLL + Loader + External). `Tools/SigScan` — отдельный `.vcxproj`, **не** в солюшене.
- Локально MSBuild есть (BuildTools 2022), но гейт — CI (`build.yml`: msbuild + проверка, что лоадер ≥2 МБ, без выгрузки артефактов).

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1  # всё + dist\ZenWare.exe
.\Verify-Signatures.bat              # 0 = паттерны ок, 2 = чинить Offsets.cpp
```

## Карта (неочевидное)

- `ZenWare.DLL/src/Entry/Entry.cpp` — порядок инициализации: лог → паттерны → интерфейсы → хуки. `DllMain.cpp` — только `CreateThread`, логики нет.
- `ZenWare.DLL/src/Features/Vars.h` — ВСЕ настройки (inline-глобалы по namespace). `Config/` — `key=value` (`aimbot.fov`), слот конфига сессионный.
- `ZenWare.DLL/src/Features/Menu/` — своё immediate-меню 320x480, табы Visuals/Move/View/Combat/Misc (`case N:` в `Render()`), скролл/клиппинг есть. Подпись виджета = EN-строка, перевод — в `Lang.h`.
- `ZenWare.DLL/src/Features/*/` — фичи: `Run/OnTick` из CreateMove, `Draw/Render` из Paint.
- `ZenWare.DLL/src/Util/Offsets|Pattern/` — 15 сигнатур; `ZenWare.DLL.vcxproj` — **единственное** место регистрации новых `.cpp/.h` (иначе LNK только на CI).
- `ZenWare.DLL/external/` — MinHook (не трогать) + subset FontAwesome (UTF-8).
- `ZenWare.Loader/resource.h` — `ZENWARE_VER_STR`, единственное место версии. Сетевого кода нет (апдейтер удалён).
- `l4d2_base.sln/.vcxproj/.filters` под `src/` — мёртвое наследие v142, не собирать.
- `XorString _()` — заглушка (`#define _(x) x`). Source-only: билды только в git-ignored `dist/`, никогда в git/релизы/CI.

## Правила кода

- Табы, Allman, `.editorconfig` + `.clang-format`; русский в комментах ок.
- Fail-closed: отсутствующий паттерн/интерфейс/указатель = ранний `return`/`false`. Образец — таблица `aNeed` + abort в `Entry.cpp`, `Init(nullptr) → false` в `Hook.h`.
- Только netvars, не виртуалки (`GetEyePosition()` = `m_vecOrigin()+m_vecViewOffset()`). `As<T>()` защищает лишь от `nullptr` — перед виртуалкой/нетваром проверять `GetClientClass()->m_ClassID` (падали на таблетках: ветка `CWeaponSpawn`-only обязательна).
- Ивентов движка нет by design (`Killfeed::OnTick` — опрос, не `IGameEventManager`).
- `float`-слайдеру нужен int-прокси + синхронизация в `Render()` и ресинк в `Load config`.
- Рисование только через `G::Draw`, строки формата — литералы (`L"%ls"`), `wsprintfW` запрещён. `trace_t`/`player_info_t` всегда `= {}`. STL ок, блокировок в детурах нет, фон — короткие detached-треды.

## Реверс

- Паттерны IDA-стиля, скан только `.text`; адрес = match ± константа — **та же арифметика обязана совпасть в `Tools/SigScan/Main.cpp`** (`m_nAdjust`).
- DLL хранит абсолютные адреса, SigScan печатает `base+RVA`, кэш — RVA + fingerprint DLL. Чинить только `NOT FOUND`/`AMBIGUOUS` (первое совпадение бьёт мимо молча — так было с `CheckForSequenceChange`, он выключен в `Hooks.cpp`).
- Находки — короткий `//` с причиной + строка в `CHANGELOG.md` (Unreleased, EN+RU). DLL игры: `<Steam>\...\Left 4 Dead 2\left4dead2\bin\` (только чтение).

## Инструменты

- Сначала читать файлы самому (read/glob/grep); за корень — только чтение логов и DLL игры, писать туда запрещено.
- context7 — только для общих библиотек (STL/MSVC), никогда для движка/оффсетов.
- sequential-thinking — перед правками ≥3 файлов, новой механикой аима/движения, неясным вылетом. Для мелочей не нужен.
- memory — только долгоживущее (архитектура, грабли типа «первое совпадение мимо»); не сохранять адреса под билд, логи, статусы.
- task-субагенты — параллельный разбор независимых подсистем, только отчёты с `file:line`, без правок.
- bash — только терминал. После правок C++: `git push` → дождаться CI success → пересобрать `dist\ZenWare.exe`, если пользователь его запускает.
- Диагностика вылета: `<gamedir>\left4dead2\ZenWare.log`, строки `[!!!] EXCEPTION` и `[!!!] last breadcrumb`.
