<p align="center">
  <img src="ZenWare.Loader/zenwareLOGO.png" width="160" alt="ZenWare logo" />
</p>

<h1 align="center">ZenWare.cc</h1>

<p align="center">
  Internal + External training software for <b>Left 4 Dead 2</b><br />
  x86 · C++17 · Visual Studio 2022 · MinHook · v3.5
</p>

<p align="center">
  <a href="https://github.com/krakensuit/ZenWare.cc/actions"><img src="https://github.com/krakensuit/ZenWare.cc/actions/workflows/build.yml/badge.svg" alt="build" /></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="license: MIT" /></a>
  <a href="SECURITY.md"><img src="https://img.shields.io/badge/distribution-source--only-blue.svg" alt="source only" /></a>
  <a href="https://store.steampowered.com/app/550/Left_4_Dead_2/"><img src="https://img.shields.io/badge/game-L4D2-2a475e.svg" alt="L4D2" /></a>
  <img src="https://img.shields.io/badge/platform-Win32%20x86-lightgrey.svg" alt="Win32 x86" />
</p>

<p align="center">
  <b>Language / Язык:</b>
  <a href="#english">English</a> ·
  <a href="#russian">Русский</a> ·
  <a href="#legal">Legal</a>
</p>

> **Source-only. Education and local servers only.**
> This repository contains **no prebuilt binaries**. Build from source and play
> on your own server with `-insecure` (e.g. `map c1m1_hotel`).
> Using it on VAC-secured servers can ban your account. Use at your own risk.
>
> **Только исходники. Только обучение и локальный сервер.**
> Готовых `.exe` / `.dll` в репозитории и релизах **нет**. Собери из исходников
> и играй на своём сервере с `-insecure` (например `map c1m1_hotel`).
> Игра на VAC-серверах = бан. Всё на твой риск.

---

## Contents

