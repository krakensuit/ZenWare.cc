# Architecture / Архитектура

How ZenWare.cc is wired. Как всё связано.

## Projects / Проекты

```text
ZenWare.sln (Release | Win32, x86, /MT, C++17)
├── ZenWare.DLL       internal module, injected into left4dead2.exe
├── ZenWare.Loader    GUI launcher, local builds only (no auto-update)
├── ZenWare.External  read-only overlay (RPM + GDI + SendInput, no injection)
└── Tools/SigScan     offline pattern verifier (own .vcxproj, not in .sln)
```

## DLL runtime / Жизнь DLL

```text
DllMain → Entry::Load (Entry/Entry.cpp)
  ├─ Logger::Init (%TEMP%\ZenWare.log → <gamedir>\ZenWare.log)
  ├─ Offsets::Init (pattern scan, or ZenWare.offsets cache)
  ├─ Interfaces (client/engine/vguimatsurface, no engine virtuals at startup)
  ├─ NetVarManager (recv-table offsets)
  ├─ Hooks::Init (MinHook + VTable)
  │    ├─ ClientMode::CreateMove  → per-tick logic (aim/movement)
  │    ├─ EngineVGui::Paint       → per-frame render (ESP/menu/overlay)
  │    └─ WndProc                 → menu keys, wheel, input block
  ├─ Config::Load (<gamedir>\ZenWare[2|3].cfg)
  └─Unload (F11) → unhook → FreeLibrary
```

## Per-tick (CreateMove) / Каждый тик

`Hooks/ClientMode/ClientMode.cpp` → `EnginePrediction.Start` →
`BunnyHop / AutoStrafe / JumpStats / AutoShove` (no weapon needed) →
`Aimbot / TriggerBot / AutoPistol / NoSpread` (needs `C_TerrorWeapon*`) →
`EnginePrediction.Finish`. Hitmarker shot counter ticks on `IN_ATTACK` edge.

## Per-frame (Paint) / Каждый кадр

`Hooks/EngineVGui/EngineVGui.cpp` (`PAINT_UIPANELS`):
`Visuals.UpdateThirdPerson → Killfeed.OnTick → Hitmarker.OnTick → ESP.Render →
Radar.Render → Alerts.Render → Menu.Render → crosshair/grenade/overlay →
Killfeed.Draw / Hitmarker.Draw → JumpStats.Draw`.

Health polling (killfeed deaths, hitmarker damage, team panel, alerts) all read
`GetHealth()` per frame — there is no game-event system by design.

## State / Состояние

- `Features/Vars.h` — all tunables, plain `inline` globals per feature.
  Floats edited via int slider proxies (`nFOVSlider` → `flFOV`, synced in `Menu::Render`).
- `Features/Config/` — explicit `key=value` table (`ZenWare.cfg` + slots 2/3).
  Missing keys keep defaults; slot is session-only (not persisted).
- `Features/Lang/` — `Lang::T(en)` EN→RU map; internal IDs stay English.
- `Features/Menu/` — immediate-mode custom UI (320x480, tabs, wheel scroll).
- `Util/Offsets/` — pattern scans + `ZenWare.offsets` cache (module fingerprint).
- `SDK/` — minimal L4D2 reversing layer (entities, interfaces, math, trace).

## Conventions / Правила

- Tabs, Allman braces (see `.editorconfig`, `.clang-format`).
- No `std::string` in config (bool/int/float/Color only).
- Fail-closed reads: null/dormant/lifeState checks before every deref.
- No new engine virtuals without a fallback (see `GameUtil::GetEyePosition`).
- Source-only: no binaries in repo, releases, or CI artifacts.
