# ZenWare.cc — Internal Cheat for Left 4 Dead 2

Internal чит для Left 4 Dead 2 на базе **Lak3/l4d2-internal-base** (x86, C++17/20, Visual Studio 2022, MinHook, DirectX 9/11, ImGui, FontAwesome 6).

> **Только для обучения и игры на локальном сервере с `-insecure`. Использование на VAC-серверах может привести к блокировке.**

---

### Возможности

**Visuals**
- **ESP** — боксы, полоска здоровья, ники, дистанция, предметы на земле (`CWeaponSpawn`), обычные заражённые
- **Chams** — `UnlitGeneric` материалы `debug/debugambientcube`, 5 палитр, `Through Walls` (`MATERIAL_VAR_IGNOREZ`)
- **NoFog / Viewmodel FOV** — `ShouldDrawFog:18`, `GetViewModelFOV:40`
- **Crosshair / FPS Overlay** — кастомный прицел, `GlobalVars->frametime`

**Combat**
- **Aimbot** — silent, приоритеты `FOV/Distance`, хитбокс `head/center`, сглаживание, `FixMovement`, `bVisibleOnly`
- **TriggerBot** — трейс из прицела `MASK_SHOT`
- **AutoShove** — авто-толчок при `m_tongueOwner / m_pounceAttacker`
- **AutoPistol** — hold-to-fire для пистолетов

**Movement**
- **BunnyHop** — perfect bhop, `EdgeJump`, `EdgeBug`, `LongJumpHelper`, `FastStop`, `Prestrafe`
- **AutoStrafe** — 3 режима: `legit mousedx / rage circle / w-only`

**System**
- **Menu** — `320x480`, drag за шапку, 4 таба `Visuals/Move/Combat/Misc`, свитчи 30x14, `F11` unload, `INSERT` по умолчанию
- **Config** — `<gamedir>\ZenWare.cfg` `key=value` (`bool/int/float/Color`)
- **Killfeed** — slide/fade 3с, полупрозрачный фон, акцент `0,255,171`
- **Logger** — `%TEMP%\ZenWare.log` → `<gamedir>\ZenWare.log`, VEH `CrashRecorder` с фильтром `<0x80000000`

---

### Структура проекта

```
ZenWare.cc/
├── ZenWare.sln
├── ZenWare.DLL/                  → bin\Release\ZenWare.dll (x86, /MT)
│   └── src/
│       ├── DllMain.cpp           CreateThread(InitThread) -> G::ModuleEntry.Load()
│       ├── Entry/Entry.cpp       VEH, Init, RequestUnload
│       ├── Hooks/                ClientMode:27, ModelRender:19, EngineVGui:Paint, WndProc, BasePlayer, CL_Main...
│       ├── Features/             Aimbot, BunnyHop, ESP, Menu, Chams, Visuals, Killfeed, Config, TriggerBot...
│       ├── SDK/DrawManager       G::Draw (CreateFont, String, Rect, Circle)
│       ├── SDK/GameUtil          IsValidTarget/GetEyePosition (netvars only)
│       └── Util/Logger, Offsets (15 паттернов), Pattern, Anim
├── ZenWare.Loader/               → bin\Release\ZenWare.Loader.exe
│   └── Main.cpp                  560x250 mint #0F1210/#00FFAB, Standard/Manual, DarkMode_Explorer
└── Tools/SigScan/                верификатор паттернов на актуальных бинарях
```

---

### Сборка

1. Visual Studio 2022 + `Desktop development with C++` + `Windows SDK 10.0.26100.0`
2. `ZenWare.sln` -> `Release | Win32` -> `Rebuild`
3. Результат: `ZenWare.DLL\bin\Release\ZenWare.dll`, `ZenWare.Loader\bin\Release\ZenWare.Loader.exe`

> Лоадер содержит DLL как `RCDATA` (`resource.rc` `IDR_ZENWARE_DLL`) — для отправки другу достаточно одного `exe`.

---

### Использование

1. Steam -> L4D2 -> Параметры запуска: `-insecure -windowed -console`
2. В игре: `map c1m1_hotel` (локальный сервер)
3. Запустить `ZenWare.Loader.exe` **от имени администратора** -> `ИНЖЕКТ` (по умолчанию `Standard (быстрый)`)
4. В игре `INSERT` — меню, `F11` — выгрузка
5. Бинды меняются в `Misc -> Menu key / Aimbot key` (клик -> `[press key]` -> нажми клавишу, `ESC` = off)

**Управление по умолчанию:**
`BunnyHop` — удерживай `Space`, `Aimbot` — `MOUSE4` (или `0` = всегда), `TriggerBot` — по прицелу.

**Конфиг:** `Misc -> Save config / Load config` пишет `<gamedir>\ZenWare.cfg`.

**Логи:** `%TEMP%\ZenWare.log` (ранний этап) + `<gamedir>\ZenWare.log` после `RelocateToGameDir()`, `%TEMP%\ZenWare.Loader.log`.

---

### Дизайн

*   Палитра `ApplyZenWareStyle()` — фон `#0F0F12`/`#16161F`, акцент `#00FFAB` -> `#00B374`, текст `#FFFFFF`/`#B0B0C0`, рамка `#00FFAB 50%`, hover `#33FFBB`, скругления `2px`.
*   Логотип `ZenWare` — `ImDrawList` градиент `#00FFAB -> #33FFBB` + 3 слоя свечения, по центру шапки.
*   Лоадер — `GdiGradientFill` header `accent->accent2` + шиммер `g_shimmer` 30мс, `DwmSetWindowAttribute` тёмный заголовок + скругление.
*   Шрифты: `Tahoma 15px` + `fa-solid-900.ttf` `MergeMode` (диапазон `0xe005-0xf8ff`), `Segoe UI Semibold 10pt` / `Consolas 9pt` в лоадере (DPI-aware).

---

### Troubleshooting

*   `XorString` MessageBox при старте — паттерн умер после обновления игры → запусти `Tools\SigScan\bin\SigScan.exe` на `left4dead2\bin\client.dll` и обнови в `Offsets.cpp`.
*   Игра крашится после `Paint` — смотри `[!!!] EXCEPTION` в логе, пришли `module+0x...`.
*   Антивирус ругается на лоадер — добавь папку проекта в исключения Защитника (эвристика на `WriteProcessMemory/CreateRemoteThread`).

---

### Credits

*   База: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
*   Хук-движок: [TsudaKagecha/MinHook](https://github.com/TsudaKagecha/minhook)
*   Иконки: [FontAwesome 6 Free](https://fontawesome.com)
*   Проект ZenWare — учебный, не для VAC-серверов.