- [English](#english) — about, features, requirements, build, hotkeys, troubleshooting
- [Русский](#russian) — о проекте, возможности, сборка, клавиши, проблемы
- [Legal](#legal) — license, notices, trademarks, distribution policy

---

<a id="english"></a>
## English

### About

ZenWare.cc is based on [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) and has 3 parts:

| Part | What it is | Output |
| ---- | ---------- | ------ |
| `ZenWare.DLL` | Internal module injected into the game | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI loader, local single-file build, themes | local `dist/ZenWare.exe` (not published) |
| `ZenWare.External` | Read-only overlay, no injection (RPM + GDI + `SendInput`) | `bin/Release/ZenWare.External.exe` |

No binaries are published. See [SECURITY.md](SECURITY.md).

### Features

<details>
<summary><b>Visuals</b> — ESP, Chams, overlays</summary>

- ESP: boxes, health bar, names, distance, weapons/items, commons, all Special Infected incl. Boomer (class-name fallback for foreign builds)
- Chams: 5 palettes, allies / enemies / all SI, through walls
- NoFog, world + viewmodel FOV, thirdperson, custom crosshair, FPS overlay
- 2D radar, spectator list, Tank / Witch alerts, killfeed

</details>

<details>
<summary><b>Combat</b> — aim assist</summary>

- Aimbot: silent, FOV / Distance priority, head / center, smoothing, visible-only, commons + all SI
- TriggerBot, AutoShove, AutoPistol, NoSpread (toggleable)

</details>

<details>
<summary><b>Movement</b> — bhop and strafe</summary>

- BunnyHop (perfect / legit), EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- AutoStrafe: legit / rage / w-only / directional

</details>

<details>
<summary><b>Menu / System</b></summary>

- `INSERT` menu, `F11` unload, RU/EN toggle (button + `F7`), animated RGB logo, `?` help icons, tabs
- Config: `<gamedir>/ZenWare.cfg` (`bool / int / float / Color`)
- Logger: `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (per-pid fallback)

</details>

### Requirements

- Windows 10/11 x64, Steam + Left 4 Dead 2
- Build: Visual Studio 2022 + `Desktop development with C++` + `Windows SDK 10.0.26100.0`
- Target: `Release | Win32` (x86)

### Build and run (local only)

```powershell
# 1. Build everything locally (DLL -> External -> Loader)
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1

# 2. Steam -> L4D2 -> launch options:
-insecure -windowed -console

# 3. In-game console:
map c1m1_hotel
```

Then run the locally built loader **as administrator** and press `INJECT`.
External-only mode (no injection): run your local
`ZenWare.External/bin/Release/ZenWare.External.exe` or `START-ZenWare.bat`.

Do not upload the built `exe` to public GitHub Releases.

### Hotkeys

| Key | Action |
| --- | ------ |
| `INSERT` | Open / close menu |
| `F11` | Unload |
| `F7` | Language RU / EN |
| `Space` (hold) | BunnyHop (default) |
| `MOUSE4` (hold) | Aimbot (default) |

Rebind in `Misc → Menu key / Aimbot key`: click → `[press key]` → press a key, `ESC` = off.

### Project structure

```text
ZenWare.cc/
├── ZenWare.sln
├── ZenWare.DLL/            internal module (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/         GUI loader (local build only, no auto-download)
├── ZenWare.External/       external overlay (src: ESP, Memory, Movement, Overlay)
├── Tools/SigScan/          signature verifier for your local client.dll
├── .github/workflows/      CI: compile check only, no binary upload
├── dist/                   local output only (git-ignored, never committed)
├── Build-SingleFile.ps1    local builder, no Publish
├── START-ZenWare.bat       local external launcher
├── README.md
├── LICENSE                 MIT
├── NOTICE.md               third-party notices
└── SECURITY.md             source-only policy
```

### Troubleshooting

| Symptom | Fix |
| ------- | --- |
| `XorString` MessageBox on start | A signature broke after a game update. Run `Tools/SigScan` on `left4dead2/bin/client.dll`, update `Offsets.cpp`. |
| Crash after `Paint` | Open the log, find `[!!!] EXCEPTION`, report `module+0x...` without dumps. |
| Antivirus flag | Project folder → exclusions (heuristic on `WriteProcessMemory` / `CreateRemoteThread`). |
| Squares instead of letters | Press `F7` (font fallback / language). |
| External shows no boxes | Check the top overlay lines (resolver diagnostics) and signatures. |

---

<a id="russian"></a>
## Русский

### О проекте

ZenWare.cc собран на базе [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base), 3 части:

| Часть | Что это | Выход |
| ----- | ------- | ----- |
| `ZenWare.DLL` | Модуль, инжектится в игру | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI-лоадер, локальный однофайловый билд, темы | локальный `dist/ZenWare.exe` (не публикуется) |
| `ZenWare.External` | Оверлей без инжекта (RPM + GDI + `SendInput`) | `bin/Release/ZenWare.External.exe` |

Бинарников в раздаче нет. См. [SECURITY.md](SECURITY.md).

### Возможности

<details>
<summary><b>Visuals</b> — ESP, Chams, оверлеи</summary>

- ESP: боксы, HP, ники, дистанция, оружие/предметы, обычные, все СИ включая бумера (фолбэк по именам)
- Chams: 5 палитр, союзники / враги / все СИ, сквозь стены
- NoFog, FOV мира + модели, 3-е лицо, прицел, FPS-оверлей
- 2D-радар, наблюдатели, алерты танка / ведьмы, киллфид

</details>

<details>
<summary><b>Combat</b> — аим</summary>

- Aimbot: silent, приоритет FOV / Distance, head / center, сглаживание, только видимые, обычные + все СИ
- TriggerBot, AutoShove, AutoPistol, NoSpread (отключаемый)

</details>

<details>
<summary><b>Movement</b> — баннихоп и стрейфы</summary>

- BunnyHop (perfect / legit), EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- AutoStrafe: legit / rage / w-only / directional

</details>

<details>
<summary><b>Меню / Система</b></summary>

- Меню на `INSERT`, выгрузка `F11`, RU/EN (кнопка + `F7`), RGB-логотип, иконки `?`, табы
- Конфиг: `<gamedir>/ZenWare.cfg` (`bool / int / float / Color`)
- Лог: `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (per-pid фолбэк)

</details>

### Требования

- Windows 10/11 x64, Steam + Left 4 Dead 2
- Сборка: Visual Studio 2022 + `Desktop development with C++` + `Windows SDK 10.0.26100.0`
- Цель: `Release | Win32` (x86)

### Сборка и запуск (только локально)

```powershell
# 1. Собрать всё локально (DLL -> External -> Loader)
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1

# 2. Steam -> L4D2 -> параметры запуска:
-insecure -windowed -console

# 3. Консоль игры:
map c1m1_hotel
```

Дальше запусти собранный лоадер **от администратора** и нажми `ИНЖЕКТ`.
Режим External без инжекта: твой локальный
`ZenWare.External/bin/Release/ZenWare.External.exe` или `START-ZenWare.bat`.

Собранный `exe` в public Releases не загружай.

### Клавиши

| Клавиша | Действие |
| ------- | -------- |
| `INSERT` | Меню |
| `F11` | Выгрузка |
| `F7` | Язык RU / EN |
| `Space` (держать) | BunnyHop (по умолчанию) |
| `MOUSE4` (держать) | Aimbot (по умолчанию) |

Смена в `Misc → Menu key / Aimbot key`: клик → `[press key]` → нажми клавишу, `ESC` = выкл.

### Структура

```text
ZenWare.cc/
├── ZenWare.sln
├── ZenWare.DLL/            internal-модуль (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/         GUI-лоадер (только локально, без автозагрузки)
├── ZenWare.External/       внешний оверлей (src: ESP, Memory, Movement, Overlay)
├── Tools/SigScan/          проверка сигнатур для твоего client.dll
├── .github/workflows/      CI: только проверка сборки, без выгрузки exe
├── dist/                   только локальный выход (в гите нет, игнорируется)
├── Build-SingleFile.ps1    локальный сборщик, без Publish
├── START-ZenWare.bat       локальный запуск External
├── README.md
├── LICENSE                 MIT
├── NOTICE.md               сторонние компоненты
└── SECURITY.md             политика source-only
```

### Частые проблемы

| Симптом | Решение |
| ------- | ------- |
| `XorString` при старте | Умер паттерн после обновы. Прогони `Tools/SigScan` на `left4dead2/bin/client.dll`, обнови `Offsets.cpp`. |
| Краш после `Paint` | Открой лог, найди `[!!!] EXCEPTION`, пришли `module+0x...` без дампов. |
| Антивирус | Папку проекта в исключения (эвристика на `WriteProcessMemory` / `CreateRemoteThread`). |
| Квадратики вместо букв | Нажми `F7` (шрифт / язык). |
| В External нет боксов | Смотри верхние строки оверлея (диагностика), проверь сигнатуры. |

---

<a id="legal"></a>
## Legal / Право

- **License:** MIT — see [LICENSE](LICENSE). Provided `AS IS`, without warranty.
- **Third-party:** see [NOTICE.md](NOTICE.md) (MinHook, FontAwesome, Valve SDK headers, base).
- **Trademarks:** Left 4 Dead 2 and Steam are trademarks of Valve Corporation. Not affiliated, not endorsed.
- **Distribution:** source-only — see [SECURITY.md](SECURITY.md). No prebuilt binaries in repo or Releases.
- **Use:** education and local `-insecure` servers only. No VAC bypass support. Bans are your responsibility.
- **Лицензия:** MIT — смотри [LICENSE](LICENSE). Код `AS IS`, без гарантий.
- **Чужие компоненты:** смотри [NOTICE.md](NOTICE.md).
- **Товарные знаки:** Left 4 Dead 2 и Steam принадлежат Valve. Проект не связан с Valve.
- **Распространение:** только исходники — смотри [SECURITY.md](SECURITY.md).
- **Использование:** обучение и локальный `-insecure`. Обхода VAC нет и не будет. Баны — твоя ответственность.

### Credits

- Base: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
- Hook engine: MinHook (Tsuda Kageyu, BSD-style, headers preserved)
- Icons: [FontAwesome 6 Free](https://fontawesome.com)
