<div align="center">

# ZenWare.cc — Português

[English](README.md) · [Русский](README.md#russian) · [Deutsch](README_DE.md) · [Español](README_ES.md) · **Português**

</div>

> **Somente código-fonte · Somente estudo e servidor local.**
> Sem `.exe` / `.dll` prontos neste repositório ou Releases — compile do código-fonte,
> inicie o jogo com `-insecure`, jogue seu próprio mapa (`map c1m1_hotel`).
> Servidores com VAC = risco de ban. Use por sua conta e risco.

## English

### About

Based on [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base). Three parts, one solution (`ZenWare.sln`, `Release | Win32`):

| Part | Output |
|---|---|
| `ZenWare.DLL` — internal module | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — GUI loader | local `dist/ZenWare.exe`, never published, releases-page pill |
| `ZenWare.External` — read-only overlay | `bin/Release/ZenWare.External.exe` |

### Features

<details open>
<summary><b>Visuals</b> — ESP · Chams · Overlays</summary>

- **ESP** — caixas, barra de vida, nomes, distância, armas/itens, comuns, todos Special Infected incl. Boomer (fallback por nome de classe para builds estrangeiros)
- **Chams** — 5 paletas, aliados / inimigos / todos SI, através das paredes
- **World** — NoFog, FOV de mundo + viewmodel, terceira pessoa, mira customizada, overlay de FPS
- **Intel** — radar 2D, lista de espectadores, alertas Tank / Witch, killfeed, hitmarker (hitsound + números de dano)
- **Team play** — aviso de agarrado, alerta de revive, barra de HP do tank, painel de HP do time, lista de SI próximos
- **Feedback** — marca de acerto na mira, stats da sessão (acertos/tiros/precisão), flash de dano, filtro de dano mínimo, timers de granadas lançadas
- **Throwables** — prévia da trajetória (molotov / pipe / bile / lança-granadas) com marcador de pouso

</details>

<details>
<summary><b>Combat</b> — mira assistida</summary>

- **Aimbot** — silent, prioridade FOV / Distância, cabeça / corpo, suavização, somente visíveis, comuns + todos SI
- **Per-weapon** — FOV / suavização / hitbox próprios por grupo (fuzis, SMG, escopetas, snipers, pistolas)
- **Helpers** — TriggerBot, AutoShove, AutoPistol, NoSpread (alternáveis)

</details>

<details>
<summary><b>Movement</b> — bhop & strafe</summary>

- **Bhop kit** — BunnyHop perfeito / legit, EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- **Strafe** — AutoStrafe legit / rage / w-only / direcional

</details>

<details>
<summary><b>Menu / System</b></summary>

- Menu `INSERT` · `F11` descarregar · alternar idioma (botão + `F7`) · logo RGB animado · ícones `?` de ajuda · abas · rolagem do mouse
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`, `bool / int / float / Color`)
- Logger — `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (fallback por pid)
- Cache de offsets — `<gamedir>/ZenWare.offsets` (auto, apague para forçar rescan)

</details>

### Build and run — local only

**1. Build**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Prepare the game** — Steam → L4D2 → opções de inicialização:

```text
-insecure -windowed -console
```

**3. Load a local map** — console do jogo:

```text
map c1m1_hotel
```

**4. Inject** — execute seu loader compilado localmente **como administrador** → `INJECT`.

External-only (sem injeção): execute seu local
`ZenWare.External/bin/Release/ZenWare.External.exe` ou `START-ZenWare.bat`.

> Não suba o `exe` compilado para GitHub Releases públicos. Política source-only: [SECURITY.md](SECURITY.md).

### Requirements

| Item | Version |
|---|---|
| OS | Windows 10/11 x64 |
| Game | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Target | `Release` · `Win32` (x86) |

### Hotkeys

| Key | Action |
|:---:|---|
| `INSERT` | Abrir / fechar menu |
| `F11` | Descarregar |
| `F7` | Idioma EN / RU / DE / ES / PT |
| `Space` (segurar) | BunnyHop (padrão) |
| `MOUSE4` (segurar) | Aimbot (padrão) |

Rebind: `Misc → Menu key / Aimbot key` — clique → `[press key]` → pressione uma tecla, `ESC` = off.

### Project structure

```text
ZenWare.cc/
├── ZenWare.sln                  solution (DLL + Loader + External)
├── ZenWare.DLL/                 internal module (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/              GUI loader — local build only, no auto-update
├── ZenWare.External/            external overlay — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               signature verifier for your local client.dll
├── .github/workflows/           CI compile check — no binary upload
├── dist/                        local output only (git-ignored)
├── Build-SingleFile.ps1         local builder (no Publish)
├── Verify-Signatures.bat        one-click pattern check vs your game files
├── START-ZenWare.bat            local external launcher
├── README.md · README_DE.md · README_ES.md · README_PT.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

### Troubleshooting

| Symptom | Fix |
|---|---|
| `XorString` MessageBox on start | Assinatura quebrou após update do jogo → duplo-clique em `Verify-Signatures.bat`, atualize `Offsets.cpp` |
| Crash após `Paint` | Abra o log → ache `[!!!] EXCEPTION` → reporte `module+0x...`, sem dumps |
| Antivirus flag | Pasta → exclusões (heurística em `WriteProcessMemory` / `CreateRemoteThread`) |
| Quadrados em vez de letras | Pressione `F7` (fallback de fonte / idioma) |
| External: sem caixas | Veja as linhas do topo do overlay (diagnóstico do resolver) e assinaturas |
