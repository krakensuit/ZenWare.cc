# Changelog / История версий

All notable changes to ZenWare.cc are documented here.
Все заметные изменения ZenWare.cc — здесь.

## [4.1.2] - 2026-09-16

### Fixed
- The loader title rainbow ended at pink and restarted abruptly. This time the fault was in the brush itself, not in the animation: the gradient was built from six stops covering hue 0 to 300 with positions spread across the full 0..1 range, so the brush's own right edge was pink and immediately after it the brush repeated from red. Sliding that window across the text produced exactly the visible seam - the colour reached pink and snapped back. The brush is now cyclic: a seventh stop at hue 360, which is the same colour as hue 0, closes the loop, so the sliding window is continuous everywhere.
  Радуга заголовка в лоадере заканчивалась на розовом и резко начиналась заново. На этот раз дефект был в самой кисти, а не в анимации: градиент собирался из шести стопов по оттенкам от 0 до 300 с позициями, растянутыми на весь диапазон 0..1, поэтому правый край кисти был розовым, а сразу за ним кисть повторялась с красного. Скольжение такого окна по тексту и давало видимый шов - цвет доходил до розового и срывался назад. Теперь кисть циклическая: седьмой стоп на оттенке 360, то есть того же цвета, что и 0, замыкает петлю, поэтому скользящее окно непрерывно везде.
- Together with the wrapped shift from 4.1.1 (which now wraps exactly at the brush span) the loader title sweep is fully seamless: the brush is cyclic and the window slides exactly one full cycle.
  Вместе с завёрнутым сдвигом из 4.1.1 (он теперь оборачивается точно по ширине кисти) проход заголовка лоадера полностью бесшовный: кисть циклическая, а окно сдвигается ровно на один полный цикл.

## [4.1.1] - 2026-09-16

### Fixed
- The loader title rainbow still snapped to pink mid-cycle. The per-letter colours were already seamless, but the gradient fallback brush used `fmodf(TitleHue() * 2.0f, span)`: taking a remainder by the brush span cuts the continuous phase at an arbitrary point whenever the span does not divide the doubled hue range evenly, so the visible gradient jumped once per sweep - exactly at the pink section. The shift is now accumulated in its own wrapped phase (`TitleShift(span)`) that wraps exactly at the span, so the sweep runs forever without a seam.
  Радуга заголовка в лоадере всё ещё срывалась в розовый посреди цикла. Цвета букв уже были бесшовными, но градиентная кисть-фолбэк использовала `fmodf(TitleHue() * 2.0f, span)`: остаток по ширине кисти режет непрерывную фазу в произвольной точке, если ширина не делит удвоенный диапазон оттенка нацело, поэтому видимый градиент прыгал раз за проход - ровно на розовом участке. Теперь сдвиг копится в собственной завёрнутой фазе (`TitleShift(span)`), которая оборачивается точно по ширине кисти, поэтому проход идёт вечно без шва.

## [4.1.0] - 2026-09-16

### Added
- A real backdrop blur behind the menu panel. While the menu is open the frame region directly under the panel is copied out of the game's render target, averaged down to half resolution (which also swaps the framebuffer's BGRA byte order into the RGBA the surface expects), box blurred twice with a radius of three on the small buffer, uploaded as a procedural texture and drawn 1:1 behind the panel. The panel fill is softened to three quarters of its configured alpha while the blur is active, so the frozen world behind the menu actually reads through.
  Настоящее размытие за панелью меню. Пока меню открыто, область кадра непосредственно под панелью копируется из render target игры, усредняется до половинного разрешения (заодно порядок байт BGRA фреймбуфера меняется на RGBA, который ждёт поверхность), дважды размывается box-фильтром с радиусом три на маленьком буфере, загружается как процедурная текстура и рисуется 1:1 за панелью. Заливка панели на время активного блюра смягчается до трёх четвертей заданной альфы, чтобы застывший мир за меню действительно проглядывал.

### Safety
- Every step of the blur is guarded, and when anything is unavailable - no material system, no render context, an allocation failure, a failed texture id - the code falls back to the previous layered scrim, exactly as in 4.0.0. A failure in the blur path degrades the picture; it cannot take the menu or the game down.
  Каждый шаг размытия защищён, и если что-то недоступно - нет material system, нет render context, не выделилась память, не создался id текстуры - код откатывается на прежнюю слоистую засветку, как в 4.0.0. Сбой в пути размытия ухудшает картинку; уронить меню или игру он не может.

### Note
- The blur refreshes every frame while the menu is open; at half resolution with two radius-3 passes the CPU cost is around a millisecond per frame on the panel-sized region. If the frozen backdrop ever looks tinted, the BGRA-to-RGBA swap in the downsample loop is the single knob to check.
  Размытие обновляется каждый кадр, пока меню открыто; на половинном разрешении с двумя проходами радиуса три стоимость на CPU - около миллисекунды на кадр для области панели. Если застывший фон когда-нибудь покажется тонированным, единственный подозреваемый - перестановка BGRA в RGBA в цикле даунсэмплинга.

## [4.0.0] - 2026-09-16

### Highlights
- This release closes the whole interface rework and the two bugs that caused the most visible complaints, so the menu no longer looks or behaves like a stock internal cheat. Interface work: custom Inter / JetBrains Mono / Font Awesome faces embedded in the DLL, a style token layer (`Theme.h`) and an 8 px layout grid (`Layout.h`), a 400x560 panel with a 48 px header, pill tabs, a footer and one single definition of the content top, controls rebuilt on those tokens, tab icons, and a frosted backdrop behind the panel.
  Этот релиз закрывает всю переработку интерфейса и два дефекта, которые вызывали больше всего жалоб, поэтому меню больше не выглядит и не ведёт себя как типовой внутренний чит. По интерфейсу: вшитые в DLL начертания Inter / JetBrains Mono / Font Awesome, слой токенов стиля (`Theme.h`) и сетка 8 px (`Layout.h`), панель 400x560 с шапкой 48 px, вкладки-pill, футер и одно определение верха контента, контролы на этих токенах, иконки вкладок и морозная подложка за панелью.
- The frame counter is honest now: the menu used to take its timing from the game (`I::GlobalVars->frametime`), and while the game was paused that value is zero, so the old guard substituted 1/60 and the watermark and footer printed a fake "60 fps". The menu now runs on its own clock and shows a frame rate it actually measured.
  Счётчик кадров теперь честный: меню брало время у игры (`I::GlobalVars->frametime`), а на паузе это значение равно нулю, поэтому старая заглушка подставляла 1/60 и ватермарк с футером печатали фальшивые «60 fps». Теперь меню живёт на своих часах и показывает фреймрейт, который действительно измерило.
- The rainbow no longer snaps anywhere in the product. Both the loader wordmark and every animated accent used a phase that either grew inside a float (losing resolution with uptime) or was cut by `GetTickCount64() % 100000`, which reset it every 100 seconds. All of them now accumulate a frame delta in a small wrapped phase, so the cycle is endless and seamless.
  Радуга больше нигде в продукте не срывается. И надпись лоадера, и все анимированные акценты использовали фазу, которая либо росла внутри float (теряя точность с аптаймом), либо обрезалась остатком `GetTickCount64() % 100000`, обнулявшим её каждые 100 секунд. Теперь все они копят дельту кадра в небольшой завёрнутой фазе, поэтому цикл вечный и бесшовный.
- A game-independent surface exists: a dedicated thread in the DLL owns its own always-on-top, click-through window with its own clock and its own frame rate, without a single game hook, so a pause, a stalled game loop or a fault there cannot freeze it or take the game down. The menu itself still paints inside the game frame; porting its drawing onto this surface is the next stage.
  Появилась независимая от игры поверхность: отдельный поток в DLL владеет своим окном поверх игры (click-through) со своими часами и своим фреймрейтом, без единого игрового хука, поэтому пауза, зависший игровой цикл или сбой там не могут её заморозить или утащить за собой игру. Само меню пока рисуется в кадре игры; перенос его отрисовки на эту поверхность — следующий этап.

### Fixed
- The version badge in the menu header was visibly off-centre: the pill and the text were centred from different origins, so the label sat 5 px right and 6 px high. Both now derive from one centre point and line up with the rows and the tab strip.
  Бейдж версии в шапке меню стоял заметно не по центру: плашка и текст центрировались от разных начал, поэтому подпись уезжала на 5 px вправо и на 6 px вверх. Теперь оба выводятся из одного центра и встают в одну линию со строками и полосой вкладок.
