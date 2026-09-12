# Changelog / История версий

All notable changes to ZenWare.cc are documented here.
Все заметные изменения ZenWare.cc — здесь.

## [Unreleased]

### Changed / Изменено

- All 8 READMEs rewritten with the expanded template (about, how it works,
  detailed features, config/logs, structure, screenshots), badges bumped to
  v3.8. Все 8 README переписаны по расширенному шаблону, бейджи подняты
  до v3.8.

## [3.8] - 2026-09-12

### Changed / Изменено

- Killfeed cards: auto-width, gradient body, shadow, accent bar, dynamic
  killer → victim layout. Карточки киллфида: ширина по тексту, градиент,
  тень, акцент, динамическая вёрстка.
- Alert pills: pinned / tank / witch / spit / revive now render as centered
  cards; SI-nearby list got its own panel. Алерты — центрированные
  карточки; у списка SI рядом своя панель.

### Changed / Изменено

- Main README is English-only and rewritten (about, how it works, features,
  config/logs, structure, screenshots); Russian content lives in
  `README_RU.md`. Главный README только на английском и переписан
  (о проекте, устройство, фичи, конфиги, структура, скриншоты); русский —
  в `README_RU.md`.
- Loader cleanup: footer pills are visible/clickable again (window height
  fix), dead code and hardcoded dev paths removed. Чистка лоадера: футер
  снова виден и кликабелен (фикс высоты окна), мёртвый код и захардкоженные
  пути удалены.
- ESP boxes are now corner brackets with dimmed snaplines, bordered HP bar
  and shadowed names. Боксы ESP — уголки, приглушённые снаплайны, HP-бар
  с обводкой, ники с тенью.

## [3.7] - 2026-09-12

### Added / Добавлено

- German menu language (EN / RU / DE cycle via button + `F7`, saved as
  `menu.lang`) and `README_DE.md` linked from the main README.
  Немецкий язык меню (переключение кнопкой + `F7`, хранится в
  `menu.lang`) и `README_DE.md` со ссылкой из основного README.
- Spanish menu language (EN / RU / DE / ES cycle) and `README_ES.md`
  linked from the main README.
  Испанский язык меню и `README_ES.md` со ссылкой из основного README.
- Portuguese menu language (EN / RU / DE / ES / PT cycle) and
  `README_PT.md` linked from the main README.
  Португальский язык меню и `README_PT.md` со ссылкой из основного
  README.
- Polish menu language (EN / RU / DE / ES / PT / PL cycle) and
  `README_PL.md` linked from the main README.
  Польский язык меню и `README_PL.md` со ссылкой из основного README.
- French menu language (EN / RU / DE / ES / PT / PL / FR cycle) and
  `README_FR.md` linked from the main README.
  Французский язык меню и `README_FR.md` со ссылкой из основного
  README.
- Chinese menu language (EN / RU / DE / ES / PT / PL / FR / ZH cycle) and
  `README_ZH.md` linked from the main README.
  Китайский язык меню и `README_ZH.md` со ссылкой из основного README.
- Loader in all 8 languages (`g_nLang`, cycle button + footer pill,
  registry `Lang` 0-7 with old 1/2 mapped to RU/EN) and README split:
  English stays default in `README.md`, Russian moved to `README_RU.md`,
  flag-emoji language table on top.
  Лоадер на всех 8 языках и разделение README: английский по умолчанию
  в `README.md`, русский переехал в `README_RU.md`, таблица языков
  с флагами сверху.
- Weapon HUD under the crosshair: active weapon name, clip and reserve
  (`m_iAmmo`) + RELOADING text and red LOW AMMO at 5 rounds or less.
  HUD оружия под прицелом: имя, магазин, запас + RELOADING и красный
  магазин при ≤5 патронах.
- ESP max distance slider (0 = unlimited): cuts players/SI/items past
  the cutoff, cleaner screen and less per-frame work.
  Дистанция ESP (0 = без лимита) для всего ESP.
- Hide hands via `r_drawviewmodel` through ICvar (no console), restored
  on toggle off. Offsets skipped: float ConVar setter index unverified.
  Скрытие рук без консоли. Офсет рук не делаем: float-сеттер не проверен.
- Live sync while airborne: current strafe sync above the crosshair,
  no need to wait for landing.
  Live-синхрон прямо в полёте над прицелом.
- Inferno age in throwable timers (FIRE 12s) + red SPIT! MOVE alert when
  standing in spitter goo (~4.5m).
  Возраст огня молотова + алерт блевотины под ногами.

### Fixed / Исправлено

- Crash in Radar spectators (log: `EXCEPTION ... ZenWare.dll+0x12DF9`,
  `last breadcrumb: ESP::Render`): the spectator loop called
  `deadflag()/m_lifeState()` on ANY non-dormant entity without a ClassID
  check, so a half-created slot (map load) or a transient entity (in game)
  killed the process. Gated by `IsPlayerEntity`, same for the Radar local
  player. `Radar::Render` now sets its own breadcrumb so the next log
  points at the real site.
  Краш в спектаторах радара: чтение нетваров игрока с любых сущностей без
  проверки класса (падение при загрузке карты и в игре). Гейт
  `IsPlayerEntity` + свой breadcrumb у радара.
- Audit sweep (crashes + logic, whole project): bhop statics reset on
  death/spec/ladder/map-change; strafe and jump-stats ignore ladder/water/
  ghost/noclip; edge/EB/JB/showtick banners need real fires (no spawn
  false-positives); Killfeed no longer casts Witch/SI to C_TerrorPlayer and
  resolves bot names by class; IsPlayerEntity gates on every local-player
  lookup (Hitmarker/Alerts/Chams/Radar); Alerts pins/revive toggles work;
  crosshair hidden in menu; DrawManager null/exception guards; menu child
  checkboxes nested, no format-string-as-argument; WndProc F7 repeat filter;
  loader: atomic inject flag, -insecure launch, thread-handle/once fixes.
  Зачистка по аудиту: ресеты статиков бхопа, гарды лестниц/воды/гостов,
  честные баннеры багов, гейты IsPlayerEntity везде, рабочий киллфид по СИ,
  мелочи меню/лоадера (-insecure, атомарный флаг инжекта).
- Menu mouse: the OS cursor was hidden every frame while the menu was open,
  so there was no cursor without ESC/console (and clicks landed in the game
  menu afterwards). The cursor is now force-shown while open, camera stays
  still, no clicks reach the game (incl. side buttons).
  Мышь в меню: курсор прятался каждый кадр — теперь показывается всегда,
  камера стоит, клики в игру не уходят. ESC/консоль больше не нужны.
- Mid-game hardening: null guards on `EngineClient/ClientEntityList` in
  viewmodel-FOV, fog, crosshair, overlay; `CalcPlayerView` applies world FOV
  only to the class-checked local survivor.
  Защита от крашей в игре: проверки интерфейсов и класса локального игрока.
- Split FOV: world camera and weapon viewmodel have separate `x100` sliders
  now (old `visuals.viewfov` still loads into world). Full bright toggle via
  `mat_fullbright` without console (restored on toggle off, local server).
  FOV по отдельности: мир и руки — разные слайдеры. Фулбрайт без консоли.

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
