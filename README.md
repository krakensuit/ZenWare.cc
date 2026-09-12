<div align="center">

<img src="ZenWare.Loader/zenwareLOGO.png" width="170" alt="ZenWare logo" />

# ZenWare.cc

**Internal + External training software for Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.6`

<br />

<a href="https://github.com/krakensuit/ZenWare.cc/actions"><img src="https://github.com/krakensuit/ZenWare.cc/actions/workflows/build.yml/badge.svg?style=flat-square" alt="build" /></a>
<a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg?style=flat-square" alt="license: MIT" /></a>
<a href="SECURITY.md"><img src="https://img.shields.io/badge/distribution-source--only-blue.svg?style=flat-square" alt="source only" /></a>
<a href="https://store.steampowered.com/app/550/Left_4_Dead_2/"><img src="https://img.shields.io/badge/game-L4D2-2a475e.svg?style=flat-square" alt="L4D2" /></a>
<img src="https://img.shields.io/badge/platform-Win32_x86-lightgrey.svg?style=flat-square" alt="Win32 x86" />

<br /><br />

**🌐 Language / Язык / Sprache / Idioma / Idioma / Język / Langue / 语言:**

| | | | |
|---|---|---|---|
| 🇬🇧 [English](#english) | 🇷🇺 [Русский](README_RU.md) | 🇩🇪 [Deutsch](README_DE.md) | 🇪🇸 [Español](README_ES.md) |
| 🇵🇹 [Português](README_PT.md) | 🇵🇱 [Polski](README_PL.md) | 🇫🇷 [Français](README_FR.md) | 🇨🇳 [中文](README_ZH.md) |

[⚖️ Legal](#legal)

</div>

---

> **Source-only · Education and local servers only.**
> No prebuilt `.exe` / `.dll` in this repo or Releases — build from source,
> launch the game with `-insecure`, play your own map (`map c1m1_hotel`).
> VAC-secured servers = ban risk. Use at your own risk.
>
> **Только исходники · Только обучение и локальный сервер.**
> Готовых `.exe` / `.dll` в репозитории и релизах нет — собери из исходников,
> запускай игру с `-insecure`, играй на своей карте (`map c1m1_hotel`).
> VAC-серверы = риск бана. Всё на твой риск.

## Navigate

| | | | |
|---|---|---|---|
| [About](#english) | [Build & Run](#build-en) | [Hotkeys](#hotkeys-en) | [FAQ](#faq-en) |
| [О проекте](#russian) | [Сборка](#build-ru) | [Клавиши](#hotkeys-ru) | [Проблемы](#faq-ru) |

## At a glance

| Component | Mode | What it does |
|---|---|---|
| `ZenWare.DLL` | Internal (injected) | ESP, Chams, Aimbot, Movement, Menu — `bin/Release/ZenWare.dll` |
| `ZenWare.Loader` | GUI launcher, local build only | Single-file assembly, themes, language — `dist/` stays on your PC |
| `ZenWare.External` | External (no injection) | RPM + GDI overlay + `SendInput` bhop/strafe — safest to study first |

Details: [SECURITY.md](SECURITY.md) · Third-party: [NOTICE.md](NOTICE.md) · License: [LICENSE](LICENSE)

---

<a id="english"></a>
## English

### About

Based on [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base). Three parts, one solution (`ZenWare.sln`, `Release | Win32`):

| Part | Output |
|---|---|
| `ZenWare.DLL` — internal module | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — GUI loader | local `dist/ZenWare.exe`, never published, releases-page pill |
| `ZenWare.External` — read-only overlay | `bin/Release/ZenWare.External.exe` |

### Features

<details open>
<summary><b>Visuals</b> — ESP · Chams · Overlays</summary>

- **ESP** — boxes, health bar, names, distance, weapons/items, commons, all Special Infected incl. Boomer (class-name fallback for foreign builds)
- **Chams** — 5 palettes, allies / enemies / all SI, through walls
- **World** — NoFog, world + viewmodel FOV, thirdperson, custom crosshair, FPS overlay
- **Intel** — 2D radar, spectator list, Tank / Witch alerts, killfeed, hitmarker (hitsound + damage numbers)
- **Team play** — pinned warning, revive alert, tank HP bar, team HP panel, SI nearby list
- **Feedback** — cross hitmark, session stats (hits/shots/accuracy), damage flash, min-damage filter, thrown grenade timers
- **Throwables** — grenade trajectory preview (molotov / pipe / bile / grenade launcher) with landing marker

</details>

<details>
<summary><b>Combat</b> — aim assist</summary>

- **Aimbot** — silent, FOV / Distance priority, head / center, smoothing, visible-only, commons + all SI
- **Per-weapon** — own FOV / smoothing / hitbox per group (rifles, SMG, shotguns, snipers, pistols)
- **Helpers** — TriggerBot, AutoShove, AutoPistol, NoSpread (toggleable)

</details>

<details>
<summary><b>Movement</b> — bhop & strafe</summary>

- **Bhop kit** — perfect / legit BunnyHop, EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- **Strafe** — legit / rage / w-only / directional AutoStrafe

</details>

<details>
<summary><b>Menu / System</b></summary>

- `INSERT` menu · `F11` unload · language EN/RU/DE/ES/PT/PL/FR/ZH (button + `F7`) · animated RGB logo · `?` help icons · tabs · wheel scroll
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`, `bool / int / float / Color`)
- Logger — `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (per-pid fallback)
- Offsets cache — `<gamedir>/ZenWare.offsets` (auto, delete to force rescan)

</details>

<a id="build-en"></a>
### Build and run — local only

**1. Build**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Prepare the game** — Steam → L4D2 → launch options:

```text
-insecure -windowed -console
```

**3. Load a local map** — in-game console:

```text
map c1m1_hotel
```

**4. Inject** — run your locally built loader **as administrator** → `INJECT`.

External-only (no injection): run your local
`ZenWare.External/bin/Release/ZenWare.External.exe` or `START-ZenWare.bat`.

> Do not upload the built `exe` to public GitHub Releases. Source-only policy: [SECURITY.md](SECURITY.md).

### Requirements

| Item | Version |
|---|---|
| OS | Windows 10/11 x64 |
| Game | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Target | `Release` · `Win32` (x86) |

<a id="hotkeys-en"></a>
### Hotkeys

| Key | Action |
|:---:|---|
| `INSERT` | Open / close menu |
| `F11` | Unload |
| `F7` | Language EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (hold) | BunnyHop (default) |
| `MOUSE4` (hold) | Aimbot (default) |

Rebind: `Misc → Menu key / Aimbot key` — click → `[press key]` → press a key, `ESC` = off.

### Project structure

```text
ZenWare.cc/
├── ZenWare.sln                  solution (DLL + Loader + External)
├── ZenWare.DLL/                 internal module (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/              GUI loader — local build only, no auto-update
├── ZenWare.External/            external overlay — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               signature verifier for your local client.dll
├── .github/workflows/           CI compile check — no binary upload
├── dist/                        local output only (git-ignored)
├── Build-SingleFile.ps1         local builder (no Publish)
├── Verify-Signatures.bat        one-click pattern check vs your game files
├── START-ZenWare.bat            local external launcher
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

<a id="faq-en"></a>
### Troubleshooting

| Symptom | Fix |
|---|---|
| `XorString` MessageBox on start | Signature broke after a game update → double-click `Verify-Signatures.bat`, update `Offsets.cpp` |
| Crash after `Paint` | Open the log → find `[!!!] EXCEPTION` → report `module+0x...`, no dumps |
| Antivirus flag | Folder → exclusions (heuristic on `WriteProcessMemory` / `CreateRemoteThread`) |
| Squares instead of letters | Press `F7` (font fallback / language) |
| External: no boxes | Check top overlay lines (resolver diagnostics) and signatures |

<a id="legal"></a>
## Legal / Право

| Topic | EN | RU |
|---|---|---|
| License | MIT — [LICENSE](LICENSE), `AS IS`, no warranty | MIT — [LICENSE](LICENSE), `AS IS`, без гарантий |
| Third-party | [NOTICE.md](NOTICE.md): MinHook, FontAwesome, Valve SDK headers, base | [NOTICE.md](NOTICE.md): MinHook, FontAwesome, заголовки Valve, база |
| Trademarks | Left 4 Dead 2 / Steam by Valve. Not affiliated, not endorsed | Left 4 Dead 2 / Steam принадлежат Valve. Не связан, не одобрен |
| Distribution | Source-only — [SECURITY.md](SECURITY.md), no binaries | Только исходники — [SECURITY.md](SECURITY.md), без бинарников |
| Use | Local `-insecure` only. No VAC bypass. Bans are yours | Только локальный `-insecure`. Обхода VAC нет. Баны — твои |

<div align="center">

**Credits** ·
Base: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Hook engine: MinHook (Tsuda Kageyu, BSD-style) ·
Icons: [FontAwesome 6 Free](https://fontawesome.com)

</div>