- Repository integrity: a bare `*.dll` rule in `.gitignore` also matched the `ZenWare.DLL` directory, so every new source file under it - the font loader, the style layer, the layout grid, the font resources and the overlay module - existed only on disk and a fresh clone would not have built. The rule is now path-anchored and all of those sources are tracked.
  Целостность репозитория: правило `*.dll` в `.gitignore` совпадало и с папкой `ZenWare.DLL`, поэтому все новые исходники внутри неё — загрузчик шрифтов, слой стиля, сетка разметки, ресурсы шрифтов и модуль оверлея — существовали только на диске, и свежий клон не собрался бы. Правило теперь привязано к путям, а все эти исходники отслеживаются.

### Known issues
- A third-party binary (`l4dx86_[unknowncheats.me]_.dll`) surfaced into the tree when the ignore rule was corrected. It belongs to nobody here and the repository ships sources only; removing it from the index needs an approval that timed out twice, so it is still tracked in the previous commit. Tracked run files are unaffected.
  Сторонний бинарник (`l4dx86_[unknowncheats.me]_.dll`) всплыл в дереве, когда правило игнорирования исправили. Он здесь ничей, а репозиторий поставляется только исходниками; для удаления из индекса нужно подтверждение, которое дважды ушло в таймаут, поэтому в предыдущем коммите он пока отслеживается. На работу собранных файлов это не влияет.
- The real blur behind the panel is still an approximation: `ISurface` exposes no UV textured rect, so a true blur needs the material route (`dev/blurfilterx|y` over a copied render target).
  Настоящее размытие за панелью всё ещё приближение: у `ISurface` нет текстурного прямоугольника с UV, поэтому для реального размытия нужен material-путь (`dev/blurfilterx|y` по скопированному render target).

## [3.21.5] - 2026-09-16

### Added
- Stage 1 of moving the GUI off the game renderer: a real overlay surface that belongs to the DLL alone. A dedicated thread creates its own always-on-top, click-through, non-activating window and repaints it from its own loop with its own clock. There is no game hook anywhere in this path, so a pause, a stalled game loop or a low frame rate cannot freeze this surface, and a fault inside it cannot take the game down with it.
  Этап 1 переезда интерфейса с игрового рендера: настоящая оверлейная поверхность, принадлежащая только DLL. Отдельный поток создаёт своё окно поверх игры (click-through, без активации) и перерисовывает его из своего цикла со своими часами. В этом пути нет ни одного игрового хука, поэтому пауза, зависший игровой цикл или низкий фреймрейт не могут заморозить эту поверхность, а сбой внутри неё не может утащить за собой игру.
- The surface currently draws the product watermark and its own measured frame rate, which is the evidence that it is really independent: `ZenWare.cc v3.21.5 | overlay N fps` in the top-left corner, plus a line in `ZenWare.log` every five seconds, `Overlay: own clock N fps (game render loop not involved)`.
  Сейчас поверхность рисует ватермарк продукта и свой измеренный фреймрейт — это и есть доказательство независимости: `ZenWare.cc v3.21.5 | overlay N fps` в левом верхнем углу и строка в `ZenWare.log` каждые пять секунд, `Overlay: own clock N fps (game render loop not involved)`.
- The surface can be switched off with `Vars::Menu::bOverlayWatermark` (on by default).
  Поверхность выключается флагом `Vars::Menu::bOverlayWatermark` (по умолчанию включена).

### Note
- What is deliberately not done yet: the menu itself is still painted inside the game's render frame (`EngineVGui::Paint`), which is why the frame-rate readout still mirrors the game and why a single-player pause still stalls everything driven from that frame. This release is the foundation - window, thread, clock and the proof of independence. Porting the menu drawing onto this surface and routing its input are the next stages.
  Что намеренно ещё не сделано: само меню по-прежнему рисуется внутри кадра игры (`EngineVGui::Paint`), поэтому счётчик фреймов всё ещё повторяет игру и пауза в одиночке всё ещё останавливает всё, что привязано к этому кадру. Этот релиз — фундамент: окно, поток, часы и доказательство независимости. Перенос отрисовки меню на эту поверхность и её ввод — следующие этапы.

## [3.21.4] - 2026-09-16

### Fixed
- The version badge in the menu header was visibly off-centre. The text centre and the pill centre were computed from different origins: the pill started at `nW - 14 - badgeW` while the text was centred on `nW - 9 - badgeW/2`, so the label sat 5 px to the right, and it was also 6 px above the pill centre because the pill started at `+12` while the text was centred on `+15`. Both now derive from one place: the pill is centred in the header band (`(Layout::kHeaderH - 18) / 2`) and the text is centred on the pill centre, with the right margin taken from `Layout::kPadding` so the badge lines up with the rows and the tab strip.
  Бейдж версии в шапке меню стоял заметно не по центру. Центр текста и центр плашки считались от разных начал: плашка начиналась с `nW - 14 - badgeW`, а текст центрировался по `nW - 9 - badgeW/2`, поэтому подпись уезжала на 5 px вправо, а по вертикали ещё и на 6 px вверх, потому что плашка стартовала с `+12`, а текст центрировался по `+15`. Теперь и то и другое выводится из одного места: плашка центрируется в полосе шапки (`(Layout::kHeaderH - 18) / 2`), текст центрируется по центру плашки, а правый отступ берётся из `Layout::kPadding`, поэтому бейдж встаёт в одну линию со строками и полосой вкладок.

## [3.21.3] - 2026-09-16

### Fixed
- The loader title rainbow was still jumping, and the culprit was not the GDI wordmark fixed in 3.21.0 but the Direct2D renderer that actually paints the window. `Renderer2D.cpp` computed the hue as `fmodf(float(GetTickCount64() % 100000) / 38.0f, 360.0f)`: the remainder by 100000 ms resets the counter every 100 seconds, so the phase was thrown from about 110 degrees straight back to 0 and the colour snapped on screen. Both uses (the per-letter title colours and the gradient fallback brush shift) now call a single `TitleHue()` helper that accumulates the frame delta in a small float and wraps it at 360.
  Радуга заголовка в лоадере всё ещё «срывалась», и виноват был не GDI-логотип, исправленный в 3.21.0, а рендерер Direct2D, который фактически рисует окно. В `Renderer2D.cpp` оттенок считался как `fmodf(float(GetTickCount64() % 100000) / 38.0f, 360.0f)`: остаток по 100000 мс обнуляет счётчик каждые 100 секунд, поэтому фаза выбрасывалась примерно с 110 градусов обратно в 0 и цвет резко менялся на экране. Оба места (цвета букв заголовка и сдвиг градиентной кисти-фолбэка) теперь вызывают один helper `TitleHue()`, который копит дельту кадра в небольшом float и заворачивает её на 360.
- Deterministic check of the old formula, computed without launching the game: tick 99961 gives hue 110.55, tick 100000 gives hue 0. That 110-degree snap every 100 seconds is exactly the visible symptom.
  Детерминированная проверка старой формулы, посчитанная без запуска игры: tick 99961 даёт оттенок 110.55, tick 100000 — 0. Этот срыв на 110 градусов каждые 100 секунд и есть видимый симптом.

### Note
- The single-file build was refreshed: `dist\ZenWare.exe` now reports FileVersion 3.21.3, so the fixed D2D renderer is inside the artifact you actually run.
  Одиночная сборка обновлена: `dist\ZenWare.exe` теперь сообщает FileVersion 3.21.3, то есть исправленный D2D-рендерер лежит внутри того файла, который ты запускаешь.

## [3.21.2] - 2026-09-15

### Fixed
- The loader had four more places with the same growing-tick hue: the party accents `Acc()` and `Acc2()` plus two animated elements. All of them now use the shared wrapped phase helper, so no rainbow in the product can step with uptime any more.
  В лоадере было ещё четыре места с тем же растущим тиком: party-акценты `Acc()` и `Acc2()` плюс две анимированные элементы. Все они переведены на общий helper с завёрнутой фазой, поэтому ни одна радуга в продукте больше не может «шагать» вместе с аптаймом.

## [3.21.1] - 2026-09-15

### Fixed
- The same colour-phase defect was present in three more menu elements: the rainbow line under the header, the footer gradient and the border pulse. All three now call the shared `HuePhase(slot, msPerDegree)` helper, which accumulates the tick delta in a small float and wraps it every frame.
  Тот же дефект фазы цвета был ещё в трёх элементах меню: радужная линия под шапкой, градиент футера и пульс рамки. Все три теперь вызывают общий helper `HuePhase(слот, мс/градус)`, который копит дельту тика в небольшом float и заворачивает её каждый кадр.

