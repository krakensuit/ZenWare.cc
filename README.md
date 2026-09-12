<div align="center">

<img src="ZenWare.Loader/zenwareLOGO.png" width="170" alt="ZenWare logo" />

# ZenWare.cc

**Internal + External training software for Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.8.2`

<br />

<a href="https://github.com/krakensuit/ZenWare.cc/actions"><img src="https://github.com/krakensuit/ZenWare.cc/actions/workflows/build.yml/badge.svg?style=flat-square" alt="build" /></a>
<a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg?style=flat-square" alt="license: MIT" /></a>
<a href="SECURITY.md"><img src="https://img.shields.io/badge/distribution-source--only-blue.svg?style=flat-square" alt="source only" /></a>
<a href="https://store.steampowered.com/app/550/Left_4_Dead_2/"><img src="https://img.shields.io/badge/game-L4D2-2a475e.svg?style=flat-square" alt="L4D2" /></a>
<img src="https://img.shields.io/badge/platform-Win32_x86-lightgrey.svg?style=flat-square" alt="Win32 x86" />

<br /><br />

**🌐 Language:**

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

## Navigate

| | | | |
|---|---|---|---|
| [About](#english) | [How it works](#how-it-works) | [Features](#features) | [Build & Run](#build-and-run--local-only) |
| [Hotkeys](#hotkeys) | [Config & Logs](#config--logs) | [Project structure](#project-structure) | [FAQ](#troubleshooting) |

---

<a id="english"></a>
## English

### About

ZenWare.cc is a training sandbox for Left 4 Dead 2 (Source engine, 2009).
It exists for one purpose: learning how game internals work — rendering,
prediction, movement and networking — on your own PC, on your own server,
with `-insecure`. There is no VAC bypass, no support for official servers,
and no prebuilt binaries anywhere in this project.

The codebase started from [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
and grew into three independent tools in one solution (`ZenWare.sln`, `Release | Win32`):

| Part | Mode | What it does | Output |
|---|---|---|---|
| `ZenWare.DLL` | Internal (injected) | Full cheat: corner ESP, Chams, Aimbot, Movement, in-game menu | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI launcher, local build only | Manual-map / LoadLibrary injection, themes, 8 languages | local `dist/ZenWare.exe`, never published |
| `ZenWare.External` | External (no injection) | RPM reads + GDI overlay + `SendInput` bhop/strafe | `bin/Release/ZenWare.External.exe` |

New to this kind of software? Start with the **External** overlay — it never
writes to game memory and is the safest way to study the pipeline.
Details: [SECURITY.md](SECURITY.md) · Third-party: [NOTICE.md](NOTICE.md) ·
License: [LICENSE](LICENSE) · Runtime wiring: [ARCHITECTURE.md](ARCHITECTURE.md).

<a id="how-it-works"></a>
### How it works

**Internal DLL.** On inject, `DllMain` spawns a thread (never runs logic on
the loader lock) and `Entry::Load` runs an ordered, fail-closed init:
logger → crash recorders → `serverbrowser.dll` wait → pattern scan (with
`ZenWare.offsets` RVA cache) → interfaces (`VEngineCvar007` probed in
`vstdlib.dll` first) → netvars → `MinHook` hooks → config load. Any missing
pattern or interface aborts with a message box instead of crashing.

Per frame two chains run:

- **Tick** (`ClientMode::CreateMove`) — prediction wrap, then movement
  (BunnyHop → AutoStrafe → JumpStats → AutoShove), then combat (Aimbot →
  TriggerBot → AutoPistol → NoSpread). The engine original is called exactly
  once per tick and its result is reused.
- **Frame** (`EngineVGui::Paint`, `PAINT_UIPANELS` only) — thirdperson /
  fullbright / hide-hands cvars, then Killfeed tick, Hitmarker tick, ESP,
  Radar, Alerts, Menu, crosshair, grenade preview, overlay, and the animated
  Killfeed / Hitmarker / JumpStats draws.

There are no engine game-events by design — Killfeed and Hitmarker poll
HP/alive transitions. Drawing goes only through the `G::Draw` facade over
`MatSystemSurface`; entity reads are netvars-only with `ClassID` checks
before any virtual call.

**Loader.** A Win32 GUI app with a splash screen, dark/light system theme,
8 interface languages and an animated RGB logo. It embeds the DLL, the
External overlay and the logo as resources, so the build output is a single
file. Injection is manual-map (relocations, imports, section protection,
header wipe) with a `LoadLibrary` fallback. No network code, no auto-update.

**External.** A read-only tool: `ReadProcessMemory` snapshots at 20 Hz, a
click-through layered GDI overlay draws at 60 Hz, movement uses `SendInput`
only. Signature + anchor + validation resolver re-finds addresses every run.

### Features

<details open>
<summary><b>Visuals</b> — ESP · Chams · Overlays</summary>

- **ESP** — corner brackets (team-colored), bordered health bar + optional
  HP text, shadowed names, distance, weapons with clip ammo, ground items,
  commons, all Special Infected incl. Boomer (class-name fallback for
  foreign builds), optional ESP max-distance limit, dimmed snaplines
- **Chams** — 5 palettes, separate colors for enemies / allies / Tank,
  through-walls toggle, ghost and dormant filtering
- **World** — NoFog, Fullbright toggle, separate world + viewmodel FOV
  sliders, thirdperson with distance slider, viewmodel hide, custom
  crosshair, FPS / position / HP overlay, alive-commons counter
- **Intel** — 2D yaw-rotated radar, spectator list (observer targets),
  Tank / Witch distance alerts, polled killfeed with animated slide-in
  cards, hitmarker with hitsound, damage numbers, cross hitmark and
  session stats (hits / shots / accuracy)
- **Team play** — pinned-by-Smoker/Hunter warning, revive alert, Tank HP
  bar, team HP panel, nearby-SI list with its own panel, spit (goo) alert,
  thrown-grenade timers (pipe / molotov fire)
- **Feedback** — red damage flash, damage-direction arrow pointing at the
  last attacker, min-damage filter, weapon HUD with clip + reserve ammo,
  yellow `RELOADING` and red low-ammo warnings
- **Throwables** — grenade trajectory preview (molotov / pipe / bile /
  grenade launcher) with bounce simulation and landing marker

</details>

<details>
<summary><b>Combat</b> — aim assist</summary>

- **Aimbot** — silent aim, FOV / Distance priority, head / center point,
  smoothing, visible-only check, key bind, commons + all SI + Tank
- **Per-weapon tuning** — own FOV / smoothing / hitbox per group (rifles,
  SMGs, shotguns, snipers, pistols)
- **Helpers** — TriggerBot (fires on the post-aim ray, no 1-tick lag),
  AutoShove (frees pinned mates), AutoPistol, NoSpread (11-gun whitelist,
  on by default)

</details>

<details>
<summary><b>Movement</b> — bhop & strafe</summary>

- **Bhop kit** — perfect / legit BunnyHop, EdgeJump, EdgeBug, JumpBug,
  LongJump helper, FastStop, Prestrafe, AutoDuck, configurable jump delay
- **Strafe** — legit / rage / W-only / directional AutoStrafe
- **JumpStats** — KZ-style panel: distance, prestrafe, max speed, fall,
  jump height, airtime, live sync % while airborne, strafe count,
  JB / EB session counters

</details>

<details>
<summary><b>Menu / System</b></summary>

- In-game menu (`INSERT`): 5 tabs (Visuals / Move / View / Combat / Misc),
  per-tab scroll, animated RGB logo, `?` help popups (EN + RU), color
  swatches, key binding rows, sliders, panic key to hide ESP instantly
- 8 interface languages with live switching (button or `F7`):
  EN / RU / DE / ES / PT / PL / FR / ZH
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`),
  `bool / int / float / Color`, float sliders use int proxies with resync
