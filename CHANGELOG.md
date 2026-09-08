# Changelog / История версий

All notable changes to ZenWare.cc are documented here.
Все заметные изменения ZenWare.cc — здесь.

## [Unreleased]

### Fixed / Исправлено

- Crash on inject: weapon code called `GetWeaponID()` virtual on unchecked
  entities from `m_hActiveWeapon` (viewmodel/hands/stale slots have a
  different vtable). New `G::Util.IsPlayerEntity/IsWeaponEntity` ClassID gates
  in ESP weapon text, grenade preview, and CreateMove; local player is also
  class-checked now. Melee/meds in hands no longer crash combat features.
  Краш при инжекте: виртуалка оружия на непроверенных сущностях из хендла
  (viewmodel/руки). ClassID-гейты в ESP, превью гранат и CreateMove.
- Config: `trigger.key` is saved now; sliders resync from floats inside
  `Load()` (startup auto-load works, no truncation drift); enum/int clamps
  against corrupt cfg; saved language no longer overwritten by system default;
  F7 toggles RU/EN even with closed menu; crosshair size up to 40;
  `Chams tank` color persisted + swatch in Misc.
  Конфиг: сохраняется клавиша триггера, слайдеры ресинкаются при загрузке,
  защита от битого cfg, язык не затирается, F7, прицел до 40, цвет танка.
- JumpStats honesty: sync is measured on raw pre-AutoStrafe input; JB/EB
  counters count real feature fires (showtick window + enabled + ducked
  landing, EB wins) with manual-duck fallback only when AutoDuck is off;
  `[edge]` only on real EdgeJump fire; no fake EdgeJump after death/ladder;
  Perfect style ignores keyboard while menu is open.
  Честная стата: синк по сырому вводу, счётчики по факту срабатываний,
  [edge] только за реальный EJ, без ложных срабатываний.
- Aimbot/trigger: player visibility is checked eye-to-aimpoint (not
  eye-to-eye); trigger traces post-aim `cmd` angles (no 1-tick lag);
  smoothing also applies to silent aim; trigger fires on Boomer/shifted IDs
  via class-name fallback (no virtuals on unknown types).
  Аим/триггер: видимость к точке аима, триггер по углам после аима, смус для
  сайлента, бумер по имени класса.
- Chams palette no longer stomps manual swatches every DrawModel (applied
  only on palette change); Boomer caught by class name.
  Палитра не затирает ручные цвета; бумер в чамсах.

### Added / Добавлено

- ESP: space-style layout — yellow nickname above the box, white `[Nm]`
  distance and `weapon [clip]` (m_iClip1) below it, green bottom-up HP bar on
  the left (`Ammo count` and `Show teammates` toggles, saved to config).
  ESP как у спейса: жёлтый ник сверху, дистанция и `оружие [патроны]` снизу,
  зелёный ХП-бар слева (тумблеры в меню, сохраняются в конфиг).

