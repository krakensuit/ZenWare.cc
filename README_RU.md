# ZenWare.cc — Русский

**Internal + External софт для тренировок в Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.6`

> **Только исходники · только обучение и локальный сервер.**
> Никаких собранных `.exe` / `.dll` в репозитории и релизах — собери сам,
> запускай игру с `-insecure` и играй на своей карте (`map c1m1_hotel`).
> VAC-серверы = риск бана, риск твой.

🇬🇧 [English](README.md) · 🇩🇪 [Deutsch](README_DE.md) · 🇪🇸 [Español](README_ES.md) · 🇵🇹 [Português](README_PT.md) · 🇵🇱 [Polski](README_PL.md) · 🇫🇷 [Français](README_FR.md) · 🇨🇳 [中文](README_ZH.md)

## Русский

### О проекте

База — [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base). Три части, один солюшен (`ZenWare.sln`, `Release | Win32`):

| Часть | Выход |
|---|---|
| `ZenWare.DLL` — internal-модуль | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — GUI-лоадер | локальный `dist/ZenWare.exe`, не публикуется |
| `ZenWare.External` — оверлей без инжекта | `bin/Release/ZenWare.External.exe` |

### Возможности

<details open>
<summary><b>Visuals</b> — ESP · Chams · Оверлеи</summary>

- **ESP** — боксы, HP, ники, дистанция, оружие/предметы, обычные, все СИ включая бумера (фолбэк по именам)
- **Chams** — 5 палитр, союзники / враги / все СИ, сквозь стены
- **Мир** — NoFog, FOV мира + модели, 3-е лицо, прицел, FPS-оверлей
- **Инфо** — 2D-радар, наблюдатели, алерты танка / ведьмы, киллфид, хитмаркер (звук + цифры урона)
- **Команда** — алерт пина, алерт реанима, полоса HP танка, панель HP команды, список особых
- **Фидбэк** — крест попадания, стата сессии (хиты/выстрелы/точность), вспышка урона, мин. урон, таймеры гранат
- **Гранаты** — предпросмотр траектории (молотов / пайп / желчь / гранатомёт) с маркером падения

</details>

<details>
<summary><b>Combat</b> — аим</summary>

- **Aimbot** — silent, приоритет FOV / Distance, head / center, сглаживание, только видимые, обычные + все СИ
- **По оружию** — свои FOV / сглаживание / хитбокс на группу (винтовки, ПП, дробовики, снайперки, пистолеты)
- **Помощники** — TriggerBot, AutoShove, AutoPistol, NoSpread (отключаемый)

</details>

<details>
<summary><b>Movement</b> — баннихоп и стрейфы</summary>

- **Bhop-набор** — perfect / legit BunnyHop, EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- **Стрейф** — legit / rage / w-only / directional AutoStrafe

</details>

<details>
<summary><b>Меню / Система</b></summary>

- Меню `INSERT` · выгрузка `F11` · язык EN/RU/DE/ES/PT/PL/FR/ZH (кнопка + `F7`) · RGB-логотип · иконки `?` · табы · скролл колесом
- Конфиг — 3 слота (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`, `bool / int / float / Color`)
- Лог — `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (per-pid фолбэк)
- Кэш оффсетов — `<gamedir>/ZenWare.offsets` (авто, удали для перескана)

</details>

### Сборка и запуск — только локально

**1. Сборка**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Игра** — Steam → L4D2 → параметры запуска:

```text
-insecure -windowed -console
```

**3. Карта** — консоль игры:

```text
map c1m1_hotel
```

**4. Инжект** — собранный лоадер **от администратора** → `ИНЖЕКТ`.

Только External (без инжекта): твой локальный
`ZenWare.External/bin/Release/ZenWare.External.exe` или `START-ZenWare.bat`.

> Собранный `exe` в public Releases не загружай. Политика: [SECURITY.md](SECURITY.md).

### Требования

| Что | Версия |
|---|---|
| ОС | Windows 10/11 x64 |
| Игра | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Цель | `Release` · `Win32` (x86) |

### Клавиши

| Клавиша | Действие |
|:---:|---|
| `INSERT` | Меню |
| `F11` | Выгрузка |
| `F7` | Язык EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (держать) | BunnyHop (по умолчанию) |
| `MOUSE4` (держать) | Aimbot (по умолчанию) |

Смена: `Misc → Menu key / Aimbot key` — клик → `[press key]` → нажми клавишу, `ESC` = выкл.

### Структура

```text
ZenWare.cc/
├── ZenWare.sln                  солюшен (DLL + Loader + External)
├── ZenWare.DLL/                 internal-модуль (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/              GUI-лоадер — только локально, без автообновлений
├── ZenWare.External/            внешний оверлей — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               проверка сигнатур твоего client.dll
├── .github/workflows/           CI-проверка сборки — без выгрузки exe
├── dist/                        только локальный выход (игнорируется гитом)
├── Build-SingleFile.ps1         локальный сборщик (без Publish)
├── Verify-Signatures.bat        проверка паттернов в один клик
├── START-ZenWare.bat            локальный запуск External
├── README.md · README_RU.md · README_DE.md · README_ES.md · README_PT.md · README_PL.md · README_FR.md · README_ZH.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

### Частые проблемы

| Симптом | Решение |
|---|---|
| `XorString` при старте | Умер паттерн после обновы → дважды кликни `Verify-Signatures.bat`, обнови `Offsets.cpp` |
| Краш после `Paint` | Открой лог → найди `[!!!] EXCEPTION` → пришли `module+0x...`, без дампов |
| Антивирус | Папку в исключения (эвристика на `WriteProcessMemory` / `CreateRemoteThread`) |
| Квадратики вместо букв | Нажми `F7` (шрифт / язык) |
| External: нет боксов | Смотри верхние строки оверлея (диагностика), проверь сигнатуры |

---

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
