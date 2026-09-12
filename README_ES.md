# ZenWare.cc — Español

**Software interno + externo de entrenamiento para Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.6`

> **Solo código fuente · Solo educación y servidores locales.**
> Sin `.exe` / `.dll` precompilados en este repo ni en Releases — compila desde el código,
> lanza el juego con `-insecure`, juega tu propio mapa (`map c1m1_hotel`).
> Servidores con VAC = riesgo de baneo. Úsalo bajo tu responsabilidad.

## Acerca de

Basado en [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base). Tres partes, una solución (`ZenWare.sln`, `Release | Win32`):

| Parte | Salida |
|---|---|
| `ZenWare.DLL` — módulo interno | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — cargador GUI | `dist/ZenWare.exe` local, nunca publicado, píldora a releases |
| `ZenWare.External` — overlay de solo lectura | `bin/Release/ZenWare.External.exe` |

## Características

<details open>
<summary><b>Visual</b> — ESP · Chams · Overlays</summary>

- **ESP** — cajas, barra de vida, nombres, distancia, armas/objetos, comunes, todos los Infectados especiales incl. Boomer (fallback por nombre de clase para builds ajenas)
- **Chams** — 5 paletas, aliados / enemigos / todos los SI, a través de paredes
- **Mundo** — NoFog, FOV de mundo + viewmodel, tercera persona, punto de mira personalizado, overlay de FPS
- **Intel** — radar 2D, lista de espectadores, alertas Tank / Witch, killfeed, hitmarker (hitsound + números de daño)
- **Equipo** — aviso de agarre, alerta de reanimación, barra de vida del Tank, panel de vida del equipo, lista de SI cercanos
- **Feedback** — cruz de impacto, stats de sesión (hits/disparos/precisión), destello de daño, filtro de daño mínimo, temporizadores de granadas lanzadas
- **Lanzables** — vista previa de trayectoria (molotov / pipe / bilis / lanzagranadas) con marcador de caída

</details>

<details>
<summary><b>Combate</b> — ayuda de apuntado</summary>

- **Aimbot** — silent, prioridad FOV / distancia, cabeza / cuerpo, suavizado, solo visibles, comunes + todos los SI
- **Por arma** — FOV / suavizado / hitbox propios por grupo (fusiles, SMG, escopetas, francotiradores, pistolas)
- **Ayudas** — TriggerBot, AutoShove, AutoPistol, NoSpread (activables)

</details>

<details>
<summary><b>Movimiento</b> — bhop y strafe</summary>

- **Kit bhop** — BunnyHop perfecto / legit, EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- **Strafe** — AutoStrafe legit / rage / w-only / direccional

</details>

<details>
<summary><b>Menú / Sistema</b></summary>

- Menú `INSERT` · `F11` descargar · idiomas EN/RU/DE/ES (botón + `F7`) · logo RGB animado · iconos `?` de ayuda · pestañas · scroll de rueda
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`, `bool / int / float / Color`)
- Logger — `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (fallback por pid)
- Caché de offsets — `<gamedir>/ZenWare.offsets` (auto, bórralo para forzar reescaneo)

</details>

## Compilar y ejecutar — solo local

**1. Compilar**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Prepara el juego** — Steam → L4D2 → opciones de lanzamiento:

```text
-insecure -windowed -console
```

**3. Carga un mapa local** — consola del juego:

```text
map c1m1_hotel
```

**4. Inyecta** — ejecuta tu cargador compilado **como administrador** → `INJECT`.

Solo external (sin inyección): ejecuta tu
`ZenWare.External/bin/Release/ZenWare.External.exe` o `START-ZenWare.bat`.

> No subas el `exe` compilado a GitHub Releases públicos. Política solo-fuente: [SECURITY.md](SECURITY.md).

## Requisitos

| Item | Versión |
|---|---|
| SO | Windows 10/11 x64 |
| Juego | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Target | `Release` · `Win32` (x86) |

## Teclas

| Tecla | Acción |
|:---:|---|
| `INSERT` | Abrir / cerrar menú |
| `F11` | Descargar |
| `F7` | Idioma EN / RU / DE / ES |
| `Space` (mantener) | BunnyHop (defecto) |
| `MOUSE4` (mantener) | Aimbot (defecto) |

Reasignar: `Misc → Menu key / Aimbot key` — clic → `[press key]` → pulsa una tecla, `ESC` = off.

## Estructura del proyecto

```text
ZenWare.cc/
├── ZenWare.sln                  solución (DLL + Loader + External)
├── ZenWare.DLL/                 módulo interno (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/              cargador GUI — solo build local, sin auto-update
├── ZenWare.External/            overlay externo — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               verificador de firmas para tu client.dll local
├── .github/workflows/           check de compilación CI — sin subida de binarios
├── dist/                        solo salida local (git-ignored)
├── Build-SingleFile.ps1         builder local (sin Publish)
├── Verify-Signatures.bat        check de patrones en un clic vs tus archivos
├── START-ZenWare.bat            lanzador local del external
├── README.md · README_DE.md · README_ES.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

## Problemas

| Síntoma | Solución |
|---|---|
| MessageBox `XorString` al inicio | Firma rota tras update del juego → doble clic `Verify-Signatures.bat`, actualiza `Offsets.cpp` |
| Crash tras `Paint` | Abre el log → busca `[!!!] EXCEPTION` → reporta `módulo+0x...`, sin dumps |
| Alerta de antivirus | Carpeta → exclusiones (heurística en `WriteProcessMemory` / `CreateRemoteThread`) |
| Cuadrados en vez de letras | Pulsa `F7` (fallback de fuente / idioma) |
| External: sin cajas | Revisa las líneas superiores del overlay (diagnóstico del resolver) y las firmas |