- JumpStats: peak height (H) and airtime row, plus session JB/EB success
  counters (the cheat's own duck-window landings).
  Высота прыжка и время полёта в JumpStats, счётчики удачных JB/EB за сессию.
- Damage arrow: 3-second pointer toward the last attacker (best-effort nearest
  visible enemy, no engine events) with distance.
  Стрелка на последнего атакующего на 3 секунды + дистанция.
- No screen effects toggle (default on = previous behaviour): skips
  `DoPostScreenSpaceEffects` (bile overlay, blur, stun).
  Выключение пост-эффектов экрана (рвота/блюр/стан), по умолчанию как раньше.
- Common counter: alive commons within ~40m in the overlay corner.
  Счётчик живых обычных рядом.

### Changed

- Thirdperson without console (space method): camera distance now goes
  through `z_view_distance` via `VEngineCvar007` instead of
  `ClientCmd` (`sv_cheats 1; ...; thirdperson`), which crashed the game.
- Aimbot locks by default: `bSilent` default is now off (visible lock like
  space; silent is still a toggle), commons/specials visibility fixed (below).
- ESP boxes: true 8-corner AABB projection (space method) instead of the
  2-point vertical projection with a fixed 0.55 width — boxes no longer lag
  behind chams at close range and screen edges; items are text-only.
  Боксы ESP: проекция 8 углов как у спейса вместо 2 точек — боксы больше не
  отстают от чамсов; предметы — только текст.
- Player ESP box: clean single outline like space; bounds are now netvar-only
  (origin + mins/maxs height), no `RenderableToWorldTransform` call.
### Fixed / Исправлено

- JumpStats JB/EB counters stuck at zero: BunnyHop strips `IN_DUCK` from cmd
  on the landing tick before JumpStats reads it, so duck-at-land was always
  false. Duck is now latched while airborne; counters also count manual bugs
  (showTick gate removed), each landing counted once (EB wins over JB).
  Счётчики JB/EB стояли на нуле: BunnyHop снимал присед до чтения статой.
  Присед теперь запоминается в полёте; считаются и ручные баги.
- JumpStats numbers: added peak fall speed to the main row, verdict uses the
  finished jump's strafe count (not live), landing log gains fall/duck/eb.
  Цифры статов: скорость падения в главной строке, вердикт по своему прыжку.

- Silent-death hunt (landing crashes with zero telemetry): removed the
  XBUTTON1 ×5 `CL_Move` loop (engine movement simulated 6x per frame while the
  bhop key is held) and the double original `CreateMove` call; JumpStats now
  logs every landing to `ZenWare.log`.
  Охота на тихие вылеты при приземлении: убран цикл XBUTTON1 ×5 в `CL_Move`
  (6 симуляций движения за кадр) и двойной вызов оригинального `CreateMove`;
  каждое приземление пишется в лог.
- Crash diagnostics second net: `SetUnhandledExceptionFilter`, `set_terminate`,
  `SIGABRT`, purecall and CRT-invalid-parameter handlers, plus a lock-free
  `WriteNoLock` crash path (debugger first, then file).
  Вторая сеть диагностики: UEF/terminate/abort/purecall/invalid-parameter и
  запись без лока (сначала в отладчик, потом в файл).
- JumpStats draw: buffers passed as `"%s"` args (stray `%` from sync text could
  hit the formatter); `EnginePrediction` weapon-switch null check.
  Формат-строки JumpStats и null-check смены оружия в предикте.

- ESP item crash: pills/medkits/bile no longer touch `CWeaponSpawn` virtuals.
  Вылет ESP на таблетках/аптечках: чужие классы больше не дёргают виртуалки.
- NoSpread guarded against missing patterns; interface/trace/material null guards.
- Zero-initialized `trace_t` / `player_info_t`; safe format strings; HealthColor div-zero guard.
- Corner brackets on SI/witch/common boxes (same style as players).
  Уголки на боксах СИ/ведьмы/обычных.

## [3.6] — 2026-09-08

### Added / Добавлено

- Hitmarker: hitsound + floating damage numbers (best-effort, local server).
  Хитсаунд + всплывающие цифры урона.
- Grenade trajectory preview for molotov / pipe bomb / vomit jar.
  Предпросмотр траектории гранат (молотов / пайп / желчь).
- Per-weapon aimbot settings: FOV / smoothing / hitbox per weapon group.
  Настройки аимбота по группам оружия.
- Offset cache (`ZenWare.offsets`): fast startup, SigScan-compatible.
  Кэш оффсетов: быстрый старт, совместим с SigScan.
- `Verify-Signatures.bat`: one-click pattern check against local game files.
  Проверка сигнатур в один клик.
- `CHANGELOG.md`, `.editorconfig`.
- Loader "updates" pill: opens the releases page in browser, downloads nothing.
  Пилюля «обновления» в лоадере: открывает страницу релизов, ничего не качает.
- Config slots 1/2/3 (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`).
  Слоты конфига 1/2/3.
- Cross hitmark on crosshair, session stats (hits/shots/accuracy), damage flash.
  Крест попадания, стата сессии, вспышка урона.
- Team HP panel, SI nearby list, thrown grenade timers (pipe fuse + ring).
  Панель HP команды, список особых, таймеры гранат.
- `docs/screenshots/` + HOWTO for README shots.
  Папка и гайд для скриншотов в README.
- Pinned warning, revive alert, tank HP bar, min-damage filter, ESP panic key.
  Алерт пина, реанима, полоса HP танка, мин. урон, паник-кнопка ESP.
- Grenade-launcher trajectory, menu wheel scroll.
  Траектория гранатомёта, скролл меню колесом.
- `ARCHITECTURE.md`, `.clang-format`.

### Removed / Удалено

- Loader auto-updater (network download of `.exe`). The project is source-only now:
  no releases with binaries, no background downloads.
  Автоапдейтер лоадера (скачивание `.exe` по сети). Проект теперь source-only.

## [3.5] — 2026-09-06

- Tank / Witch alerts, aimbot name fallback, ESP common fallback.
  Алерты танка/ведьмы, фолбэк аимбота по именам, ESP обычных.
- CI build workflow, README refresh.

## [3.4] — 2026-09-06

- 2D radar + spectator list (team colors, RU/EN, config).
  2D-радар + список наблюдателей.

## [3.3] — 2026-09-05

- Single-file distribution (`Build-SingleFile.ps1`), centralized version in `resource.h`.
  Однофайловый билд, версия в одном месте.
- RU/EN language toggle (loader, DLL menu, external), offset resolver with diagnostics.
  Переключение RU/EN везде, резолвер оффсетов с диагностикой.

## [3.0–3.2] — 2026-09-04

- Internal DLL + loader + external + SigScan tool, animated menu/splash.
  Internal DLL + лоадер + external + SigScan, анимированные меню/сплэш.
- ESP, Chams, Aimbot (+commons), TriggerBot, movement pack (bhop/strafe/edgejump/jumpbug).