## [3.21.0] - 2026-09-15

### Fixed
- The RGB "ZenWare.cc" wordmark no longer throws its colour. The hue used to be recomputed as `float(GetTickCount64()) / 38`, i.e. a value that starts near zero and grows for the whole uptime of the machine while sitting inside a `float`. A float keeps roughly seven significant digits, so the step between representable values grows with the magnitude: after long uptime the hue advances in visible jumps instead of a smooth sweep, and the cycle never looks endless. The phase is now accumulated frame by frame in a small float that is wrapped every frame, so it stays in `0..360` with full precision forever.
  RGB-надпись «ZenWare.cc» больше не «бросает» цвет. Раньше оттенок пересчитывался как `float(GetTickCount64()) / 38`, то есть величина, которая начинается около нуля и растёт всё время аптайма машины, находясь внутри `float`. Float хранит примерно семь значащих цифр, поэтому шаг между представимыми значениями растёт вместе с величиной: при большом аптайме оттенок идёт видимыми ступеньками вместо плавного хода, и цикл перестаёт выглядеть вечным. Теперь фаза накапливается покадрово в небольшом float и заворачивается каждый кадр, поэтому всегда остаётся в диапазоне `0..360` с полной точностью.
- The same defect existed in the loader window wordmark, which is drawn with GDI; both implementations now share the wrapped-phase approach.
  Тот же дефект был в надписи окна лоадера, которая рисуется через GDI; теперь обе реализации используют один подход с завёрнутой фазой.

### Note
- Everything else about the logo is unchanged on purpose: 38 ms per degree, the 5-degree drift per letter, the dark glow at value 0.35, and the eight offset copies around the text.
  Всё остальное в логотипе намеренно не менялось: 38 мс на градус, дрейф 5 градусов на букву, тёмное свечение со значением 0.35 и восемь смещённых копий вокруг текста.

## [3.20.1] - 2026-09-15

### Fixed
- The frosted backdrop from 3.20.0 shipped without its drawing code: the flag existed but nothing read it, because the helper that inserted the block failed on a PowerShell type error while the script still reported success. The backdrop is now actually wired in `DrawPanel` and the commit states the correction plainly.
  Подложка из 3.20.0 вышла без кода отрисовки: флаг был, но его никто не читал — вставка блока упала на ошибке типов PowerShell, а скрипт всё равно отчитался об успехе. Сейчас подложка действительно встроена в `DrawPanel`, и коммит прямо говорит об исправлении.

## [3.20.0] - 2026-09-15

### Added
- Stage 6 of the menu rework: a frosted backdrop behind the panel, gated by `Vars::Menu::bEnableBlur` (on by default, one constant to turn off). It draws three nested translucent layers that fade outward plus a vertical depth gradient, so the menu reads as a surface above the game instead of a flat rectangle on top of it.
  Этап 6 переработки меню: морозная подложка за панелью под флагом `Vars::Menu::bEnableBlur` (по умолчанию включена, выключение — одна константа). Она рисует три вложенных полупрозрачных слоя, затухающих наружу, плюс вертикальный градиент глубины, поэтому меню читается как поверхность над игрой, а не как плоский прямоугольник сверху.
- Stage 7 polish: the version label became a proper pill badge (accent tint, accent outline, `MENU_SMALL` face) instead of bare text, and the scrollbar moved onto the 8 px grid (`Layout::kPadding/2` from the right edge) instead of the old magic 7 px.
  Полировка этапа 7: подпись версии стала полноценным бейджем-пилюлей (акцентная заливка, акцентная обводка, начертание `MENU_SMALL`) вместо голого текста, а полоса прокрутки переехала на сетку 8 px (`Layout::kPadding/2` от правого края) вместо старого «магического» 7 px.

### Note
- The backdrop is an honest approximation, not a gaussian blur: `ISurface` exposes no UV textured rect, so a real blur still needs the material route (`dev/blurfilterx|y` over a render target copied through `IMatRenderContext`). That remains the open item; the flag and the layer structure are in place so the real implementation can slot in without touching the layout.
  Подложка — честное приближение, а не гауссово размытие: у `ISurface` нет текстурного прямоугольника с UV, поэтому настоящему размытию всё ещё нужен путь через material system (`dev/blurfilterx|y` по render target, скопированному через `IMatRenderContext`). Это остаётся открытым пунктом; флаг и структура слоёв уже на месте, чтобы реальная реализация встала без правок разметки.

## [3.19.0] - 2026-09-15

### Added
- Stage 5 of the menu rework: icons. The tab strip now carries a glyph per tab (eye / runner / crosshairs / bolt / sliders), taken from the embedded Font Awesome 6 Free face and drawn through `EFonts::MENU_ICONS` next to the label.
  Этап 5 переработки меню: иконки. Полоса вкладок теперь несёт глиф на каждую вкладку (глаз / бегун / прицел / молния / ползунки) из вшитого начертания Font Awesome 6 Free, рисуется через `EFonts::MENU_ICONS` рядом с подписью.

### Fixed
- The icon face could not actually render anything: `SetFontGlyphSet` was called with the default glyph range `0, 0` (basic Latin), so VGUI never built the Font Awesome private-use glyphs and any icon would have come out blank. `Init()` now re-registers the icon font with the range `0xF000-0xF8FF` and logs the result, so the failure mode is visible in `ZenWare.log` instead of showing as empty boxes.
  Шрифт иконок физически не мог ничего нарисовать: `SetFontGlyphSet` вызывался с диапазоном глифов по умолчанию `0, 0` (базовая латиница), поэтому VGUI вообще не строил глифы из приватной области Font Awesome, и любая иконка вышла бы пустой. Теперь `Init()` перерегистрирует шрифт иконок с диапазоном `0xF000-0xF8FF` и пишет результат в лог, то есть отказ видно в `ZenWare.log`, а не в виде пустых квадратов.
- Tab icons are matched by the internal tab id, not by the localized caption, so switching the menu language cannot scramble the icons.
  Иконки вкладок сопоставляются по внутреннему идентификатору вкладки, а не по локализованной подписи, поэтому смена языка меню не может перепутать иконки.

### Note
- The bundled icon header currently defines only six Font Awesome glyphs, so the tab set (five) consumes almost all of them. Rows, toggles and section headers keep text-only labels until the header is extended with more glyphs.
  Вшитый заголовок иконок сейчас определяет всего шесть глифов Font Awesome, поэтому набор вкладок (пять) забирает почти все. Строки, тумблеры и заголовки секций остаются без иконок, пока в заголовок не добавят остальные глифы.

## [3.18.0] - 2026-09-15

### Changed
- Stage 4 of the menu rework: the controls now read their geometry from the style tokens instead of local magic numbers. Checkbox and action-button rows take their padding from `Layout::kPadding` (16 px, the same gutter as the header and footer) and a uniform height of `Layout::G(3)+4` (28 px), so rows line up with the panel grid like the tabs do.
  Этап 4 переработки меню: контролы берут геометрию из токенов стиля, а не из локальных «магических» чисел. Строки чекбоксов и кнопок-действий получают отступ из `Layout::kPadding` (16 px, тот же гаттер, что у шапки и футера) и единую высоту `Layout::G(3)+4` (28 px), поэтому строки выравниваются по сетке панели так же, как вкладки.
- The toggle is now 32x18 (`Theme::Size::toggleW/toggleH`) instead of 30x14, i.e. a bigger and easier click target, and the knob travel is computed from the knob radius (`knobR+1` start, `toggleW-(knobR*2+2)` travel) so the knob stays inside the track at both ends instead of relying on a hardcoded 14.
  Тумблер теперь 32x18 (`Theme::Size::toggleW/toggleH`) вместо 30x14 — цель для клика крупнее, а ход кноба считается из радиуса кноба (старт `knobR+1`, ход `toggleW-(knobR*2+2)`), поэтому кноб остаётся внутри дорожки на обоих концах, а не держится на зашитом 14.
- Slider and label rows express their 20 px indent and 40 px width inset through `Layout::kPadding+4`, and the slider knob radius and track height come from `Theme::Size`, so the whole control set now shares one source of numbers.
  Строки слайдера и подписи выражают отступ 20 px и сужение ширины 40 px через `Layout::kPadding+4`, а радиус кноба и высота дорожки слайдера берутся из `Theme::Size`, поэтому весь набор контролов теперь делит один источник чисел.

