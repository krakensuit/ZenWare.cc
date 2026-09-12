<div align="center">

# ZenWare.cc — Português

[English](README.md) · [Русский](README_RU.md) · [Deutsch](README_DE.md) · [Español](README_ES.md) · **Português**

</div>

> **Somente código-fonte · Somente estudo e servidor local.**
> Sem `.exe` / `.dll` prontos neste repositório ou Releases — compile do código-fonte,
> inicie o jogo com `-insecure`, jogue seu próprio mapa (`map c1m1_hotel`).
> Servidores com VAC = risco de ban. Use por sua conta e risco.

🇵🇱 [Polski](README_PL.md) · 🇫🇷 [Français](README_FR.md) · 🇨🇳 [中文](README_ZH.md)

## Português

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.8.3`

### Sobre o projeto

ZenWare.cc é um sandbox de treino para Left 4 Dead 2 (engine Source, 2009):
render, predição, movimento e rede — no seu PC, no seu servidor, com
`-insecure`. Sem bypass de VAC, sem servidores oficiais, sem binários
prontos.

O código nasceu do [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
e virou três ferramentas numa solution (`ZenWare.sln`, `Release | Win32`):

| Parte | Modo | O que faz | Saída |
|---|---|---|---|
| `ZenWare.DLL` | Interna (injetada) | Cheat completo: ESP de cantos, chams, aimbot, movimento, menu no jogo | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI loader, só build local | Injeção manual-map / LoadLibrary, temas, 8 idiomas | `dist/ZenWare.exe` local, nunca publicado |
| `ZenWare.External` | Externa (sem injeção) | Leitura RPM + overlay GDI + bhop/strafe com `SendInput` | `bin/Release/ZenWare.External.exe` |

Novo nisso? Comece pelo **External** — nunca escreve na memória do jogo, é
o jeito mais seguro de estudar o pipeline.
Detalhes: [SECURITY.md](SECURITY.md) · Terceiros: [NOTICE.md](NOTICE.md) ·
Licença: [LICENSE](LICENSE) · Estrutura: [ARCHITECTURE.md](ARCHITECTURE.md).

### Como funciona

**DLL interna.** Ao injetar, a `DllMain` cria uma thread (a lógica nunca roda
no loader lock) e o `Entry::Load` segue um init ordenado fail-closed:
logger → crash recorders → espera do `serverbrowser.dll` → scan de padrões
(com cache RVA `ZenWare.offsets`) → interfaces (`VEngineCvar007` primeiro
na `vstdlib.dll`) → netvars → hooks `MinHook` → carrega config. Padrão ou
interface ausente = message box, não crash.

Cada frame rodam duas cadeias:

- **Tick** (`ClientMode::CreateMove`) — predição, movimento (BunnyHop →
  AutoStrafe → JumpStats → AutoShove), combate (Aimbot → TriggerBot →
  AutoPistol → NoSpread). O original da engine é chamado exatamente uma vez
  por tick.
- **Frame** (`EngineVGui::Paint`, só `PAINT_UIPANELS`) — cvars de thirdperson
  / fullbright / hide-hands, tick do killfeed e hitmarker, ESP, radar,
  alertas, menu, mira, preview de granadas, overlay e desenho animado de
  killfeed / hitmarker / JumpStats.

Sem eventos da engine by design — killfeed e hitmarker leem transições
HP/alive. Todo desenho passa por `G::Draw` sobre `MatSystemSurface`; as
entidades são lidas só por netvars com check de `ClassID` antes de chamar
virtuais.

**Loader.** GUI Win32 com splash, tema escuro/claro do sistema, 8 idiomas e
logo RGB animado. DLL, External e logo vão embutidos como recursos — um
único arquivo de saída. Injeção manual-map (relocs, imports, proteção de
seções, limpeza de headers) com fallback para `LoadLibrary`. Sem rede, sem
auto-update.

**External.** Só leitura: snapshots `ReadProcessMemory` a 20 Hz, overlay GDI
click-through a 60 Hz, movimento só com `SendInput`. O resolver (assinatura
+ âncora + validação) reencontra endereços a cada execução.

### Recursos

<details open>
<summary><b>Visuals</b> — ESP · Chams · Overlays</summary>

- **ESP** — cantos (cor por time), barra HP com borda + texto HP opcional,
  nomes com sombra, distância, armas com balas do pente, itens, comuns,
  todos os especiais incl. Boomer (fallback por nome de classe), limite de
  distância ESP opcional, snaplines suaves
- **Chams** — 5 paletas, cores próprias para inimigos / aliados / Tank,
  chave through-walls, filtro de fantasmas e dormant
- **Mundo** — NoFog, Fullbright, sliders FOV mundo + viewmodel separados,
  terceira pessoa com distância, ocultar mãos, mira própria, overlay FPS /
  posição / HP, contador de comuns vivos
- **Intel** — radar 2D com rotação yaw, lista de espectadores, alertas de
  Tank / Witch, killfeed lido com cartões animados, hitmarker com som,
  números de dano, cruz de acerto e stats da sessão
- **Time** — aviso de pin (Smoker/Hunter), alerta de revive, barra HP do
  Tank, painel HP do time, lista de SI próximos com painel próprio, alerta
  de gosma, timers de granadas lançadas (pipe / fogo molotov)
- **Feedback** — flash vermelho de dano, seta para o último atacante,
  filtro de dano mínimo, HUD da arma com balas (pente + reserva), aviso
  amarelo `RELOADING` e vermelho de pouca munição
- **Granadas** — preview de trajetória (molotov / pipe / bile /
  lança-granadas) com simulação de quiques e marcador de queda

</details>

<details>
<summary><b>Combat</b> — ajuda de mira</summary>

- **Aimbot** — silent, prioridade FOV / Distância, ponto cabeça / centro,
  suavização, só visíveis, tecla, comuns + todos os SI + Tank
- **Por arma** — FOV / suavização / hitbox próprios por grupo (fuzis, SMGs,
  escopetas, snipers, pistolas)
- **Ajudantes** — TriggerBot (atira no raio pós-aim, sem lag de tick),
  AutoShove (solta colegas presos), AutoPistol, NoSpread (whitelist de 11
  armas, ligado por padrão)

</details>

<details>
<summary><b>Movement</b> — bhop e strafes</summary>

- **Kit bhop** — BunnyHop perfect / legit, EdgeJump, EdgeBug, JumpBug,
  ajudante LongJump, FastStop, Prestrafe, AutoDuck, delay de pulo ajustável
- **Strafe** — AutoStrafe legit / rage / w-only / directional
- **JumpStats** — painel estilo KZ: distância, prestrafe, velocidade máx.,
  queda, altura do pulo, tempo no ar, sync % ao vivo, contagem de strafes,
  contadores JB / EB da sessão

</details>

<details>
<summary><b>Menu / Sistema</b></summary>

- Menu no jogo (`INSERT`): 5 abas (Visuals / Move / View / Combat / Misc),
  scroll por aba, logo RGB, ajudas `?` (EN + RU), swatches de cor, binds de
  teclas, sliders, tecla pânico que esconde o ESP na hora
- 8 idiomas com troca ao vivo (botão ou `F7`):
  EN / RU / DE / ES / PT / PL / FR / ZH
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`),
  `bool / int / float / Color`, sliders float com proxy int e resync
