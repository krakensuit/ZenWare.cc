# ZenWare.cc — Internal Cheat for Left 4 Dead 2

**Language / Язык: [English](#english) | [Русский](#russian)**

---

<a id="english"></a>
## English

Internal cheat for Left 4 Dead 2 based on **Lak3/l4d2-internal-base** (x86, C++17, Visual Studio 2022, MinHook).

> **For education and local-server play with `-insecure` only. Using it on VAC servers may get you banned.**

### Features

**Visuals**
- **ESP** — boxes, health bar, names, distance, ground items, common infected
- **Chams** — `UnlitGeneric` `debug/debugambientcube` materials, 5 palettes, through-walls (`MATERIAL_VAR_IGNOREZ`)
- **NoFog / Viewmodel FOV**, custom **crosshair**, **FPS overlay**

**Combat**
- **Aimbot** — silent, `FOV/Distance` priority, `head/center`, smoothing, `bVisibleOnly`
- **TriggerBot**, **AutoShove** (`m_tongueOwner / m_pounceAttacker`), **AutoPistol**

**Movement**
- **BunnyHop** — perfect bhop, `EdgeJump`, `EdgeBug`, `LongJumpHelper`, `FastStop`, `Prestrafe`
- **AutoStrafe** — `legit mousedx / rage circle / w-only`

**System**
- **Menu** — animated RGB `ZenWare.cc` logo, `?` help icons with descriptions, drag by header, tabs, `F11` unload, `INSERT` by default
- **Config** — `<gamedir>\ZenWare.cfg` (`bool/int/float/Color`)
- **Killfeed**, **Logger** (`%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log`)

**Loader**
- Animated intro splash (logo reveal, particles, ESP corners, progress bar)
- `Standard` / `Manual Map` injection, `LAUNCH GAME` via Steam, rainbow button outline
- Dark/light system theme, RU/EN UI based on system language

**External** (`ZenWare.External`) — separate read-only build: RPM + GDI overlay + `SendInput` bhop, nothing injected.

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
- **ESP** — боксы, полоска здоровья, ники, дистанция, предметы на земле, обычные заражённые
- **Chams** — материалы `UnlitGeneric` `debug/debugambientcube`, 5 палитр, сквозь стены (`MATERIAL_VAR_IGNOREZ`)
- **NoFog / Viewmodel FOV**, кастомный **прицел**, **FPS-оверлей**

**Combat**
- **Aimbot** — silent, приоритет `FOV/Distance`, `head/center`, сглаживание, `bVisibleOnly`
- **TriggerBot**, **AutoShove** (`m_tongueOwner / m_pounceAttacker`), **AutoPistol**

**Movement**
- **BunnyHop** — perfect bhop, `EdgeJump`, `EdgeBug`, `LongJumpHelper`, `FastStop`, `Prestrafe`
- **AutoStrafe** — `legit mousedx / rage circle / w-only`

**System**
- **Меню** — анимированный RGB-логотип `ZenWare.cc`, иконки `?` с описаниями, drag за шапку, табы, `F11` выгрузка, `INSERT` по умолчанию
- **Config** — `<gamedir>\ZenWare.cfg` (`bool/int/float/Color`)
- **Killfeed**, **Logger** (`%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log`)

**Лоадер**
- Анимированный интро-сплэш (появление логотипа, частицы, ESP-уголки, прогресс-бар)
- Инжект `Standard` / `Manual Map`, кнопка `ЗАПУСТИТЬ ИГРУ` через Steam, радужная обводка
- Тёмная/светлая тема из системы, язык RU/EN по языку системы

**External** (`ZenWare.External`) — отдельная read-only сборка: RPM + GDI-оверлей + bhop через `SendInput`, ничего не инжектится.

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