## [3.17.0] - 2026-09-15

### Changed
- Stage 3 of the menu rework: panel composition. The panel moves from 320x480 to 400x560 on the 8 px grid (`Layout::kPanelW/kPanelH`), the header band grows to 48 px, the footer to 24 px, and the tab strip becomes taller pills (32 px) with the same sliding indicator, so the tab labels are no longer cramped.
  Этап 3 переработки меню: композиция панели. Панель переходит с 320x480 на 400x560 по сетке 8 px (`Layout::kPanelW/kPanelH`), полоса шапки растёт до 48 px, футер до 24 px, а полоса вкладок становится более высокими pill-кнопками (32 px) с тем же скользящим индикатором, поэтому подписи вкладок больше не сжаты.
- Content geometry now has a single definition: `nContentTop = header + tabs + 8` feeds both the row cursor and the scrollbar, replacing the duplicated `HEADER_H+34` magic and the hardcoded scrollbar bounds `+80 / -30`.
  Геометрия контента получила единое определение: `nContentTop = шапка + вкладки + 8` питает и курсор строк, и полосу прокрутки, вместо продублированного «магического» `HEADER_H+34` и жёстких границ скроллбара `+80 / -30`.
- Version metadata is real again: `ZENWARE_VER_STR` / `kVersion` were stuck at 3.14.0 while the tags moved on, so the panel badge and the watermark lied. They now read 3.17.0, and all eight READMEs were updated in the same commit.
  Метаданные версии снова правдивы: `ZENWARE_VER_STR` / `kVersion` застряли на 3.14.0, пока теги шли вперёд, поэтому бейдж панели и ватермарк врали. Теперь там 3.17.0, и все восемь README обновлены тем же коммитом.

## [3.16.0] - 2026-09-15

### Added
- Embedded fonts are in the build now: the five provided faces (Inter Regular / Medium / SemiBold, JetBrains Mono Regular, Font Awesome 6 Free Solid) are compiled into `ZenWare.DLL` as RCDATA resources through `res\fonts.rc` (ids 300-304), so `Fonts::LoadAll` registers the real faces at startup instead of falling back. The DLL grows by ~2.5 MB.
  Встроенные шрифты теперь в сборке: пять предоставленных начертаний (Inter Regular / Medium / SemiBold, JetBrains Mono Regular, Font Awesome 6 Free Solid) компилируются внутрь `ZenWare.DLL` как RCDATA-ресурсы через `res\fonts.rc` (идентификаторы 300-304), поэтому `Fonts::LoadAll` при запуске регистрирует настоящие начертания, а не уходит в фолбэк. Размер DLL растёт примерно на 2.5 МБ.
- Stage 2 of the menu rework: style tokens. New `ZenWare.DLL\src\Styles\Theme.h` holds the palette (`bg`, `surface`, `surfaceHover`, `border`, `borderAccent`, `textPrimary`, `textSecondary`, `textDim`, `accent`, `accentDim`, `accentSoft`, `danger`, `success`), metrics (radii 6/8/12/16, spacing 4/8/12/16/24, font sizes 11/13/14/18/22) and timings (hover 150 ms, press 80 ms, tab 200 ms, stagger 30 ms per row, spring k=180 c=20).
  Этап 2 переработки меню: токены стиля. Новый файл `ZenWare.DLL\src\Styles\Theme.h` содержит палитру (`bg`, `surface`, `surfaceHover`, `border`, `borderAccent`, `textPrimary`, `textSecondary`, `textDim`, `accent`, `accentDim`, `accentSoft`, `danger`, `success`), метрики (радиусы 6/8/12/16, отступы 4/8/12/16/24, размеры шрифтов 11/13/14/18/22) и тайминги (hover 150 мс, press 80 мс, вкладка 200 мс, stagger 30 мс на строку, пружина k=180 c=20).
- New `ZenWare.DLL\src\Styles\Layout.h`: the 8 px grid (`Layout::G(units)`), target panel geometry 400x560 with header 48 / tabs 40 / footer 24, plus `Scale` for the upcoming HiDPI factor and `ClampX/ClampY` so the panel cannot be dragged off screen.
  Новый `ZenWare.DLL\src\Styles\Layout.h`: сетка 8 px (`Layout::G(units)`), целевая геометрия панели 400x560 с шапкой 48 / табами 40 / футером 24, а также `Scale` под будущий множитель HiDPI и `ClampX/ClampY`, чтобы панель нельзя было утащить за пределы экрана.
- `Menu.cpp` palette aliases now come from `Theme` instead of local literals, so the accent has a single definition path (the live value still follows `Vars::Menu::clrAccent`).
  Псевдонимы палитры в `Menu.cpp` теперь берутся из `Theme`, а не из локальных литералов, поэтому у акцента один путь определения (живое значение по-прежнему следует за `Vars::Menu::clrAccent`).

### Note
- The panel size stays 320x480 in this stage on purpose: switching to 400x560 belongs to the composition stage, so nothing jumps visually in the meantime.
  Размер панели на этом этапе сознательно остаётся 320x480: переход на 400x560 относится к этапу композиции, поэтому визуально ничего не «прыгает».

## [3.15.0] - 2026-09-15

### Added
- Stage 1 of the in-game menu rework: custom fonts. New module `ZenWare.DLL/src/Util/Fonts/FontLoader.h|.cpp` registers embedded `.ttf` faces from RCDATA resources through `AddFontMemResourceEx` (with the `nFonts > 0` check) and releases them with `RemoveFontMemResourceEx` on unload. Resource ids live in `Fonts::EResource` (300-304), so nothing breaks while the font files are not in the repository yet - a missing resource is logged and the renderer falls back.
  Этап 1 переработки внутриигрового меню: кастомные шрифты. Новый модуль `ZenWare.DLL/src/Util/Fonts/FontLoader.h|.cpp` регистрирует встроенные `.ttf` из RCDATA-ресурсов через `AddFontMemResourceEx` (с проверкой `nFonts > 0`) и освобождает их через `RemoveFontMemResourceEx` при выгрузке. Идентификаторы ресурсов живут в `Fonts::EResource` (300-304), поэтому ничего не ломается, пока файлы шрифтов не добавлены в репозиторий: отсутствующий ресурс логируется, а рендер уходит на запасной шрифт.
- Menu faces replaced: `MENU_TAHOMA / MENU_CONSOLAS / MENU_VERDANA / MENU_ARIAL` are gone, replaced by `MENU_BODY` (Inter Medium 13), `MENU_SMALL` (Inter Regular 11), `MENU_HEADER` (Inter SemiBold 18), `MENU_MONO` (JetBrains Mono Regular 12), `MENU_ICONS` (Font Awesome 6 Free 12); `MENU_TAB` moved to Inter. All call sites across five feature files were updated.
  Шрифты меню заменены: `MENU_TAHOMA / MENU_CONSOLAS / MENU_VERDANA / MENU_ARIAL` удалены, вместо них `MENU_BODY` (Inter Medium 13), `MENU_SMALL` (Inter Regular 11), `MENU_HEADER` (Inter SemiBold 18), `MENU_MONO` (JetBrains Mono Regular 12), `MENU_ICONS` (Font Awesome 6 Free 12); `MENU_TAB` переведён на Inter. Обновлены все места вызова в пяти файлах фич.
- `FONTFLAG_OUTLINE` is replaced by `FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW` for the menu faces, with a fallback chain Inter -> Segoe UI Variable -> Segoe UI -> Tahoma and one log line per font at init, so it is immediately visible which face was used.
  `FONTFLAG_OUTLINE` заменён на `FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW` для шрифтов меню, добавлена цепочка фолбэков Inter -> Segoe UI Variable -> Segoe UI -> Tahoma и по одной строке лога на шрифт при инициализации, чтобы сразу видеть, какой шрифт использован.
- Texture plumbing in `G::Draw` for the future blur pass: `SetTexture`, `DrawTexturedRect`, `ResetTexture`, `GetTextureSize` over `ISurface`. Note: `ISurface` has no UV variant, so the rect is drawn 1:1 - UV support would need the material system (stage 6).
  Подготовка текстур в `G::Draw` для будущего blur: `SetTexture`, `DrawTexturedRect`, `ResetTexture`, `GetTextureSize` поверх `ISurface`. Замечание: у `ISurface` нет варианта с UV, поэтому прямоугольник рисуется 1:1 - для UV понадобится material system (этап 6).

