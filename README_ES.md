# ZenWare.cc — Español

**Software interno + externo de entrenamiento para Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.8.3`

> **Solo código fuente · Solo educación y servidores locales.**
> Sin `.exe` / `.dll` precompilados en este repo ni en Releases — compila desde el código,
> lanza el juego con `-insecure`, juega tu propio mapa (`map c1m1_hotel`).
> Servidores con VAC = riesgo de baneo. Úsalo bajo tu responsabilidad.

🇬🇧 [English](README.md) · 🇷🇺 [Русский](README_RU.md) · 🇩🇪 [Deutsch](README_DE.md) · 🇵🇹 [Português](README_PT.md) · 🇵🇱 [Polski](README_PL.md) · 🇫🇷 [Français](README_FR.md) · 🇨🇳 [中文](README_ZH.md)

## Acerca de

ZenWare.cc es un sandbox de entrenamiento para Left 4 Dead 2 (motor Source,
2009): render, predicción, movimiento y red — en tu PC, en tu servidor, con
`-insecure`. Sin bypass de VAC, sin servidores oficiales, sin binarios
precompilados.

El código nació de [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
y creció a tres herramientas en una solución (`ZenWare.sln`, `Release | Win32`):

| Parte | Modo | Qué hace | Salida |
|---|---|---|---|
| `ZenWare.DLL` | Interno (inyectado) | Cheat completo: ESP de esquinas, chams, aimbot, movimiento, menú en juego | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI loader, solo build local | Inyección manual-map / LoadLibrary, temas, 8 idiomas | `dist/ZenWare.exe` local, nunca publicado |
| `ZenWare.External` | Externo (sin inyección) | Lectura RPM + overlay GDI + bhop/strafe con `SendInput` | `bin/Release/ZenWare.External.exe` |

¿Nuevo en esto? Empieza con el **External** — nunca escribe en la memoria
del juego, es la forma más segura de estudiar el pipeline.
Detalles: [SECURITY.md](SECURITY.md) · Terceros: [NOTICE.md](NOTICE.md) ·
Licencia: [LICENSE](LICENSE) · Estructura: [ARCHITECTURE.md](ARCHITECTURE.md).

### Cómo funciona

**DLL interna.** Al inyectar, `DllMain` crea un hilo (la lógica nunca corre
en el loader lock) y `Entry::Load` sigue un init ordenado fail-closed:
logger → crash recorders → espera de `serverbrowser.dll` → escaneo de
patrones (con caché RVA `ZenWare.offsets`) → interfaces (`VEngineCvar007`
primero en `vstdlib.dll`) → netvars → hooks `MinHook` → carga de config.
Un patrón o interfaz ausente = message box, no crash.

Cada frame corren dos cadenas:

- **Tick** (`ClientMode::CreateMove`) — predicción, movimiento (BunnyHop →
  AutoStrafe → JumpStats → AutoShove), combate (Aimbot → TriggerBot →
  AutoPistol → NoSpread). El original del motor se llama exactamente una
  vez por tick.
- **Frame** (`EngineVGui::Paint`, solo `PAINT_UIPANELS`) — cvars de
  thirdperson / fullbright / hide-hands, tick de killfeed e hitmarker, ESP,
  radar, alertas, menú, crosshair, preview de granadas, overlay y dibujado
  animado de killfeed / hitmarker / JumpStats.

Sin eventos del motor by design — killfeed e hitmarker leen transiciones
HP/alive. Todo el dibujo pasa por `G::Draw` sobre `MatSystemSurface`; las
entidades se leen solo por netvars con check de `ClassID` antes de llamar
virtuales.

**Loader.** GUI Win32 con splash, tema oscuro/claro del sistema, 8 idiomas
y logo RGB animado. DLL, External y logo van embebidos como recursos — un
solo archivo de salida. Inyección manual-map (relocs, imports, protección
de secciones, borrado de headers) con fallback a `LoadLibrary`. Sin red,
sin auto-update.

**External.** Solo lectura: snapshots `ReadProcessMemory` a 20 Hz, overlay
GDI click-through a 60 Hz, movimiento solo con `SendInput`. El resolver
(firma + ancla + validación) reencuentra direcciones en cada arranque.

### Funciones

<details open>
<summary><b>Visuals</b> — ESP · Chams · Overlays</summary>

- **ESP** — esquinas (color por equipo), barra HP con borde + texto HP
  opcional, nombres con sombra, distancia, armas con balas del cargador,
  items, comunes, todos los infectados especiales incl. Boomer (fallback
  por nombre de clase), límite de distancia ESP opcional, snaplines tenues
- **Chams** — 5 paletas, colores propios para enemigos / aliados / Tank,
  interruptor through-walls, filtro de fantasmas y dormant
- **Mundo** — NoFog, Fullbright, sliders FOV mundo + viewmodel separados,
  tercera persona con distancia, ocultar manos, crosshair propio, overlay
  FPS / posición / HP, contador de comunes vivos
- **Intel** — radar 2D con rotación yaw, lista de espectadores, alertas de
  Tank / Witch, killfeed leído con tarjetas animadas, hitmarker con sonido,
  números de daño, cruz de impacto y stats de sesión
- **Equipo** — aviso de pin (Smoker/Hunter), alerta de revive, barra HP del
  Tank, panel HP del equipo, lista de SI cercanos con su panel, alerta de
  escupitajo, timers de granadas lanzadas (pipe / fuego molotov)
- **Feedback** — flash rojo de daño, flecha hacia el último atacante,
  filtro de daño mínimo, HUD del arma con balas (cargador + reserva),
  aviso amarillo `RELOADING` y rojo de poca munición
- **Granadas** — preview de trayectoria (molotov / pipe / bilis /
  lanzagranadas) con simulación de rebotes y marcador de caída

</details>

<details>
<summary><b>Combat</b> — ayuda de apuntado</summary>

- **Aimbot** — silent, prioridad FOV / Distancia, punto cabeza / centro,
  suavizado, solo visibles, tecla, comunes + todos los SI + Tank
- **Por arma** — FOV / suavizado / hitbox propios por grupo (rifles, SMG,
  escopetas, francotiradores, pistolas)
- **Ayudantes** — TriggerBot (dispara en el rayo post-aim, sin lag de tick),
  AutoShove (libera compañeros pineados), AutoPistol, NoSpread (whitelist
  de 11 armas, activado por defecto)

</details>

<details>
<summary><b>Movement</b> — bhop y strafes</summary>

- **Kit bhop** — BunnyHop perfect / legit, EdgeJump, EdgeBug, JumpBug,
  ayudante LongJump, FastStop, Prestrafe, AutoDuck, delay de salto ajustable
- **Strafe** — AutoStrafe legit / rage / w-only / directional
- **JumpStats** — panel estilo KZ: distancia, prestrafe, velocidad máx.,
  caída, altura de salto, tiempo en aire, sync % en vivo, conteo de
  strafes, contadores JB / EB de sesión

</details>

<details>
<summary><b>Menú / Sistema</b></summary>

- Menú en juego (`INSERT`): 5 pestañas (Visuals / Move / View / Combat /
  Misc), scroll por pestaña, logo RGB, ayudas `?` (EN + RU), swatches de
  color, binds de teclas, sliders, tecla pánico que oculta el ESP al
  instante
- 8 idiomas con cambio en vivo (botón o `F7`):
  EN / RU / DE / ES / PT / PL / FR / ZH
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`),
  `bool / int / float / Color`, sliders float con proxy int y resync
