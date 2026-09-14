# Changelog / История версий

All notable changes to ZenWare.cc are documented here.
Все заметные изменения ZenWare.cc — здесь.

## [Unreleased]

## [3.8.14] - 2026-09-14

### Fixed / Исправлено
- JumpStats sync and the strafe counter were dead with AutoStrafe on: the side
  snapshot was taken before AutoStrafe writes `cmd->sidemove`, so every landing
  read `sync 0%` and `0 strafes`. Sync now measures the strafe that is actually
  applied this tick (raw mouse X is still used for the direction check).
  Счётчик стрейфов и sync в JumpStats не работали при включённом AutoStrafe:
  снимок стороны стрейфа делался до записи `cmd->sidemove`, поэтому каждая
  посадка показывала `sync 0%` и `0 strafes`. Теперь синк считается по
  фактически применённому стрейфу (для проверки направления по-прежнему
  берётся сырой `mousedx`).
- Audit sweep (commit 92cf318): 22 code fixes plus documentation aligned with
  the code — null-address guards for StartDrawing/FinishDrawing, m_iAmmo read as
  an inline int[32], pattern-scan window (BaseOfCode + SizeOfCode), overlay DIB
  recreated only on resize, loader LogPacket/KeyValues leaks, PE section-table
  and import-thunk bounds, TriggerBot visible-only wired (default off), docs
  numbers corrected (121 config keys, 14-gun NoSpread, MOUSE4 = bhop alt key).
  Аудит-правки (коммит 92cf318): 22 исправления в коде и приведение
  документации к коду — защита от нулевых адресов StartDrawing/FinishDrawing,
  m_iAmmo как встроенный int[32], окно pattern-скана, оверлей без пересоздания
  DIB каждый кадр, утечки LogPacket/KeyValues в загрузчике, границы таблицы
  секций и импортов, рабочая галка TriggerBot (по умолчанию выключена), цифры
  в документации (121 ключ конфига, 14 стволов в NoSpread, MOUSE4 = алт-клавиша
  распрыжки).


## [3.8.13] - 2026-09-13

### Fixed / Исправлено
- Netvar manager now validates the declared type size against the real prop
  size (RECVINFO stores `sizeof(field)` in `m_StringBufferSize` for every prop
  type) and logs every mismatch as
  `[!] netvar Class.Prop: real prop size N, declared M — FIX THE SDK TYPE`.
  This is the systematic guard for the mis-typed-netvar class of bugs (the
  m_nWaterLevel int-over-byte that randomly killed the bhop). Startup
  netvar diagnostics pass the declared sizes too. Менеджер нетваров сверяет
  объявленный размер типа с реальным размером пропа и пишет каждое
  несовпадение в лог — системная защита от класса багов «int поверх байта»
  (как m_nWaterLevel). Стартовая диагностика нетваров тоже передаёт размеры.