- Logger — `%TEMP%/ZenWare.log` muda para `<gamedir>/ZenWare.log`
  (fallback por PID), crash records como `module+offset` com breadcrumbs
- Cache de offsets — `<gamedir>/ZenWare.offsets` (auto, apague para
  reescanear); `Tools/SigScan` verifica os 15 padrões offline contra seus
  arquivos do jogo

</details>

### Compilar e jogar — só local

**1. Compilar** (Visual Studio 2022 com `Desenvolvimento para desktop com C++`):

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Jogo** — Steam → L4D2 → opções de inicialização:

```text
-insecure -windowed -console
```

**3. Mapa** — console do jogo:

```text
map c1m1_hotel
```

**4. Injetar** — rode seu loader compilado **como administrador** → `INJECT`.

Só external (sem injeção): rode seu
`ZenWare.External/bin/Release/ZenWare.External.exe` local ou `START-ZenWare.bat`.

> Não suba o `exe` compilado nos Releases públicos. Política só-fontes: [SECURITY.md](SECURITY.md).

### Requisitos

| O quê | Versão |
|---|---|
| SO | Windows 10/11 x64 |
| Jogo | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desenvolvimento para desktop com C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Alvo | `Release` · `Win32` (x86) |

### Teclas

| Tecla | Ação |
|:---:|---|
| `INSERT` | Abrir / fechar menu |
| `F11` | Descarregar |
| `F7` | Idioma EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (segurar) | BunnyHop (padrão) |
| `MOUSE4` (segurar) | Aimbot (padrão) |

