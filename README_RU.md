# ZenWare.cc — Русский

**Internal + External софт для тренировок в Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.8`

> **Только исходники · только обучение и локальный сервер.**
> Никаких собранных `.exe` / `.dll` в репозитории и релизах — собери сам,
> запускай игру с `-insecure` и играй на своей карте (`map c1m1_hotel`).
> VAC-серверы = риск бана, риск твой.

🇬🇧 [English](README.md) · 🇩🇪 [Deutsch](README_DE.md) · 🇪🇸 [Español](README_ES.md) · 🇵🇹 [Português](README_PT.md) · 🇵🇱 [Polski](README_PL.md) · 🇫🇷 [Français](README_FR.md) · 🇨🇳 [中文](README_ZH.md)

## Русский

### О проекте

ZenWare.cc — песочница для изучения внутренностей Left 4 Dead 2 (движок
Source, 2009): рендер, предикт, движение и сеть — на своём ПК, на своём
сервере, с `-insecure`. Обхода VAC нет, официальных серверов нет, собранных
бинарников в проекте нет.

Код вырос из [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
в три независимых инструмента в одном солюшене (`ZenWare.sln`, `Release | Win32`):

| Часть | Режим | Что делает | Выход |
|---|---|---|---|
| `ZenWare.DLL` | Internal (инжект) | Полный чит: уголки ESP, чамсы, аим, движение, меню в игре | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI-лоадер, только локальная сборка | Manual-map / LoadLibrary инжект, темы, 8 языков | локальный `dist/ZenWare.exe`, не публикуется |
| `ZenWare.External` | External (без инжекта) | Чтение RPM + GDI-оверлей + бхоп/стрейфы через `SendInput` | `bin/Release/ZenWare.External.exe` |

Новичок? Начни с **External** — он не пишет в память игры, это самый
безопасный способ изучить пайплайн.
Подробности: [SECURITY.md](SECURITY.md) · Стороннее: [NOTICE.md](NOTICE.md) ·
Лицензия: [LICENSE](LICENSE) · Устройство: [ARCHITECTURE.md](ARCHITECTURE.md).

### Как устроено

**Internal DLL.** При инжекте `DllMain` создаёт поток (логика никогда не
работает на лоадер-локе), `Entry::Load` идёт по порядку с отказом в
безопасную сторону: логгер → краш-рекордеры → ожидание `serverbrowser.dll` →
скан паттернов (с кэшем RVA `ZenWare.offsets`) → интерфейсы
(`VEngineCvar007` ищется сначала в `vstdlib.dll`) → нетвары → хуки
`MinHook` → загрузка конфига. Отсутствующий паттерн/интерфейс = окно с
ошибкой, а не краш.

Каждый кадр работают две цепочки:

- **Тик** (`ClientMode::CreateMove`) — предикт, движение (BunnyHop →
  AutoStrafe → JumpStats → AutoShove), бой (Aimbot → TriggerBot →
  AutoPistol → NoSpread). Оригинал движка вызывается ровно раз за тик,
  результат переиспользуется.
- **Кадр** (`EngineVGui::Paint`, только `PAINT_UIPANELS`) — квары
  thirdperson / fullbright / hide-hands, тик киллфида и хитмаркера, ESP,
  радар, алерты, меню, прицел, предпросмотр гранат, оверлей и отрисовка
  анимаций киллфида / хитмаркера / JumpStats.

Игровых ивентов движка нет by design — киллфид и хитмаркер опрашивают
переходы HP/alive. Рисование только через фасад `G::Draw` поверх
`MatSystemSurface`; чтение сущностей — только нетвары с проверкой
`ClassID` перед любым виртуальным вызовом.

**Лоадер.** Win32 GUI со сплэшем, системной тёмной/светлой темой, 8 языками
и анимированным RGB-лого. DLL, External и лого вшиты ресурсами — на выходе
один файл. Инжект — manual-map (релокации, импорты, защита секций, затирание
заголовков) с фолбэком на `LoadLibrary`. Сети нет, автообновления нет.

**External.** Только чтение: снапшоты `ReadProcessMemory` на 20 Гц,
click-through GDI-оверлей на 60 Гц, движение только `SendInput`.
Резолвер (сигнатура + якорь + валидация) находит адреса при каждом запуске.

### Возможности

<details open>
<summary><b>Visuals</b> — ESP · чамсы · оверлеи</summary>

- **ESP** — уголки (цвет по команде), HP-бар с обводкой + опциональный текст
  HP, ники с тенью, дистанция, оружие с патронами в магазине, предметы,
  обычные, все особые включая бумера (фолбэк по именам классов), лимит
  дистанции ESP, приглушённые снаплайны
- **Чамсы** — 5 палитр, отдельные цвета врагов / союзников / танка, сквозь
  стены, фильтр призраков и дорманта
- **Мир** — NoFog, Fullbright, раздельные слайдеры FOV мира и модели,
  3-е лицо с дистанцией, скрытие рук, свой прицел, оверлей FPS / позиции /
  HP, счётчик живых обычных
- **Инфо** — 2D-радар с поворотом по yaw, список наблюдателей, алерты танка
  и ведьмы, опрашиваемый киллфид с анимированными карточками, хитмаркер со
  звуком, цифрами урона, крестом попадания и статой сессии
- **Команда** — предупреждение о пине (курильщик/охотник), алерт реанима,
  полоса HP танка, панель HP команды, список особых рядом со своей панелью,
  алерт блевотины, таймеры брошенных гранат (пайп / огонь молотова)
- **Фидбэк** — красная вспышка урона, стрелка в сторону последнего
  атакующего, фильтр мин. урона, HUD оружия с патронами (магазин + запас),
  жёлтое `RELOADING` и красное предупреждение о малых остатках
- **Гранаты** — предпросмотр траектории (молотов / пайп / желчь /
  гранатомёт) с симуляцией отскоков и маркером падения

</details>

<details>
<summary><b>Combat</b> — аим</summary>

- **Аимбот** — silent, приоритет FOV / Distance, точка head / center,
  сглаживание, только видимые, клавиша, обычные + все СИ + танк
- **По оружию** — свои FOV / сглаживание / хитбокс на группу (винтовки, ПП,
  дробовики, снайперки, пистолеты)
- **Помощники** — TriggerBot (стреляет по лучу после аима, без лага в тик),
  AutoShove (освобождает запиненных), AutoPistol, NoSpread (вайтлист
  11 стволов, включён по умолчанию)

</details>

<details>
<summary><b>Movement</b> — баннихоп и стрейфы</summary>

- **Bhop-набор** — perfect / legit BunnyHop, EdgeJump, EdgeBug, JumpBug,
  помощник LongJump, FastStop, Prestrafe, AutoDuck, настраиваемая задержка
  прыжка
- **Стрейф** — legit / rage / w-only / directional AutoStrafe
- **JumpStats** — панель в стиле KZ: дистанция, престрейф, макс. скорость,
  падение, высота прыжка, время в воздухе, live-синхрон % в полёте, счёт
  стрейфов, счётчики JB / EB за сессию

</details>

<details>
<summary><b>Меню / Система</b></summary>

- Меню в игре (`INSERT`): 5 табов (Visuals / Move / View / Combat / Misc),
  скролл в табах, RGB-лого, подсказки `?` (EN + RU), свотчи цветов, бинды
  клавиш, слайдеры, паник-кнопка мгновенно прячет ESP
- 8 языков с переключением на лету (кнопка или `F7`):
  EN / RU / DE / ES / PT / PL / FR / ZH
- Конфиг — 3 слота (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`),
  `bool / int / float / Color`, float-слайдеры через int-прокси с ресинком