- Mis-typed netvars fixed: `m_ubEFNoInterpParity` int → unsigned byte
  ("ub" prefix, byte prop); removed `m_szLastPlaceName` declared as
  `const char*` (it is a char array in the game, the old declaration aliased
  the first 4 characters as a pointer; unused); `C_Infected::m_nWaterLevel`
  int → unsigned byte (same byte prop as CBasePlayer's).
  Исправлены неверно типизированные нетвары: m_ubEFNoInterpParity (байт),
  убран m_szLastPlaceName (массив, а не указатель), C_Infected::m_nWaterLevel
  (байт).
- `GetPlayerInfo` calls in Radar/Alerts/ESP::DrawTeam are now gated by
  `GetMaxClients()` (entity indices above the client slots are bot SI —
  same guard Killfeed already documented and used). Вызовы GetPlayerInfo в
  радаре/алертах/панели команды ограничены maxclients (тот же гейт, что уже
  документирован в Killfeed).

### Known crashes / Известные вылеты
- All 7 exceptions recorded in the game log (0x12D09/0x12DF9 ESP::Render,
  0x5AD5 Aimbot commons, 0x12979, 0x1B36D, one inside engine.dll) happened on
  builds 3.8.7 and earlier and are covered by those fixes; the eight
  sessions on 3.8.8+ contain zero exceptions. Все 7 исключений в логе
  произошли на сборках 3.8.7 и ранее и покрыты их фиксами; восемь
  сессий на 3.8.8+ — ни одного исключения.

## [3.8.12] - 2026-09-13

### Fixed / Исправлено
- **THE bhop root cause, found in the user's own game logs**: `m_nWaterLevel`
  is a one-byte prop (the resolved offset 0x146 sits right before
  `m_lifeState` at 0x147), but the SDK header declared it as a 4-byte `int` —
  every read produced garbage like 0x777B0080, and the bhop gate
  `m_nWaterLevel() > 1` silently disabled the bhop on ~half of all ticks.
  That single mis-typed netvar explains every bhop report since 3.8.4
  ("sometimes jumps, sometimes does not at all"), the broken JumpStats
  landings in recent sessions, and why the prediction fixes of 3.8.9-3.8.11
  changed nothing. The accessor is now `unsigned char`, and all three water
  gates (BunnyHop/AutoStrafe/JumpStats) tolerate offset drift: a byte outside
  0..3 is treated as garbage and does not gate. **Корень бхопа найден в
  логах пользователя**: m_nWaterLevel — однобайтовый проп (оффсет 0x146, сразу
  перед m_lifeState 0x147), а в заголовке он был объявлен как 4-байтовый int —
  каждое чтение давало мусор, и гейт `m_nWaterLevel() > 1` молча выключал
  бхоп примерно на половине тиков. Один неверно типизированный нетвар
  объясняет все жалобы на бхоп с 3.8.4 и то, что фиксы предикта 3.8.9-3.8.11
  ничего не изменили. Теперь доступ — unsigned char, а все три водяных гейта
  толерантны к дрейфу оффсета (байт вне 0..3 = мусор, не гейтит).
- Logger: the log file is now opened with `_SH_DENYWR`, so it stays readable
  by other processes while the game writes it — live diagnosis during a
  session is possible again (the exclusive lock made the log unreadable even
  for reading). Лог открывается с `_SH_DENYWR` и читается другими процессами
  прямо во время игры (эксклюзивная блокировка делала файл нечитаемым даже
  на чтение).
- BunnyHop: temporary ground/air transition logging (`[bh] ground/air tick=…
  cmd=… flags=… jump_in_cmd=…`) for the next diagnostic session — a few lines
  per hop, removed once the behavior is confirmed. Временный лог переходов
  земля/воздух в BunnyHop для контрольной сессии (несколько строк на прыжок).

## [3.8.11] - 2026-09-13

### Fixed / Исправлено
- Removed the manual engine-prediction wrapper from CreateMove entirely. It ran
  `SetupMove/ProcessMovement/FinishMove` on the cmd being built, so every cmd
  was simulated twice (our run + the engine's own prediction) and half the
  player state stayed advanced afterwards (`m_hGroundEntity`, fall velocity,
  duck time were never restored) — the ground-entity/flags desync broke
  `CheckJumpButton` and made the bhop janky or dead. Reference implementations
  (CSGOSimple, l4d2-internal-base) run movement on the engine's own predicted
  state and only touch `cmd->buttons`; ZenWare does the same now. Movement and
  combat both read the freshest predicted flags/origin in CreateMove.
  Ручной prediction-wrap убран из CreateMove полностью: он прогонял
  SetupMove/ProcessMovement/FinishMove по ещё не отправленной cmd — каждый тик
  симулировался дважды, и половина состояния игрока оставалась продвинутой
  (ground entity, скорость падения, duck-время не восстанавливались).
  Рассинхрон ground-entity с флагами ломал CheckJumpButton — бхоп прыгал
  криво или не прыгал. Референсы (CSGOSimple, l4d2-internal-base) работают на
  собственном предикте движка и трогают только cmd->buttons — теперь так же.

## [3.8.10] - 2026-09-13

### Fixed / Исправлено
- Movement features ran INSIDE the prediction wrap, after `ProcessMovement` had
  already simulated the current cmd: on a grounded tick with jump held the
  simulation itself executed the jump, flags flipped to airborne, and BunnyHop
  then stripped `IN_JUMP` from the cmd — the jump only ever existed inside the
  rolled-back simulation, so the bhop never fired (3.8.9 regression; the 3.8.8
  flakiness came from the same misplaced order plus double simulation).
  BunnyHop/AutoStrafe/JumpStats now run BEFORE the wrap, on exactly the state
  the engine will simulate this cmd from; combat features stay inside the wrap.
  Фичи движения работали внутри prediction-wrap'а после того, как
  `ProcessMovement` уже просимулировал текущую cmd: на наземном тике с зажатым
  прыжком симуляция сама исполняла прыжок, флаги уезжали в «воздух», и BunnyHop
  вырезал IN_JUMP — прыжок существовал только в откатываемой симуляции, бхоп
  не стрелял вовсе (регрессия 3.8.9; рваность 3.8.8 — тот же порядок плюс
  двойная симуляция). BunnyHop/AutoStrafe/JumpStats теперь до wrap'а, на
  состоянии, с которого движок начнёт тик; бой остался внутри.
- Removed the temporary BunnyHop debug logging (cause found and fixed).
  Убран временный дебаг-лог BunnyHop (причина найдена и устранена).

## [3.8.9] - 2026-09-13

### Fixed / Исправлено
- EnginePrediction: full state snapshot/restore. Previously flags/tickbase were
  rolled back inside `Start` while origin/velocity stayed advanced and were never
  restored — movement features read one-tick-stale ground flags (bhop randomly
  skipped landings, "does not jump at all") and `ProcessMovement` ran on an
  already-advanced state (jerky "triple jumps", garbage jump distance, dead
  JumpBug window, JumpStats filter discarding every jump). EnginePrediction:
  полный снапшот/откат состояния — раньше флаги откатывались в `Start`, а
  origin/velocity оставались продвинутыми и не восстанавливались: фичи движения
  читали устаревшие флаги земли (бхоп пропускал посадки, «не прыгает»), а
  движение симулировалось по сдвинутому состоянию (рваные прыжки, мусорная
  дистанция, мёртвое окно JumpBug, стата отбрасывала все прыжки).
- JumpStats: takeoff is measured from the last ground tick (ground origin +
  ground speed snapshot) instead of the first airborne tick — distance and
  prestrafe no longer systematically undershoot by one tick. Взлёт статы
  считается от последнего наземного тика — дистанция и престрейф больше не
  занижены на тик.

### Changed / Изменено
- Visual polish: ESP boxes got a soft two-layer glow; the health bar is now a
  vertical gradient; the custom crosshair draws a dark outline around its arms
  (readable on snow/sky); the jump stats panel sits on a translucent backing
  card with a green/grey verdict accent. Визуальный полиш: мягкое двухслойное
  свечение ESP-боксов, градиентная полоса HP, тёмная обводка прицела (видно на
  снегу/небе), карточка-подложка панели статы прыжка с зелёным/серым акцентом.

## [3.8.8] - 2026-09-13

### Fixed / Исправлено
- Loader manual-map stub: 23 bytes, not 22 — the trailing `ret` never reached the
  target, so the remote thread executed zeros right after a "successful" DllMain
  (AV in the game). Stub layout check added (fail-closed). В стабе manual-map
  было 22 байта вместо 23: финальный `ret` не попадал в цель, поток после
  возврата из DllMain исполнял нули (AV в игре).
- External resolver: `SecHdr` now matches `IMAGE_SECTION_HEADER` (40 bytes; the
  two WORD pairs were `uint32_t`) — section iteration drifted +4 per step, so
  `.data` was never found and the runtime resolver silently always fell back to
  hardcoded offsets. `SecHdr` в резолвере совпадает с реальным заголовком
  секции: раньше `.data` не находилась и резолв всегда падал на хардкод.
- External resolver: LocalPlayer signature now validates the rebased disp32
  against the actual module base instead of the preferred `ImageBase` — on
  ASLR-relocated builds the check silently failed. Сигнатурная ветка LocalPlayer
  сверяет перебазированный адрес с фактической базой модуля, а не с preferred
  `ImageBase` (на ASLR-билдах проверка молча проваливалась).
- Loader: `g_busy` captured BEFORE DLL extraction in `StartInject`; `LaunchExternal`
  guarded too — a double click no longer runs two `WriteTempFile` racing on the
  same `%TEMP%` path or launches two overlay processes. `g_busy` берётся до
  распаковки ресурсов; LaunchExternal тоже под guard — двойной клик больше не
  гоняет две распаковки в один путь и два процесса оверлея.
- Loader: untrusted-PE bounds in manual-map — `SizeOfHeaders`, relocation block
  overrun, import directory/descriptor/thunk RVAs validated before use (heap OOB
  reads on a corrupt image). Границы недоверенного PE в manual-map: SizeOfHeaders,
  выход блока релокаций за каталог, RVA импортов проверяются до использования.
- Loader: language persists in a new `Lang2` registry value; legacy `Lang`
  migration applied once — EN/DE selection no longer reverted to RU/EN on restart.
  Язык UI хранится в новом значении `Lang2`; миграция старого `Lang` одноразовая —
  выбор EN/DE больше не сбрасывался на RU/EN после перезапуска.
- Loader: `InjectStandard` read-back clamped to the buffer size; dead
  `..\..\` fallback path to the External exe fixed (`..\..\..\`); `patch_theme.ps1`
  (foreign hardcoded path, stale targets) removed. Read-back в InjectStandard
  клампится в размер буфера; починен мёртвый fallback-путь к External.exe;
  удалён мёртвый `patch_theme.ps1`.
- Build-SingleFile.ps1: stale "updates arrive automatically" line (the updater
  was removed) dropped from the generated KAK-ZAPUSTIT.txt. Из генерируемого
  KAK-ZAPUSTIT.txt убрана строка про авто-обновления (апдейтер удалён).
- Verify-Signatures.bat actually runs now: unquoted parens in an `echo` inside
  nested `(...)` blocks were parsed as a block terminator (". was unexpected");
  `cmd /c` quote-stripping chopped the quoted vswhere path into "C:\Program"
  (fixed with a `call` prefix and hardcoded paths first); engine.dll and
  vguimatsurface.dll are resolved from the game ROOT `bin\`, not
  `left4dead2\bin` where they do not exist. Verified against the local game
  build: 14 patterns OK, 0 failed (CheckForSequenceChange AMBIGUOUS is known
  and its hook is disabled). Verify-Signatures.bat теперь реально работает:
  незакавыченные скобки в `echo` внутри вложенных блоков рвали парсинг блока;
  `cmd /c` срезал кавычки у пути vswhere (лечится префиксом `call`, захардкоженные
  пути проверяются первыми); engine.dll и vguimatsurface.dll берутся из
  корневого `bin\` игры, а не из `left4dead2\bin`. Проверено на локальном билде:
  14 паттернов OK, 0 failed.
- Entry: `std::string` for the missing-interfaces report; DllMain aborts with a
  message box if `CreateThread` fails; 30 s bounded wait for `serverbrowser.dll`.
  (параллельная сессия) Entry: отчёт о недостающих интерфейсах на `std::string`;
  DllMain падает с сообщением, если `CreateThread` не сработал; ожидание
  `serverbrowser.dll` ограничено 30 секундами.

## [3.8.7] - 2026-09-13

### Fixed / Исправлено
- ESP::Render crash guard (`IsPlayerEntity` + `GetClientClass` check before any virtual/netvar access) — устранён краш `0x12DF9`/`0x12D09`.
- Aimbot::FindCommonTarget дополнительная валидация (`ClassID == Infected`) — устранён краш `0x5AD5` (`Aimbot:common`).
- BunnyHop: `s_nJumpDelayTicks` статик + защита счётчика на смену карты/смерть.
- ESP/Aimbot: версия везде `v3.8.7` (`resource.h`, `README_*`, `CHANGELOG`).

## [3.8.4] - 2026-09-12

### Fixed / Исправлено

- AutoShove never fired: tongue owner (Smoker) and pounce attacker (Hunter)
  are SI classes, rejected by the old `IsPlayerEntity` gate — now checked
  by ClassID, attacker handled via netvars only. Автошов не срабатывал:
  держатель языка и автор прыжка — классы СИ, старый гейт их резал.
- Chams no longer reads the `CTerrorPlayer` ghost netvar on SI/Tank
  (garbage could silently hide their chams). Чамсы больше не читают
  нетвар ghost на СИ/танке.
- `GetEyePosition` widened to `C_BaseEntity` (netvars only, no virtuals).
  `GetEyePosition` расширен до `C_BaseEntity` (только нетвары).

## [3.8.3] - 2026-09-12

### Fixed / Исправлено

- Menu widgets that ignored clipping (section labels, color swatches,
  int labels) no longer draw outside the panel. Виджеты меню, игнорировавшие
  клиппинг, больше не вылезают за панель.
- English is the default language in the cheat and the loader (saved
  choice still wins); language switch lives in Misc + F7.
  Английский по умолчанию в чите и лоадере (сохранённый выбор важнее);
  переключение — в Misc + F7.
- Netvar offset diagnostics at init (see `netvar` lines in ZenWare.log) —
  send the log tail after a crash. Диагностика оффсетов нетваров при
  старте — пришли хвост лога после краша.

## [3.8.2] - 2026-09-12

### Fixed / Исправлено

- Audit sweep: melee-vtable gates (`IsGunEntity` before `GetWeaponID` in
  ESP/WeaponHUD, ClassID gate in DrawGrenade), SI handled via netvars
  (Killfeed/Chams, no `C_TerrorPlayer` virtuals on siblings), SI/common
  kills counted in Hitmarker, damage flash/arrow/stats no longer gated on
  outgoing damage, prediction time restore behind `m_bInPrediction` flag,
  AutoShove runs after Aimbot, aimbot hitbox/prio clamps, trigger uses
  crosshair-trace hit as visibility proof + dormant check, NoSpread spread
  sanity, JB/EB traces require real hit, water/incap bails, EdgeJump disarm
  on reset, JumpStats takeoff arming + strafe counting + counter toggle
  gates, AutoStrafe statics reset, Config::Load clamps + Save resync,
  menu capture/XBUTTON/F11/drag/focus fixes, loader OOB fix, CTable scan
  cap + bounds, DrawManager font lookup, CreateMove cmd check before
  original. Аудит: гейты виртуалок на меле, СИ через нетвары, киллы СИ
  в хитмаркере, флаг предикта, порядок шова, клампы конфига и меню,
  фикс OOB лоадера, капы CTable, проверки DrawManager.

## [3.8.1] - 2026-09-12

### Added / Добавлено

- Spread circle around the crosshair (live NoSpread radius), per-class SI
  colors for ESP and chams, bind list panel, damage log panel (dealt,
  kills, incoming). Круг разброса у прицела, цвета классов SI для ESP и
  чамсов, панель биндов, лог урона (нанесённый, киллы, входящий).

### Changed / Изменено

- All 8 READMEs rewritten with the expanded template (about, how it works,
  detailed features, config/logs, structure), screenshots sections removed,
  badges bumped to v3.8. Все 8 README переписаны по расширенному шаблону,
  секции скриншотов удалены, бейджи подняты до v3.8.

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