Reatribuir: `Misc → Menu key / Aimbot key` — clique → `[press key]` → aperte a tecla, `ESC` = off.
A tecla pânico esconde as caixas na hora (`Visuals → ESP`).

### Configs e logs

- Slots junto ao caminho DLL do jogo: `ZenWare.cfg`, `ZenWare2.cfg`,
  `ZenWare3.cfg` — `key=value` puro, 117 chaves. Valores de sessão
  (contadores JB/EB) nunca são salvos.
- Log: `<gamedir>/left4dead2/ZenWare.log`. Um crash aparece como
  `[!!!] EXCEPTION code=... at module+0x...` + último breadcrumb — esse par
  basta, sem dumps.
- Cache de assinaturas: `<gamedir>/left4dead2/ZenWare.offsets` (RVAs +
  impressão do módulo); após atualizar o jogo apague ou rode
  `Verify-Signatures.bat` e corrija `Offsets.cpp` se disser `NOT FOUND`.

### Estrutura do projeto

```text
ZenWare.cc/
├── ZenWare.sln                  solution (DLL + Loader + External)
├── ZenWare.DLL/                 módulo interno
│   ├── src/Entry/               ordem de init, crash recorders, unload
│   ├── src/Hooks/               ClientMode / EngineVGui / ModelRender / WndProc …
│   ├── src/SDK/                 interfaces L4D2, entidades, DrawManager, GameUtil
│   ├── src/Features/            Vars (ajustes) + Aimbot/ESP/Menu/… + Config + Lang
│   └── src/Util/                Logger, Pattern, cache Offsets, NetVars, MinHook
├── ZenWare.Loader/              GUI loader — só build local, sem auto-update
├── ZenWare.External/            overlay externo — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               verificador de assinaturas para sua client.dll
├── .github/workflows/           check CI de compilação — sem subir binários
├── dist/                        só saída local (git-ignored)
├── Build-SingleFile.ps1         builder local (sem Publish)
├── Verify-Signatures.bat        check de padrões em um clique
├── START-ZenWare.bat            arranque local do external
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
└── README_RU/DE/ES/PT/PL/FR/ZH.md   traduções deste arquivo
```

### Problemas

| Sintoma | Solução |
|---|---|
| `XorString` / `FATAL ERROR` ao iniciar | Assinatura quebrada após update do jogo → `Verify-Signatures.bat`, atualize `Offsets.cpp` |
| Crash no jogo | Abra o log → ache `[!!!] EXCEPTION` → reporte `module+0x...` + último breadcrumb, sem dumps |
| Alerta de antivírus | Pasta → exclusões (heurística em `WriteProcessMemory` / `CreateRemoteThread`) |
| Quadrados em vez de letras | Aperte `F7` (fallback de fonte / idioma) |
| External: sem caixas | Veja as linhas superiores do overlay (diagnóstico do resolver) e assinaturas |
| Menu aberto mas cliques vão ao jogo | Feche primeiro o menu do jogo (o cheat captura `ESC` só com seu menu aberto) |

## Legal

| Tema | Termos |
|---|---|
| Licença | MIT — [LICENSE](LICENSE), `AS IS`, sem garantia |
| Terceiros | [NOTICE.md](NOTICE.md): MinHook, FontAwesome, headers da Valve, base |
| Marcas | Left 4 Dead 2 / Steam da Valve. Sem afiliação nem endosso |
| Distribuição | Só fontes — [SECURITY.md](SECURITY.md), sem binários |
| Uso | Só `-insecure` local. Sem bypass de VAC. Os bans são seus |

<div align="center">

**Créditos** ·
Base: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Motor de hooks: MinHook (Tsuda Kageyu, estilo BSD) ·
Ícones: [FontAwesome 6 Free](https://fontawesome.com)

</div>