- Логгер — `%TEMP%/ZenWare.log` переезжает в `<gamedir>/ZenWare.log`
  (per-PID фолбэк), краш-записи вида `module+offset` с хлебными крошками
- Кэш оффсетов — `<gamedir>/ZenWare.offsets` (авто, удали для перескана);
  `Tools/SigScan` проверяет все 15 паттернов оффлайн по твоим файлам игры

</details>

### Сборка и запуск — только локально

**1. Сборка** (Visual Studio 2022 с `Desktop development with C++`):

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Игра** — Steam → L4D2 → параметры запуска:

```text
-insecure -windowed -console
```

**3. Карта** — консоль в игре:

```text
map c1m1_hotel
```

**4. Инжект** — запусти собранный лоадер **от администратора** → `INJECT`.

Только external (без инжекта): запусти локальный
`ZenWare.External/bin/Release/ZenWare.External.exe` или `START-ZenWare.bat`.

> Не заливай собранный `exe` в публичные GitHub Releases. Политика только исходников: [SECURITY.md](SECURITY.md).

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
| `INSERT` | Открыть / закрыть меню |
| `F11` | Выгрузить |
| `F7` | Язык EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (держать) | BunnyHop (по умолчанию) |
| `MOUSE4` (держать) | Аимбот (по умолчанию) |