### Note
- The `.ttf` files are not in the repository yet; the mechanism is complete and safe without them (fallback is active). To enable the new faces, place the files under `ZenWare.DLL\res\fonts\` and add them to `resource.rc` with ids 300-304.
  Файлы `.ttf` в репозитории пока нет; механизм полон и безопасен без них (работает фолбэк). Чтобы включить новые начертания, положи файлы в `ZenWare.DLL\res\fonts\` и добавь их в `resource.rc` с идентификаторами 300-304.

## [3.14.0] - 2026-09-15

### Added
- Release helper `tools/release.ps1` together with `tools/release-notes-template.md`: one command bumps the version in `resource.h`, `Vars.h` and all READMEs, inserts the CHANGELOG section, builds the solution and the single-file loader, commits, tags, pushes and creates the GitHub release in the repository's reference format. `-DryRun` prints the plan without changing anything. Before this, a release took five or six manual steps.
  Помощник релиза `tools/release.ps1` вместе с шаблоном `tools/release-notes-template.md`: одной командой поднимает версию в `resource.h`, `Vars.h` и всех README, вставляет секцию в CHANGELOG, собирает решение и однофайловый лоадер, делает коммит, тег, пуш и создаёт релиз на GitHub в эталонном формате. Режим `-DryRun` печатает план, ничего не меняя. До этого релиз занимал пять-шесть ручных шагов.

## [Unreleased]
## [3.13.7] - 2026-09-15

### Fixed
- Removed the pale translucent band at the top of the loader window: the header surface fill (35% alpha) read as a dull stripe over the glass, so it is gone and the header separator is now a faint hairline.
  Убрана светлая полупрозрачная полоса вверху окна лоадера: заливка шапки с альфой 35% выглядела как мутная полоса поверх стекла, поэтому она удалена, а разделитель шапки стал едва заметной линией.
- The rainbow title now has a real fallback: when the per-letter text layout is unavailable, the title is drawn with an animated rainbow gradient brush instead of plain white text, so the caption is never colourless.
  У радужного заголовка появился настоящий фолбэк: если по-буквенная разметка недоступна, заголовок рисуется анимированным радужным градиентом, а не обычным белым текстом, поэтому подпись больше не бывает бесцветной.


## [3.13.6] - 2026-09-15

### Added
- Crash breadcrumbs for the first uninstrumented calls in the Paint path:
  `Visuals::UpdateThirdPerson`, `Visuals::UpdateFullbright`,
  `Visuals::UpdateHideHands` and `Killfeed::OnTick`. A real crash log showed
  `EXCEPTION 0xC0000005 at ZenWare.dll+0x0001C5ED` with the last breadcrumb
  `Paint` - the fault is inside the module but before the first crumb-carrying
  call, so these four calls are the narrowed candidate set. The next crash will
  name the exact function.

### Note
- The faulting offset is not in the previously known list of crash offsets.


## [3.13.5] - 2026-09-15

### Changed / Изменено
- Direct2D interface: the button captions are no longer hardcoded English. The
  renderer now receives them from the very controls that already carry the
  eight-language strings (`LoaderUtil::SW`), so `LAUNCH GAME` / `INJECT` /
  `LAUNCH EXTERNAL` are localised in the D2D path exactly as they are in the GDI
  path, and there is a single source of truth for the wording.
  Интерфейс Direct2D: подписи кнопок больше не жёстко английские. Рендерер
  получает их из тех же контролов, в которых уже лежат строки на восьми языках
  (`LoaderUtil::SW`), поэтому `LAUNCH GAME` / `INJECT` / `LAUNCH EXTERNAL`
  локализованы в D2D-пути ровно так же, как в GDI-пути, и формулировка имеет
  один источник истины.


## [3.13.2] - 2026-09-15

### Changed / Изменено
- Direct2D title: `ZenWare.cc` is now drawn letter by letter with a rainbow hue
  that flows smoothly (the hue advances every frame and each letter is offset by
  5 degrees), the same effect the GDI path already had in `DrawRgbLogo`. The text
  layout is created once and reused, so there is no per-frame allocation.
  Заголовок в Direct2D: `ZenWare.cc` теперь рисуется по буквам с радужным
  оттенком, который плавно течёт (оттенок прирастает каждый кадр, каждая буква
  смещена на 5 градусов) — тот же эффект, что уже был в GDI-пути в `DrawRgbLogo`.
  Разметка текста создаётся один раз и переиспользуется, поэтому аллокаций в
  кадре нет.
- Small labels (the status line and the footer version/language) are anchored to
  the top of their rectangle instead of the vertical centre: with another language
  the font metrics changed and the text visually drifted upward. Pills and buttons
  keep their centred alignment.
  Мелкие подписи (строка статуса и версия/язык в футере) выравниваются по
  верхнему краю своего прямоугольника, а не по вертикальному центру: на другом
  языке менялись метрики шрифта, и текст визуально уезжал вверх. Пилюли и кнопки
  остаются выровненными по центру.


## [3.13.3] - 2026-09-14

### Changed / Изменено
- All Russian code comments across the DLL, the loader and the external overlay
  are translated to English (~420 comments in 70 files). User-facing strings are
  untouched: the menu still ships its 8 languages, loader UI and hotkey labels
  keep their translations. No functional changes.
  Все русские комментарии в коде DLL, лоадера и внешнего оверлея переведены на
  английский (~420 комментариев в 70 файлах). Пользовательские строки не тронуты:
  меню по-прежнему на 8 языках, UI лоадера и подписи хоткеев не изменились.
  Функциональных изменений нет.

### Fixed / Исправлено
- The in-menu version badge/watermark (`Vars::Menu::kVersion`) follows
  `resource.h` again: it stayed on v3.13.1 through the 3.13.2 release.
  Версия в бейдже/водяном знаке меню (`Vars::Menu::kVersion`) снова следует за
  `resource.h`: она осталась v3.13.1 после релиза 3.13.2.

## [3.13.4] - 2026-09-15

### Fixed / Исправлено
- The loader window after the splash was empty frosted glass: only the native
  title bar was visible. The Direct2D renderer was left disabled by the
  "behind the flag" switch from v3.11.2 (`SetEnabled(false)` in `wWinMain`),
  while v3.13 hid the GDI child controls and created the window with
  `WS_EX_NOREDIRECTIONBITMAP` for DirectComposition — the combination shows
  nothing at all. The renderer is enabled again when `Init` succeeds, and if
  composition failed to come up the `WS_EX_NOREDIRECTIONBITMAP` style is
  stripped so the legacy/HwndRenderTarget path (or GDI controls) stays visible.
  Окно лоадера после сплэша было пустым матовым стеклом: виден только нативный
  тайтлбар. Рендерер Direct2D остался выключенным переключателем «behind the
  flag» из v3.11.2 (`SetEnabled(false)` в `wWinMain`), а v3.13 тем временем
  скрыла GDI-контролы и создала окно с `WS_EX_NOREDIRECTIONBITMAP` под
  DirectComposition — вместе это даёт полностью пустое окно. Рендерер снова
  включается после успешного `Init`, а если композиция не поднялась, стиль
  `WS_EX_NOREDIRECTIONBITMAP` снимается, чтобы legacy/HwndRenderTarget-путь
  (или GDI-контролы) оставались видимыми.
- The D2D progress bar never animated during injection: the frame state built
  in `WM_TIMER` lost `busy` (and `dark`/`cursor`). Frame state assembly moved
  into one `FillFrameState()` used by both the timer and `WM_PAINT`.
  Прогресс-бар D2D не анимировался во время инжекта: состояние кадра в
  `WM_TIMER` теряло `busy` (и `dark`/`cursor`). Сборка состояния вынесена в
  одну `FillFrameState()`, которую используют и таймер, и `WM_PAINT`.
- Timer-driven rendering duplicated `WM_PAINT` rendering: per-tick
  invalidations of the GDI-only regions (header, rainbow button, progress
  area) are now skipped while the D2D interface is enabled.
  Рендер по таймеру дублировался с рендером из `WM_PAINT`: ежетиковые
  инвалидации чисто-GDI областей (шапка, радужная кнопка, прогресс) теперь
  пропускаются, пока интерфейс рисует D2D.
- On a device lost (TDR, GPU switch) the composition path froze forever:
  `EndDraw` failure now rebuilds the composition chain (target bitmap, brush,
  logo) once, and if composition cannot come back the renderer falls back to
  `HwndRenderTarget` and strips `WS_EX_NOREDIRECTIONBITMAP` from the window.
  При потере устройства (TDR, переключение GPU) композиция замирала навсегда:
  теперь ошибка `EndDraw` пересобирает композиционную цепочку (целевой битмап,
  кисть, логотип), а если композиция не восстанавливается — рендерер уходит на
  `HwndRenderTarget` и снимает с окна `WS_EX_NOREDIRECTIONBITMAP`.
- Without any glass backdrop (old OS) the composition frame cleared at alpha
  0.45 over nothing, so all content rendered dim. The background alpha now
  falls back to 0.92 when `Glass::IsAvailable()` is false.
  Без стеклянного бэкдропа (старая ОС) кадр очищался альфой 0.45 «в никуда»,
  и весь контент рисовался тусклым: альфа фона теперь откатывается к 0.92,
  если `Glass::IsAvailable()` ложно.
- Text in the composition path now uses grayscale antialiasing: ClearType on
  a premultiplied-alpha swap chain produces color fringes.
  Текст в композиции теперь с grayscale-сглаживанием: ClearType на
  premultiplied-альфе swapchain даёт цветные ореолы.

## [3.13.1] - 2026-09-14

### Fixed / Исправлено
- DPI mismatch in the composition renderer: the swap chain lives in physical
  pixels while the D2D target bitmap was created at a fixed 96 DPI, so on 125-150%
  displays the content was scaled and blurred. The window DPI is now read with
  `GetDpiForWindow` (resolved through `GetProcAddress`, so older systems still
  load), the target bitmap is created with that DPI, and the device context gets
  `SetDpi` right after `SetTarget`. The swap chain keeps physical pixels
  (`m_wPx`/`m_hPx`) while all layout runs in logical pixels (`m_w`/`m_h`).
  Несовпадение DPI в рендере композиции: swapchain живёт в физических пикселях,
  а целевой битмап D2D создавался с фиксированными 96 DPI, поэтому на 125-150%
  контент масштабировался и мылился. Теперь DPI окна читается через
  `GetDpiForWindow` (через `GetProcAddress`, чтобы старые системы продолжали
  загружаться), битмап создаётся с этим DPI, а контексту сразу после `SetTarget`
  выставляется `SetDpi`. Swapchain по-прежнему в физических пикселях
  (`m_wPx`/`m_hPx`), вся вёрстка — в логических (`m_w`/`m_h`).

### Note / Примечание
- There is no test red square in the repository code: if one is visible on
  screen, a different build is running.
  В коде репозитория нет тестового красного квадрата: если он виден на экране,
  запущена другая сборка.


## [3.13] - 2026-09-14

### Added / Добавлено
- The loader now renders through DirectComposition with per-pixel alpha: D3D11
  device with `BGRA_SUPPORT`, `CreateSwapChainForComposition` (2 buffers,
  `B8G8R8A8_UNORM`, `PREMULTIPLIED`, flip-sequential), `D2D1CreateDevice` ->
  `ID2D1DeviceContext`, a target bitmap created from the swap-chain back buffer
  with `D2D1_ALPHA_MODE_PREMULTIPLIED` and `SetTarget` before the first
  `BeginDraw`, then `SetContent` / `SetRoot` / `Commit` on the DComp visual, with
  `Present(1, 0)` after every `EndDraw`.
  Лоадер теперь рисует через DirectComposition со сквозной альфой: D3D11 с
  `BGRA_SUPPORT`, `CreateSwapChainForComposition` (2 буфера, `B8G8R8A8_UNORM`,
  `PREMULTIPLIED`, flip-sequential), `D2D1CreateDevice` → `ID2D1DeviceContext`,
  целевой битмап из back buffer со `D2D1_ALPHA_MODE_PREMULTIPLIED` и `SetTarget`
  до первого `BeginDraw`, затем `SetContent` / `SetRoot` / `Commit` у визуала
  DComp и `Present(1, 0)` после каждого `EndDraw`.
- Window style: `Zen2D::ProbeComposition()` decides up front whether the window
  gets `WS_EX_NOREDIRECTIONBITMAP` (needed for a transparent swap chain) or keeps
  the previous style; rendering moved to the 16 ms timer because such a window
  receives no `WM_PAINT`.
  Стиль окна: `Zen2D::ProbeComposition()` заранее решает, получит ли окно
  `WS_EX_NOREDIRECTIONBITMAP` (нужен для прозрачного swapchain) или прежний
  стиль; отрисовка переехала в 16-мс таймер, потому что такое окно не получает
  `WM_PAINT`.
- Alpha values per the spec: frame clears at 0.45 so the backdrop shows through,
  header 0.35, buttons 0.55 (hover 0.70), pills 0.55, borders 0.40, the primary
  INJECT button is mint at 0.85 with dark text, separators 0.30, secondary text
  0.85.
  Значения альфы по спецификации: кадр очищается при 0.45, чтобы был виден
  бэкдроп, шапка 0.35, кнопки 0.55 (hover 0.70), пилюли 0.55, бордеры 0.40,
  основная кнопка INJECT — мятная 0.85 с тёмным текстом, разделители 0.30,
  вторичный текст 0.85.
- Fail-safe: every init step logs an `HRESULT` to `OutputDebugStringW`, and if
  the composition chain fails at any point the renderer falls back to the previous
  `ID2D1HwndRenderTarget` path (opaque frame) so the window can never stay black.
  Отказоустойчивость: каждый шаг инициализации пишет `HRESULT` в
  `OutputDebugStringW`, а если цепочка композиции падает на любом шаге, рендерер
  возвращается к прежнему `ID2D1HwndRenderTarget` (непрозрачный кадр), поэтому
  чёрным окно остаться не может.

### 新增 / Китайский
- 加载器改为通过 DirectComposition 渲染并支持逐像素透明度：带 `BGRA_SUPPORT` 的
  D3D11 设备、`CreateSwapChainForComposition`（2 个缓冲、`B8G8R8A8_UNORM`、
  `PREMULTIPLIED`）、`D2D1CreateDevice` 与 `ID2D1DeviceContext`、由后台缓冲创建
  的目标位图（`D2D1_ALPHA_MODE_PREMULTIPLIED`），并在首次 `BeginDraw` 之前
  `SetTarget`，随后 `SetContent`/`SetRoot`/`Commit`，每次 `EndDraw` 后 `Present(1,0)`。
- 窗口样式由 `Zen2D::ProbeComposition()` 预先决定是否使用
  `WS_EX_NOREDIRECTIONBITMAP`；由于这类窗口收不到 `WM_PAINT`，渲染改由 16 毫秒定时器驱动。
- 任何一步失败都会回退到旧的 `ID2D1HwndRenderTarget` 路径，窗口不会变成黑屏。


## [3.12.1] - 2026-09-14

### Fixed / Исправлено
- Removed DwmExtendFrameIntoClientArea from the backdrop path: it handed the whole
  client area to DWM, so with GDI painting the interface disappeared and only the
  blurred backdrop remained. The material stays visible through the frame alpha
  composite (AlphaBlend 224).
  Убран DwmExtendFrameIntoClientArea из бэкдропа: он отдавал всю клиентскую
  область DWM, из-за чего при GDI-отрисовке интерфейс пропадал и оставался только
  размытый фон. Материал виден через альфа-композит кадра (AlphaBlend 224).

### Note / Примечание
- DirectComposition rendering is still not implemented: there is no swapchain,
  Present or SetTarget in the loader (the renderer is ID2D1HwndRenderTarget).
  Отрисовка через DirectComposition пока не реализована: в лоадере нет ни
  swapchain, ни Present, ни SetTarget (рендер — ID2D1HwndRenderTarget).


## [3.12] - 2026-09-14

### Changed / Изменено
- The window backdrop module was rewritten to the spec: Windows 11 22H2+ now uses
  the system Acrylic material (`DWMWA_SYSTEMBACKDROP_TYPE` = `DWMSBT_TRANSIENTWINDOW`,
  with `DwmExtendFrameIntoClientArea{-1,-1,-1,-1}` so the material covers the whole
  client area), Windows 10 1803+ falls back to the undocumented acrylic accent
  policy with an ABGR tint (`0xCC0D0E0A`), older builds get the plain blur
  (`0x990D0E0A`), and if nothing applies the loader keeps the plain dark frame.
  Модуль бэкдропа окна переписан по спецификации: Windows 11 22H2+ использует
  системный материал Acrylic (`DWMWA_SYSTEMBACKDROP_TYPE` = `DWMSBT_TRANSIENTWINDOW`
  вместе с `DwmExtendFrameIntoClientArea{-1,-1,-1,-1}`, чтобы материал покрывал всю
  клиентскую область), Windows 10 1803+ переходит на недокументированный
  акриловый accent policy с тинтом в ABGR (`0xCC0D0E0A`), более старые сборки
  получают простой блюр (`0x990D0E0A`), а если не применилось ничего — лоадер
  остаётся с обычной тёмной рамкой.
- `Glass::IsAvailable` now returns true only when a backdrop was actually applied,
  and the OS build is read through `RtlGetVersion` (`GetVersionEx` lies under a
  manifest). The legacy `Glass::PaintGlass` (software overlay) is removed.
  `Glass::IsAvailable` теперь возвращает true, только если бэкдроп реально
  применён, а номер сборки ОС читается через `RtlGetVersion` (`GetVersionEx` врёт
  при манифесте). Прежний `Glass::PaintGlass` (программный оверлей) удалён.
- Libraries for the DirectComposition step are linked ahead of time
  (`d3d11`, `dxgi`, `dcomp`).
  Библиотеки под шаг DirectComposition подключены заранее (`d3d11`, `dxgi`, `dcomp`).

### Not done yet / Пока не сделано
- Per-pixel alpha rendering (D3D11 + `CreateSwapChainForComposition` with
  `PREMULTIPLIED`, window with `WS_EX_NOREDIRECTIONBITMAP`, D2D on an
  `ID2D1DeviceContext`) and the D2D interface completion (localised labels, pills,
  layout, contrast) described earlier.
  Отрисовка со сквозной альфой (D3D11 + `CreateSwapChainForComposition` с
  `PREMULTIPLIED`, окно с `WS_EX_NOREDIRECTIONBITMAP`, D2D на `ID2D1DeviceContext`)
  и ранее описанное завершение D2D-интерфейса (локализация, пилюли, раскладка,
  контраст).


## [3.11.2] - 2026-09-14

### Changed / Изменено
- The Direct2D interface is back behind the flag (off by default): the default look
  is the previous full interface again. On a real screenshot the D2D frame was
  functional but incomplete - hardcoded English labels instead of the 8-language
  strings, missing pills, wide empty gaps and a clipped element at the bottom.
  Интерфейс Direct2D снова за флагом (выключен по умолчанию): вид по умолчанию —
  прежний полный интерфейс. На реальном скриншоте D2D-кадр был рабочим, но
  незавершённым: жёстко английские подписи вместо 8 языков, отсутствующие пилюли,
  большие пустоты и обрезанный элемент снизу.

### Not done yet / Пока не сделано
- D2D completion: localized labels through LoaderUtil::SW, language/update pills,
  footer without the duplicate version, tightened spacing and a higher-contrast
  secondary text; plus the frosted backdrop (DirectComposition swapchain with
  per-pixel alpha).
  Завершение D2D: локализация подписей через LoaderUtil::SW, пилюли языка и
  обновлений, футер без дубля версии, плотнее раскладка и контрастнее вторичный
  текст; плюс frosted-фон (swapchain DirectComposition со сквозной альфой).


## [3.11.1] - 2026-09-14

### Fixed / Исправлено
- The loader is dark in every code path now: the Windows light theme no longer
  switches the palette (a light-theme run produced a pale background with
  unreadable text - visible on a real screenshot). `RefreshTheme` always builds
  the dark theme.
  Лоадер теперь тёмный в любом пути исполнения: светлая тема Windows больше не
  переключает палитру (на светлой теме окно получалось бледным, текст
  нечитаемым — это видно на реальном скриншоте). `RefreshTheme` всегда собирает
  тёмную тему.
- The Direct2D frame now calls `Clear` with the palette background before drawing,
  so the surface can never keep or show a stale light background.
  Кадр Direct2D теперь явно очищается фоном палитры перед отрисовкой, поэтому
  поверхность не может сохранить или показать прежний светлый фон.


## [3.11] - 2026-09-14

### Changed / Изменено
- The Direct2D loader interface is now the default: after `Init`, the renderer is
  enabled and the old child controls that duplicated it (the two BS_OWNERDRAW
  buttons and the status static) are hidden. Clicks on the D2D buttons are routed
  from the parent's `WM_LBUTTONDOWN` through `BM_CLICK` to the same `IDC_*`
  controls, so the injection logic is untouched.
  Интерфейс на Direct2D теперь основной: после `Init` рендерер включается, а
  старые дочерние элементы, которые его дублировали (две кнопки BS_OWNERDRAW и
  статик статуса), скрываются. Нажатия по D2D-кнопкам передаются из
  `WM_LBUTTONDOWN` родителя через `BM_CLICK` тем же `IDC_*`, поэтому логика
  инжекта не тронута.
- The mode pill, the language pill and the update pill stay clickable in the D2D
  path: their rectangles are published from the D2D frame (mode pill top-right,
  language bottom-right, version/updates bottom-left) and the existing mouse
  handlers keep working.
  Пилюли режима, языка и обновлений остаются кликабельными в D2D-режиме: их
  прямоугольники выставляются из D2D-кадра (режим — справа сверху, язык — справа
  снизу, версия/обновления — слева снизу), а прежние обработчики мыши работают.

### Known gap / Известный пробел
- The D2D frame paints an opaque background, so the acrylic/system backdrop of
  3.10 is not visible while the D2D path is active. Bringing the frosted backdrop
  to the D2D path needs a DirectComposition swapchain with per-pixel alpha
  (window with `WS_EX_NOREDIRECTIONBITMAP`); until then the loader shows the flat
  dark interface.
  D2D-кадр рисует непрозрачный фон, поэтому акрил/системный материал из 3.10 в
  D2D-режиме не виден. Чтобы вернуть frosted-фон в D2D, нужен swapchain
  DirectComposition со сквозной альфой (окно с `WS_EX_NOREDIRECTIONBITMAP`);
  до этого лоадер показывает плоский тёмный интерфейс.


## [3.10.3] - 2026-09-14

### Fixed / Исправлено
- Direct2D loader UI: the light theme is gone for good - it flooded the window
  with mint and made the text unreadable. The frame now always renders with
  `Dark()`; `Light()` no longer exists, so it cannot be called by accident.
  Убрана светлая тема D2D-интерфейса: она заливала окно мятным и делала текст
  нечитаемым. Кадр всегда рисуется `Dark()`; `Light()` больше не существует,
  поэтому вызвать её случайно нельзя.
- Rounded corners everywhere: buttons 8 px, mode pill 6 px, status dot and
  progress track are rounded as well
  (`FillRoundedRectangle` / `DrawRoundedRectangle`).
  Скругления везде: кнопки 8 px, пилюля режима 6 px, точка статуса и трек
  прогресса тоже скруглены.
- Accent calmed down from the acid `#7FFFD4` to `#6EE7B7`; `accentDim` is
  `#3DB88F`, the rest of the palette is unchanged.
  Акцент стал спокойным: `#7FFFD4` → `#6EE7B7`; `accentDim` — `#3DB88F`,
  остальная палитра без изменений.