- Logger — `%TEMP%/ZenWare.log` se mueve a `<gamedir>/ZenWare.log`
  (fallback por PID), crash records como `module+offset` con breadcrumbs
- Caché de offsets — `<gamedir>/ZenWare.offsets` (auto, bórralo para
  reescanear); `Tools/SigScan` verifica los 15 patrones offline contra tus
  archivos del juego

</details>

### Compilar y jugar — solo local

**1. Compilar** (Visual Studio 2022 con `Desarrollo para escritorio con C++`):

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Juego** — Steam → L4D2 → opciones de lanzamiento:

```text
-insecure -windowed -console
```

**3. Mapa** — consola del juego:

```text
map c1m1_hotel
```

**4. Inyectar** — ejecuta tu loader compilado **como administrador** → `INJECT`.

Solo external (sin inyección): ejecuta tu
`ZenWare.External/bin/Release/ZenWare.External.exe` local o `START-ZenWare.bat`.

> No subas el `exe` compilado a los Releases públicos. Política solo-fuentes: [SECURITY.md](SECURITY.md).

### Requisitos

| Qué | Versión |
|---|---|
| SO | Windows 10/11 x64 |
| Juego | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desarrollo para escritorio con C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Target | `Release` · `Win32` (x86) |

### Teclas

| Tecla | Acción |
|:---:|---|
| `INSERT` | Abrir / cerrar menú |
| `F11` | Descargar |
| `F7` | Idioma EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (mantener) | BunnyHop (defecto) |
| `MOUSE4` (mantener) | Aimbot (defecto) |