Переназначение: `Misc → Menu key / Aimbot key` — клик → `[press key]` → нажми клавишу, `ESC` = выкл.
Паник-кнопка мгновенно прячет боксы (`Visuals → ESP`).

### Конфиги и логи

- Слоты рядом с путём DLL игры: `ZenWare.cfg`, `ZenWare2.cfg`,
  `ZenWare3.cfg` — plain `key=value`, 117 ключей. Сессионные значения
  (счётчики JB/EB) не сохраняются.
- Лог: `<gamedir>/left4dead2/ZenWare.log`. Краш выглядит как
  `[!!!] EXCEPTION code=... at module+0x...` + последняя крошка — этой пары
  хватает, дампы не нужны.
- Кэш сигнатур: `<gamedir>/left4dead2/ZenWare.offsets` (RVA + отпечаток
  модуля); после обновления игры удали его или запусти
  `Verify-Signatures.bat` и поправь `Offsets.cpp` при `NOT FOUND`.

### Структура проекта

```text
ZenWare.cc/
├── ZenWare.sln                  солюшен (DLL + Loader + External)
├── ZenWare.DLL/                 internal-модуль
│   ├── src/Entry/               порядок инициализации, краш-рекордеры, выгрузка
│   ├── src/Hooks/               ClientMode / EngineVGui / ModelRender / WndProc …
│   ├── src/SDK/                 интерфейсы L4D2, сущности, DrawManager, GameUtil
│   ├── src/Features/            Vars (настройки) + Aimbot/ESP/Menu/… + Config + Lang
│   └── src/Util/                Logger, Pattern, кэш Offsets, NetVars, MinHook
├── ZenWare.Loader/              GUI-лоадер — только локальная сборка, без автообновлений
├── ZenWare.External/            external-оверлей — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               проверка сигнатур по твоему client.dll
├── .github/workflows/           CI-проверка сборки — без выгрузки бинарников
├── dist/                        только локальный вывод (git-ignored)
├── Build-SingleFile.ps1         локальный сборщик (без Publish)
├── Verify-Signatures.bat        проверка паттернов по файлам игры в один клик
├── START-ZenWare.bat            локальный запуск external
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
└── README_RU/DE/ES/PT/PL/FR/ZH.md   переводы этого файла
```

### Проблемы

| Симптом | Решение |
|---|---|
| `XorString` / `FATAL ERROR` при старте | Сигнатура сломалась после обновления игры → `Verify-Signatures.bat`, обнови `Offsets.cpp` |
| Краш в игре | Открой лог → найди `[!!!] EXCEPTION` → пришли `module+0x...` и последнюю крошку, дампы не нужны |
| Ругается антивирус | Папка → исключения (эвристика на `WriteProcessMemory` / `CreateRemoteThread`) |
| Квадраты вместо букв | Нажми `F7` (фолбэк шрифта / язык) |
| External: нет боксов | Смотри верхние строки оверлея (диагностика резолвера) и сигнатуры |
| Меню открыто, а клики уходят в игру | Сначала закрой игровое меню (чит обрабатывает `ESC`, только когда открыто его меню) |

## Право

| Тема | Условия |
|---|---|
| Лицензия | MIT — [LICENSE](LICENSE), `AS IS`, без гарантий |
| Стороннее | [NOTICE.md](NOTICE.md): MinHook, FontAwesome, заголовки Valve, база |
| Товарные знаки | Left 4 Dead 2 / Steam принадлежат Valve. Не связан, не одобрен |
| Распространение | Только исходники — [SECURITY.md](SECURITY.md), без бинарников |
| Использование | Только локальный `-insecure`. Обхода VAC нет. Баны — твои |

<div align="center">

**Авторы** ·
База: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Хуки: MinHook (Tsuda Kageyu, BSD-style) ·
Иконки: [FontAwesome 6 Free](https://fontawesome.com)

</div>
