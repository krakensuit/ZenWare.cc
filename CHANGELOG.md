# Changelog / История версий

All notable changes to ZenWare.cc are documented here.
Все заметные изменения ZenWare.cc — здесь.

## [Unreleased]

### Fixed / Исправлено

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