- Logger — `%TEMP%/ZenWare.log` relocated to `<gamedir>/ZenWare.log`
  (per-PID fallback), crash records with `module+offset` breadcrumbs
- Offsets cache — `<gamedir>/ZenWare.offsets` (auto, delete to force
  rescan); `Tools/SigScan` verifies all 15 patterns offline against your
  game files

</details>

<a id="build-and-run--local-only"></a>
### Build and run — local only

**1. Build** (Visual Studio 2022 with `Desktop development with C++`):

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

<a id="hotkeys"></a>
### Hotkeys

| Key | Action |
|:---:|---|
| `INSERT` | Open / close menu |
| `F11` | Unload |
| `F7` | Language EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (hold) | BunnyHop (default) |
| `MOUSE4` (hold) | Aimbot (default) |

Rebind: `Misc → Menu key / Aimbot key` — click → `[press key]` → press a key, `ESC` = off.
ESP panic key hides all boxes instantly (bind in `Visuals → ESP`).

<a id="config--logs"></a>
### Config & logs

- Config slots live next to the game DLL path resolution: `ZenWare.cfg`,
  `ZenWare2.cfg`, `ZenWare3.cfg` — plain `key=value`, one value per line
  (117 keys). Session-only values (JB/EB counters, notify timestamps) are
  never saved.
