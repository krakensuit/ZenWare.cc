# ZenWare.cc — Internal Cheat for Left 4 Dead 2

**Language / Язык: [English](#english) | [Русский](#russian)**

---

<a id="english"></a>
## English

Internal cheat for Left 4 Dead 2 based on **Lak3/l4d2-internal-base** (x86, C++17, Visual Studio 2022, MinHook).

> **For education and local-server play with `-insecure` only. Using it on VAC servers may get you banned.**

### Features

**Visuals**
- **ESP** — boxes, health bar, names, distance, ground items, commons, all SI incl. boomer (class-name fallback survives ID shifts on foreign builds)
- **Chams** — 5 palettes, allies/enemies/all SI, through-walls
- **NoFog / FOV** (world + viewmodel) / **thirdperson**, custom **crosshair**, **FPS overlay**, **killfeed**, **2D radar**, **spectators**, **tank/witch alerts**
- **Menu** — RU/EN toggle (button + `F7`), animated RGB `ZenWare.cc` logo, `?` help icons in both languages, tabs, `F11` unload, `INSERT` by default
- **Config** — `<gamedir>\ZenWare.cfg` (`bool/int/float/Color`)
- **Logger** (`%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log`, per-pid fallback)

**Loader**
- Single-file build: DLL + External + logo embedded, extracted to `%TEMP%` on demand (`dist\ZenWare.exe` via `Build-SingleFile.ps1`)
- Animated intro splash, `Standard` injection, `LAUNCH GAME` via Steam, rainbow button outline
- Dark/light theme, RU/EN toggle (saved), **auto-updater** from GitHub Releases

**External** (`ZenWare.External`) — read-only: RPM + GDI overlay + `SendInput` bhop/strafe/stats, runtime offset resolver (signatures + anchor + on-screen diag), RU/EN (`F7`), nothing injected.

### Layout

```
ZenWare.cc/
├── ZenWare.sln
├── ZenWare.DLL/        → bin\Release\ZenWare.dll (x86, /MT)
├── ZenWare.Loader/     → bin\Release\ZenWare.Loader.exe
├── ZenWare.External/   → bin\Release\ZenWare.External.exe
└── Tools/SigScan/      pattern verifier for current game binaries
```

### Build

1. Visual Studio 2022 + `Desktop development with C++` + `Windows SDK 10.0.26100.0`
2. `ZenWare.sln` → `Release | Win32` → `Rebuild`

> The loader embeds the DLL as `RCDATA` (`resource.rc` `IDR_ZENWARE_DLL`) — one `exe` is enough to share.

### Usage

1. Steam → L4D2 → launch options: `-insecure -windowed -console`
2. In game: `map c1m1_hotel` (local server)
3. Run `ZenWare.Loader.exe` **as administrator** → `INJECT`
4. In game `INSERT` — menu, `F11` — unload
5. Keybinds: `Misc → Menu key / Aimbot key` (click → `[press key]` → press a key, `ESC` = off)

Defaults: hold `Space` for BunnyHop, `MOUSE4` for Aimbot.

### Troubleshooting

- `XorString` MessageBox on start — a pattern died after a game update → run `Tools\SigScan\bin\SigScan.exe` on `left4dead2\bin\client.dll` and update `Offsets.cpp`
- Crash after `Paint` — check `[!!!] EXCEPTION` in the log, send `module+0x...`
- Antivirus flags the loader — exclude the project folder (heuristic on `WriteProcessMemory/CreateRemoteThread`)

### Credits

- Base: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
- Hook engine: [TsudaKagecha/MinHook](https://github.com/TsudaKagecha/minhook)
- Icons: [FontAwesome 6 Free](https://fontawesome.com)
- ZenWare is an educational project, not for VAC servers.

---

<a id="russian"></a>
## Русский

Internal-чит для Left 4 Dead 2 на базе **Lak3/l4d2-internal-base** (x86, C++17, Visual Studio 2022, MinHook).

> **Только для обучения и игры на локальном сервере с `-insecure`. Использование на VAC-серверах может привести к блокировке.**

### Возможности

**Visuals**
- **ESP** — боксы, полоска HP, ники, дистанция, предметы, обычные, все СИ включая бумера (фолбэк по именам переживает сдвиг ID на чужих билдах)
- **Chams** — 5 палитр, союзники/враги/все СИ, сквозь стены
- **NoFog / FOV** (мир + модель) / **3-е лицо**, кастомный **прицел**, **FPS-оверлей**, **киллфид**, **2D-радар**, **наблюдатели**, **алерты танка/ведьмы**
- **Меню** — тоггл RU/EN (кнопка + `F7`), анимированный RGB-логотип `ZenWare.cc`, иконки `?` с описаниями на двух языках, табы, `F11` выгрузка, `INSERT` по умолчанию
- **Config** — `<gamedir>\ZenWare.cfg` (`bool/int/float/Color`)
- **Logger** (`%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log`, per-pid фолбэк)

**Combat**
- **Aimbot** — silent, приоритет `FOV/Distance`, `head/center`, сглаживание, только видимые, обычные + все СИ (ID + фолбэк по именам)
- **TriggerBot**, **AutoShove**, **AutoPistol**, **NoSpread** (отключаемый)

**Movement**
- **BunnyHop** — perfect/legit, `EdgeJump`, `EdgeBug`, `JumpBug`, `LongJump`, `FastStop`, `Prestrafe`, `AutoDuck`, `JumpStats`, `SpeedHUD`
- **AutoStrafe** — `legit / rage / w-only / directional`

**System**
- **Меню** — анимированный RGB-логотип `ZenWare.cc`, иконки `?` с описаниями, drag за шапку, табы, `F11` выгрузка, `INSERT` по умолчанию
- **Config** — `<gamedir>\ZenWare.cfg` (`bool/int/float/Color`)
- **Killfeed**, **Logger** (`%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log`)

**Лоадер**
- Однофайловый билд: DLL + External + лого внутри, распаковка в `%TEMP%` по требованию (`dist\ZenWare.exe` через `Build-SingleFile.ps1`)
- Анимированный интро-сплэш, инжект `Standard`, кнопка `ЗАПУСТИТЬ ИГРУ` через Steam, радужная обводка
- Тёмная/светлая тема, тоггл RU/EN (сохраняется), **автоапдейтер** с GitHub Releases

**External** (`ZenWare.External`) — read-only: RPM + GDI-оверлей + bhop/strafe/stats через `SendInput`, runtime-резолв оффсетов (сигнатуры + якорь + диагностика на экране), RU/EN (`F7`), ничего не инжектится.

### Структура

```
ZenWare.cc/
├── ZenWare.sln
├── ZenWare.DLL/        → bin\Release\ZenWare.dll (x86, /MT)
├── ZenWare.Loader/     → bin\Release\ZenWare.Loader.exe
├── ZenWare.External/   → bin\Release\ZenWare.External.exe
└── Tools/SigScan/      верификатор паттернов на актуальных бинарях
```

### Сборка

1. Visual Studio 2022 + `Desktop development with C++` + `Windows SDK 10.0.26100.0`
2. `ZenWare.sln` → `Release | Win32` → `Rebuild`

> Лоадер содержит DLL как `RCDATA` (`resource.rc` `IDR_ZENWARE_DLL`) — для отправки другу достаточно одного `exe`.

### Использование

1. Steam → L4D2 → параметры запуска: `-insecure -windowed -console`
2. В игре: `map c1m1_hotel` (локальный сервер)
3. Запустить `ZenWare.Loader.exe` **от имени администратора** → `ИНЖЕКТ`
4. В игре `INSERT` — меню, `F11` — выгрузка
5. Бинды: `Misc → Menu key / Aimbot key` (клик → `[press key]` → нажми клавишу, `ESC` = off)

По умолчанию: `Space` — BunnyHop, `MOUSE4` — Aimbot.

### Troubleshooting

- `XorString` MessageBox при старте — паттерн умер после обновления игры → запусти `Tools\SigScan\bin\SigScan.exe` на `left4dead2\bin\client.dll` и обнови в `Offsets.cpp`
- Краш после `Paint` — смотри `[!!!] EXCEPTION` в логе, пришли `module+0x...`
- Антивирус ругается на лоадер — добавь папку проекта в исключения Защитника (эвристика на `WriteProcessMemory/CreateRemoteThread`)

### Credits

- База: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
- Хук-движок: [TsudaKagecha/MinHook](https://github.com/TsudaKagecha/minhook)
- Иконки: [FontAwesome 6 Free](https://fontawesome.com)
- Проект ZenWare — учебный, не для VAC-серверов.
