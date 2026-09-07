# ZenWare.cc

Internal + External cheat for **Left 4 Dead 2** (x86, C++17, Visual Studio 2022, MinHook). Current version: **v3.5**.

[![build](https://github.com/krakensuit/ZenWare.cc/actions/workflows/build.yml/badge.svg)](https://github.com/krakensuit/ZenWare.cc/actions)
[![license](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![game](https://img.shields.io/badge/game-L4D2-blue.svg)](https://store.steampowered.com/app/550/Left_4_Dead_2/)
[![platform](https://img.shields.io/badge/platform-Win32%20x86-lightgrey.svg)](#)

**Language / Язык: [English](#english) · [Русский](#russian)**

> ⚠️ **Education and local servers only.** Launch the game with `-insecure` and play on your own maps (e.g. `map c1m1_hotel`).
> Using this on VAC-secured servers can get your account banned. The authors are not responsible for bans.
>
> ⚠️ **Только обучение и локальный сервер.** Запускай игру с `-insecure` и играй на своих картах (например `map c1m1_hotel`).
> Игра на VAC-серверах = бан. Авторы не несут ответственности.

---

<a id="english"></a>
## English

### About

ZenWare.cc is based on [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) and contains 3 parts:

- **ZenWare.DLL** — internal cheat (injected into the game).
- **ZenWare.Loader** — GUI loader: injection, single-file build, theme, auto-updater from GitHub Releases.
- **ZenWare.External** — read-only external overlay (no injection): RPM + GDI + `SendInput`.

### Features

**Visuals**

- ESP: boxes, health bar, names, distance, weapons/items, commons, all Special Infected (incl. Boomer, class-name fallback for foreign builds)
- Chams: 5 palettes, allies / enemies / all SI, through walls
- NoFog, world + viewmodel FOV, thirdperson, custom crosshair, FPS overlay
- 2D radar, spectator list, Tank / Witch alerts, killfeed

**Combat**

- Aimbot (silent, FOV / Distance priority, head / center, smoothing, visible-only, commons + all SI)
- TriggerBot, AutoShove, AutoPistol, NoSpread (toggleable)

**Movement**

- BunnyHop (perfect / legit), EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- AutoStrafe: legit / rage / w-only / directional

**Menu / System**

- `INSERT` menu, `F11` unload, RU/EN toggle (button + `F7`), animated RGB logo, `?` help icons, tabs
- Config: `<gamedir>\ZenWare.cfg` (`bool / int / float / Color`)
- Logger: `%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log` (per-pid fallback)

**Loader**

- Single-file build: DLL + External + logo embedded (`dist\ZenWare.exe` via `Build-SingleFile.ps1`)
- Animated splash, `Standard` injection, `LAUNCH GAME` via Steam, dark / light theme, saved RU/EN, auto-updater

### Requirements

- Windows 10/11 x64, Steam + Left 4 Dead 2
- For build: Visual Studio 2022 + `Desktop development with C++` + `Windows SDK 10.0.26100.0`
- Config: `Release | Win32` (x86)

### Quick start

**Option A — single file (easiest):**

1. Build once: `powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1` → get `dist\ZenWare.exe`.
2. Steam → L4D2 → launch options: `-insecure -windowed -console`.
3. In game console: `map c1m1_hotel`.
4. Run `dist\ZenWare.exe` **as administrator** → `INJECT`.

**Option B — from source:**

1. Open `ZenWare.sln` → `Release | Win32` → `Rebuild`.
2. Run `ZenWare.Loader\bin\Release\ZenWare.Loader.exe` as admin → `INJECT`.

**Option C — external only (no injection):**

1. Start the game as above.
2. Run `ZenWare.External\bin\Release\ZenWare.External.exe`, or double-click `START-ZenWare.bat`.

### Hotkeys

| Key | Action |
| --- | ------ |
| `INSERT` | Open / close menu |
| `F11` | Unload cheat |
| `F7` | Switch language RU / EN |
| `Space` (hold) | BunnyHop (default) |
| `MOUSE4` (hold) | Aimbot (default) |

Menu key and Aimbot key can be rebound in `Misc → Menu key / Aimbot key` (click → `[press key]` → press a key, `ESC` = off).

### Project structure

```text
ZenWare.cc/
├── ZenWare.sln
├── ZenWare.DLL/        internal cheat → bin\Release\ZenWare.dll (x86, /MT)
│   └── src/            Entry, Features, Hooks, SDK, Styles, Util
├── ZenWare.Loader/     GUI loader → bin\Release\ZenWare.Loader.exe
├── ZenWare.External/   external overlay → bin\Release\ZenWare.External.exe
│   └── src/            ESP, Memory, Movement, Offsets, Overlay, Resolve
├── Tools/SigScan/      pattern verifier for current client.dll
├── .github/workflows/  CI build (Release | Win32)
├── dist/               single-file output (ZenWare.exe + KAK-ZAPUSTIT.txt)
├── Build-SingleFile.ps1
├── START-ZenWare.bat
├── patch_theme.ps1
├── README.md
└── LICENSE
```

### Troubleshooting

| Symptom | Fix |
| ------- | --- |
| `XorString` MessageBox on start | A signature broke after a game update. Run `Tools\SigScan\bin\SigScan.exe` on `left4dead2\bin\client.dll` and update `Offsets.cpp`. |
| Crash after `Paint` | Open the log, find `[!!!] EXCEPTION`, report `module+0x...`. |
| Antivirus flags the loader | Add the project folder to exclusions (heuristic on `WriteProcessMemory` / `CreateRemoteThread`). |
| Squares instead of letters | Press `F7` (font fallback / language switch). |
| External shows no boxes | Screenshot the top 3 overlay lines (resolver diag) and check signatures. |

### Credits

- Base: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
- Hook engine: [TsudaKagecha/MinHook](https://github.com/TsudaKagecha/minhook)
- Icons: [FontAwesome 6 Free](https://fontawesome.com)

---

<a id="russian"></a>
## Русский

### О проекте

ZenWare.cc собран на базе [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) и состоит из 3 частей:

- **ZenWare.DLL** — internal-чит (инжектится в игру).
- **ZenWare.Loader** — GUI-лоадер: инжект, однофайловый билд, темы, автоапдейтер с GitHub Releases.
- **ZenWare.External** — внешний оверлей без инжекта: RPM + GDI + `SendInput`.

### Возможности

**Visuals**

- ESP: боксы, HP, ники, дистанция, оружие/предметы, обычные, все особые заражённые (включая бумера, фолбэк по именам для чужих билдов)
- Chams: 5 палитр, союзники / враги / все СИ, сквозь стены
- NoFog, FOV мира + модели, 3-е лицо, кастомный прицел, FPS-оверлей
- 2D-радар, список наблюдателей, алерты танка / ведьмы, киллфид

**Combat**

- Aimbot (silent, приоритет FOV / Distance, head / center, сглаживание, только видимые, обычные + все СИ)
- TriggerBot, AutoShove, AutoPistol, NoSpread (отключаемый)

**Movement**

- BunnyHop (perfect / legit), EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- AutoStrafe: legit / rage / w-only / directional

**Меню / Система**

- Меню на `INSERT`, выгрузка на `F11`, RU/EN (кнопка + `F7`), анимированный RGB-логотип, иконки `?`, табы
- Конфиг: `<gamedir>\ZenWare.cfg` (`bool / int / float / Color`)
- Лог: `%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log` (per-pid фолбэк)

**Лоадер**

- Однофайловый билд: DLL + External + лого внутри (`dist\ZenWare.exe` через `Build-SingleFile.ps1`)
- Анимированный сплэш, инжект `Standard`, кнопка запуска игры через Steam, тёмная / светлая тема, сохранение языка, автоапдейтер

### Требования

- Windows 10/11 x64, Steam + Left 4 Dead 2
- Для сборки: Visual Studio 2022 + `Desktop development with C++` + `Windows SDK 10.0.26100.0`
- Конфигурация: `Release | Win32` (x86)

### Быстрый старт

**Вариант A — один файл (проще всего):**

1. Собери один раз: `powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1` → получишь `dist\ZenWare.exe`.
2. Steam → L4D2 → параметры запуска: `-insecure -windowed -console`.
3. В консоли игры: `map c1m1_hotel`.
4. Запусти `dist\ZenWare.exe` **от администратора** → `ИНЖЕКТ`.

**Вариант B — из исходников:**

1. Открой `ZenWare.sln` → `Release | Win32` → `Rebuild`.
2. Запусти `ZenWare.Loader\bin\Release\ZenWare.Loader.exe` от админа → `ИНЖЕКТ`.

**Вариант C — только External (без инжекта):**

1. Запусти игру как выше.
2. Запусти `ZenWare.External\bin\Release\ZenWare.External.exe` или дважды кликни `START-ZenWare.bat`.

### Горячие клавиши

| Клавиша | Действие |
| ------- | -------- |
| `INSERT` | Открыть / закрыть меню |
| `F11` | Выгрузить чит |
| `F7` | Смена языка RU / EN |
| `Space` (держать) | BunnyHop (по умолчанию) |
| `MOUSE4` (держать) | Aimbot (по умолчанию) |

Кнопки меню и аимбота меняются в `Misc → Menu key / Aimbot key` (клик → `[press key]` → нажми клавишу, `ESC` = выкл).

### Структура проекта

```text
ZenWare.cc/
├── ZenWare.sln
├── ZenWare.DLL/        internal-чит → bin\Release\ZenWare.dll (x86, /MT)
│   └── src/            Entry, Features, Hooks, SDK, Styles, Util
├── ZenWare.Loader/     GUI-лоадер → bin\Release\ZenWare.Loader.exe
├── ZenWare.External/   внешний оверлей → bin\Release\ZenWare.External.exe
│   └── src/            ESP, Memory, Movement, Offsets, Overlay, Resolve
├── Tools/SigScan/      проверка паттернов для актуального client.dll
├── .github/workflows/  CI-сборка (Release | Win32)
├── dist/               однофайловый выход (ZenWare.exe + KAK-ZAPUSTIT.txt)
├── Build-SingleFile.ps1
├── START-ZenWare.bat
├── patch_theme.ps1
├── README.md
└── LICENSE
```

### Частые проблемы

| Симптом | Решение |
| ------- | ------- |
| `XorString` MessageBox при старте | Умер паттерн после обновы игры. Запусти `Tools\SigScan\bin\SigScan.exe` на `left4dead2\bin\client.dll` и обнови `Offsets.cpp`. |
| Краш после `Paint` | Открой лог, найди `[!!!] EXCEPTION`, пришли `module+0x...`. |
| Антивирус ругается | Добавь папку проекта в исключения (эвристика на `WriteProcessMemory` / `CreateRemoteThread`). |
| Квадратики вместо букв | Нажми `F7` (сменятся шрифт / язык). |
| В External нет боксов | Сфоткай верхние 3 строки оверлея (диагностика резолвера), проверь сигнатуры. |

### Credits

- База: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
- Хук-движок: [TsudaKagecha/MinHook](https://github.com/TsudaKagecha/minhook)
- Иконки: [FontAwesome 6 Free](https://fontawesome.com)

---

## License / Лицензия

MIT — see [LICENSE](LICENSE) / смотри [LICENSE](LICENSE).

Educational project. Not affiliated with Valve. Only for local / `-insecure` play.
Учебный проект. Не связан с Valve. Только локальная игра / `-insecure`.