Reasignar: `Misc → Menu key / Aimbot key` — clic → `[press key]` → pulsa la tecla, `ESC` = off.
La tecla pánico oculta las cajas al instante (`Visuals → ESP`).

### Configs y logs

- Slots junto a la ruta DLL del juego: `ZenWare.cfg`, `ZenWare2.cfg`,
  `ZenWare3.cfg` — `key=value` plano, 117 claves. Los valores de sesión
  (contadores JB/EB) nunca se guardan.
- Log: `<gamedir>/left4dead2/ZenWare.log`. Un crash se ve como
  `[!!!] EXCEPTION code=... at module+0x...` + último breadcrumb — con ese
  par basta, sin dumps.
- Caché de firmas: `<gamedir>/left4dead2/ZenWare.offsets` (RVAs + huella
  del módulo); tras actualizar el juego bórralo o corre
  `Verify-Signatures.bat` y arregla `Offsets.cpp` si dice `NOT FOUND`.

### Estructura del proyecto

```text
ZenWare.cc/
├── ZenWare.sln                  solución (DLL + Loader + External)
├── ZenWare.DLL/                 módulo interno
│   ├── src/Entry/               orden de init, crash recorders, unload
│   ├── src/Hooks/               ClientMode / EngineVGui / ModelRender / WndProc …
│   ├── src/SDK/                 interfaces L4D2, entidades, DrawManager, GameUtil
│   ├── src/Features/            Vars (ajustes) + Aimbot/ESP/Menu/… + Config + Lang
│   └── src/Util/                Logger, Pattern, caché Offsets, NetVars, MinHook
├── ZenWare.Loader/              GUI loader — solo build local, sin auto-update
├── ZenWare.External/            overlay externo — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               verificador de firmas para tu client.dll
├── .github/workflows/           check CI de compilación — sin subir binarios
├── dist/                        solo salida local (git-ignored)
├── Build-SingleFile.ps1         builder local (sin Publish)
├── Verify-Signatures.bat        check de patrones en un clic
├── START-ZenWare.bat            arranque local del external
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
└── README_RU/DE/ES/PT/PL/FR/ZH.md   traducciones de este archivo
```

### Problemas

| Síntoma | Solución |
|---|---|
| `XorString` / `FATAL ERROR` al arrancar | Firma rota tras update del juego → `Verify-Signatures.bat`, actualiza `Offsets.cpp` |
| Crash en juego | Abre el log → busca `[!!!] EXCEPTION` → reporta `module+0x...` + último breadcrumb, sin dumps |
| Alerta de antivirus | Carpeta → exclusiones (heurística en `WriteProcessMemory` / `CreateRemoteThread`) |
| Cuadrados en vez de letras | Pulsa `F7` (fallback de fuente / idioma) |
| External: sin cajas | Mira las líneas superiores del overlay (diagnóstico del resolver) y firmas |
| Menú abierto pero los clics van al juego | Cierra primero el menú del juego (el cheat captura `ESC` solo con su menú abierto) |

## Legal

| Tema | Términos |
|---|---|
| Licencia | MIT — [LICENSE](LICENSE), `AS IS`, sin garantía |
| Terceros | [NOTICE.md](NOTICE.md): MinHook, FontAwesome, headers de Valve, base |
| Marcas | Left 4 Dead 2 / Steam de Valve. Sin afiliación ni respaldo |
| Distribución | Solo fuentes — [SECURITY.md](SECURITY.md), sin binarios |
| Uso | Solo `-insecure` local. Sin bypass de VAC. Los baneos son tuyos |

<div align="center">

**Créditos** ·
Base: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Motor de hooks: MinHook (Tsuda Kageyu, estilo BSD) ·
Iconos: [FontAwesome 6 Free](https://fontawesome.com)

</div>