- The footer language now comes from `LoaderUtil::LangCode()` instead of an
  empty string.
  Язык в футере берётся из `LoaderUtil::LangCode()` вместо пустой строки.
- The main window logo is the real PNG from the resource (WIC decoder into an
  `ID2D1Bitmap`, 40x40) with the title next to it; the text stays as a fallback
  when the bitmap is unavailable.
  Логотип в главном окне — настоящий PNG из ресурса (WIC-декодер в
  `ID2D1Bitmap`, 40×40), название рядом; текст остался фолбэком, если битмап
  недоступен.
- The mint sine bands in the background are gone: a solid near-black base plus a
  single very soft vertical gradient (surface 0.25 -> background 0), built once
  as an `ID2D1LinearGradientBrush` and refreshed on theme change.
  Мятные синусоидные полосы убраны: чистый near-black фон плюс один очень
  мягкий вертикальный градиент (surface 0.25 -> background 0), создаётся один
  раз как `ID2D1LinearGradientBrush` и обновляется при смене темы.


## [3.10.2] - 2026-09-14

### Added / Добавлено
- Windows 11 system backdrop for the loader (`DWMWA_SYSTEMBACKDROP_TYPE` =
  `DWMSBT_MAINWINDOW`), tried before the acrylic path; the OS build is read
  through `RtlGetVersion` because `GetVersionEx` lies under a manifest. The
  order is now: system material (Win11 22H2+) -> acrylic blur -> plain blur ->
  the previous layered window.
  Системный материал Windows 11 для загрузчика (`DWMWA_SYSTEMBACKDROP_TYPE` =
  `DWMSBT_MAINWINDOW`) — пробуется раньше акрила; сборка ОС читается через
  `RtlGetVersion`, потому что `GetVersionEx` врёт при манифесте. Порядок теперь:
  системный материал (Win11 22H2+) -> акрил -> обычный блюр -> прежнее
  layered-окно.