- Runtime log: `<gamedir>/left4dead2/ZenWare.log`. Crash reports look like
  `[!!!] EXCEPTION code=... at module+0x...` followed by the last breadcrumb
  — that pair is everything needed to locate a crash, no dumps required.
- Signature cache: `<gamedir>/left4dead2/ZenWare.offsets` stores RVAs plus a
  module fingerprint; after a game update delete it or run
  `Verify-Signatures.bat` and fix `Offsets.cpp` if it reports `NOT FOUND`.

### Project structure

```text
ZenWare.cc/
├── ZenWare.sln                  solution (DLL + Loader + External)
├── ZenWare.DLL/                 internal module
│   ├── src/Entry/               init order, crash recorders, unload
│   ├── src/Hooks/               ClientMode / EngineVGui / ModelRender / WndProc …
│   ├── src/SDK/                 L4D2 interfaces, entities, DrawManager, GameUtil
│   ├── src/Features/            Vars (settings) + Aimbot/ESP/Menu/… + Config + Lang
│   └── src/Util/                Logger, Pattern, Offsets cache, NetVars, MinHook
├── ZenWare.Loader/              GUI loader — local build only, no auto-update
├── ZenWare.External/            external overlay — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               signature verifier for your local client.dll
├── .github/workflows/           CI compile check — no binary upload
├── dist/                        local output only (git-ignored)
├── Build-SingleFile.ps1         local builder (no Publish)
├── Verify-Signatures.bat        one-click pattern check vs your game files
├── START-ZenWare.bat            local external launcher
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
└── README_RU/DE/ES/PT/PL/FR/ZH.md   translations of this file
```

<a id="troubleshooting"></a>
### Troubleshooting

| Symptom | Fix |
|---|---|
| `XorString` / `FATAL ERROR` MessageBox on start | Signature broke after a game update → double-click `Verify-Signatures.bat`, update `Offsets.cpp` |
| Crash in game | Open the log → find `[!!!] EXCEPTION` → report `module+0x...` plus the last breadcrumb, no dumps |
| Antivirus flag | Folder → exclusions (heuristic on `WriteProcessMemory` / `CreateRemoteThread`) |
| Squares instead of letters | Press `F7` (font fallback / language) |
| External: no boxes | Check top overlay lines (resolver diagnostics) and signatures |
| Menu opens but clicks go to the game | Close the game menu first (`ESC` handled by the cheat only when its own menu is open) |

<a id="legal"></a>
## Legal

| Topic | Terms |
|---|---|
| License | MIT — [LICENSE](LICENSE), `AS IS`, no warranty |
| Third-party | [NOTICE.md](NOTICE.md): MinHook, FontAwesome, Valve SDK headers, base |
| Trademarks | Left 4 Dead 2 / Steam by Valve. Not affiliated, not endorsed |
| Distribution | Source-only — [SECURITY.md](SECURITY.md), no binaries |
| Use | Local `-insecure` only. No VAC bypass. Bans are yours |

<div align="center">

**Credits** ·
Base: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Hook engine: MinHook (Tsuda Kageyu, BSD-style) ·
Icons: [FontAwesome 6 Free](https://fontawesome.com)

</div>