## [3.10.1] - 2026-09-14

### Added / Добавлено
- Direct2D foundation for the loader UI, compiled in but disabled by default: new
  files `Theme.h` (final palette + `FrameState_t`) and `Renderer2D.h|.cpp` with
  D2D1 / DirectWrite / WIC initialization, per-widget draw layers (background,
  header, logo, mode pill, buttons, progress, status, footer) and proper COM
  cleanup. The loader keeps the 3.10 look: the D2D path paints an opaque
  background, so it stays behind a flag (`Renderer2D::SetEnabled`) until the real
  DWM/DirectComposition backdrop lands.
  Основа на Direct2D для интерфейса загрузчика — собирается, но выключена по
  умолчанию: новые файлы `Theme.h` (финальная палитра + `FrameState_t`) и
  `Renderer2D.h|.cpp` с инициализацией D2D1 / DirectWrite / WIC, послойной
  отрисовкой (фон, шапка, логотип, пилюля режима, кнопки, прогресс, статус,
  футер) и корректным освобождением COM. Вид остаётся как в 3.10: D2D-путь
  рисует непрозрачный фон, поэтому он за флагом (`Renderer2D::SetEnabled`) до
  появления настоящего backdrop через DWM/DirectComposition.
- New palette and typography defined for the planned look: near-black base
  (#0A0E0D), mint accent #7FFFD4 (instead of the acid green), Segoe UI Variable
  Display with a Segoe UI fallback and 400/500/600 weights.
  Заданы палитра и типографика нового вида: near-black база (#0A0E0D), мятный
  акцент #7FFFD4 (вместо кислотного), Segoe UI Variable Display с фолбэком
  Segoe UI и весами 400/500/600.

### Not done yet / Пока не сделано
- Frosted backdrop for the D2D path (DWM mica / DirectComposition swapchain with
  per-pixel alpha), splash rewrite, moving the buttons off `BS_OWNERDRAW`, and
  removing PARTY MODE / rainbow logo / ESP-corner decorations from `Main.cpp`.
  Frosted-фон для D2D-пути (mica / DirectComposition со сквозной альфой),
  переписывание splash, перевод кнопок с `BS_OWNERDRAW` и удаление PARTY MODE /
  радужного логотипа / ESP-уголков из `Main.cpp`.


## [3.10] - 2026-09-14

### Added / Добавлено
- Loader liquid glass: the window now blurs the desktop behind it (acrylic via
  the undocumented `SetWindowCompositionAttribute`) with a mint tint
  (#73FFCC at 0.85), an animated sheen band, a light edge highlight and a soft
  glow that follows the cursor. Rounded corners keep using DWM on Windows 11
  with a window-region fallback on Windows 10.
  Жидкое стекло в загрузчике: окно размывает рабочий стол под собой (акрил
  через недокументированный `SetWindowCompositionAttribute`) с мятным тинтом
  (#73FFCC, 0.85), анимированной полосой отблеска, светлым бликом по кромке и
  мягким пятном, следующим за курсором. Скругление углов — DWM на Windows 11
  с фолбэком на регион окна для Windows 10.

### Changed / Изменено
- The loader fade-in now animates the glass tint alpha instead of a layered
  window alpha: a layered window cannot show a blurred backdrop. When the
  system call is unavailable the window falls back to the previous layered
  behaviour, so nothing is lost on older builds.
  Появление окна загрузчика теперь анимирует альфу тинта, а не альфу layered-
  окна: layered-окно не может показывать размытие фона. Если системный вызов
  недоступен, окно возвращается к прежнему layered-поведению — на старых
  сборках ничего не теряется.
- New module `ZenWare.Loader/Glass.h|.cpp` (WinAPI + GDI, no third-party
  libraries); the frame is composited over the blurred backdrop with a constant
  alpha so the glass stays readable.
  Новый модуль `ZenWare.Loader/Glass.h|.cpp` (WinAPI + GDI, без сторонних
  библиотек); кадр накладывается на размытый фон с постоянной альфой, чтобы
  стекло оставалось читаемым.


## [3.9.1] - 2026-09-14

### Fixed / Исправлено
- Menu localisation completed for all eight languages: every UI key now exists in
  EN/RU/DE/ES/PT/PL/FR/ZH (163 keys per language, verified after the build).
  Локализация меню доведена до всех восьми языков: каждый ключ интерфейса теперь
  есть в EN/RU/DE/ES/PT/PL/FR/ZH (по 163 ключа на язык, проверено после сборки).
- The rows added in v3.9 (watermark, accent presets, menu opacity) landed in the
  wrong table: Chinese showed Russian strings while every other language fell
  back to English. All eight tables now carry proper translations.
  Строки, добавленные в v3.9 (водяной знак, пресеты акцента, прозрачность
  меню), попали не в ту таблицу: по-китайски выводился русский текст, а
  остальные языки падали в английский. Теперь во всех восьми таблицах
  корректные переводы.
- Remaining untranslated entries translated: German (Visuals, Snaplines, Chams,
  Prestrafe, Aimbot, Hitmarker, Strafes, Sync), Spanish (Witch, Chams,
  Prestrafe, Aimbot, Killfeed, Hitmarker, Sync), Portuguese (Witch, Chams,
  Prestrafe, Aimbot, Killfeed, Hitmarker, Revive, Sync), Polish (Edge/Jump bug,
  Aimbot, Killfeed, Hitmarker), French (Witch, Chams, Edge/Jump bug, Aimbot,
  Killfeed, Hitmarker), Chinese (Tank).
  Добиты оставшиеся непереведённые пункты: немецкие (Visuals, Snaplines, Chams,
  Prestrafe, Aimbot, Hitmarker, Strafes, Sync), испанские (Witch, Chams,
  Prestrafe, Aimbot, Killfeed, Hitmarker, Sync), португальские (Witch, Chams,
  Prestrafe, Aimbot, Killfeed, Hitmarker, Revive, Sync), польские (Edge/Jump
  bug, Aimbot, Killfeed, Hitmarker), французские (Witch, Chams, Edge/Jump bug,
  Aimbot, Killfeed, Hitmarker), китайский (Tank).
- Known loanwords are intentionally kept where they are the correct term in that
  language (Radar, TANK, Bhop/Strafe/Legit/Rage/W-Only, INTERFACE).
  Устоявшиеся заимствования оставлены осознанно, так как именно так и пишется в
  этих языках (Radar, TANK, Bhop/Strafe/Legit/Rage/W-Only, INTERFACE).


## [3.9] - 2026-09-14

### Added / Добавлено
- Menu look pass: gradient panel with adjustable opacity, version badge in the
  header, full-width section dividers, gradient slider fill, and the current tab
  name in the footer hint bar.
  Видовой апдейт меню: градиентная панель с регулируемой прозрачностью, бейдж
  версии в шапке, разделители секций на всю ширину, градиентная заливка
  слайдера, имя текущей вкладки в подсказке футера.
- Watermark HUD (bottom-right, visible while the menu is closed) with the
  version and fps, plus separate toggles for the badge and the fps part.
  Водяной знак (справа снизу, когда меню закрыто): версия и fps, плюс
  отдельные переключатели для плашки и счётчика кадров.
- Accent presets: five built-in palettes (Mint / Sunset / Ice / Gold / Violet)
  cycled by one menu button; the custom swatch row still works.
  Пресеты акцента: пять встроенных палитр (Мята / Закат / Лёд / Золото /
  Фиолет) переключаются одной кнопкой; ручные свотчи сохранены.
- Menu opacity slider (120..255) for the panel background.
  Слайдер прозрачности панели меню (120..255).

### Changed / Изменено
- Version strings bumped to 3.9 (loader resource + 8 READMEs).
  Строки версии подняты до 3.9 (ресурс загрузчика + 8 README).



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
