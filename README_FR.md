# ZenWare.cc — Français

**Logiciel d'entraînement interne + externe pour Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.8.1`

> **Sources uniquement · Apprentissage et serveurs locaux uniquement.**
> Aucun `.exe` / `.dll` compilé dans ce dépôt ni dans les Releases — compile depuis les sources,
> lance le jeu avec `-insecure`, joue sur ta propre carte (`map c1m1_hotel`).
> Serveurs VAC = risque de bannissement. À tes risques et périls.

🇬🇧 [English](README.md) · 🇷🇺 [Русский](README_RU.md) · 🇩🇪 [Deutsch](README_DE.md) · 🇪🇸 [Español](README_ES.md) · 🇵🇹 [Português](README_PT.md) · 🇵🇱 [Polski](README_PL.md) · 🇨🇳 [中文](README_ZH.md)

## À propos

ZenWare.cc est un bac à sable d'entraînement pour Left 4 Dead 2 (moteur
Source, 2009) : rendu, prédiction, mouvement et réseau — sur ton PC, sur ton
serveur, avec `-insecure`. Pas de bypass VAC, pas de serveurs officiels, pas
de binaires précompilés.

Le code est né de [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
et a grandi en trois outils dans une solution (`ZenWare.sln`, `Release | Win32`) :

| Partie | Mode | Ce qu'elle fait | Sortie |
|---|---|---|---|
| `ZenWare.DLL` | Interne (injectée) | Cheat complet : ESP en coins, chams, aimbot, mouvement, menu en jeu | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI loader, build local uniquement | Injection manual-map / LoadLibrary, thèmes, 8 langues | `dist/ZenWare.exe` local, jamais publié |
| `ZenWare.External` | Externe (sans injection) | Lecture RPM + overlay GDI + bhop/strafe via `SendInput` | `bin/Release/ZenWare.External.exe` |

Nouveau ici ? Commence par l'**External** — il n'écrit jamais dans la
mémoire du jeu, c'est le moyen le plus sûr d'étudier le pipeline.
Détails : [SECURITY.md](SECURITY.md) · Tiers : [NOTICE.md](NOTICE.md) ·
Licence : [LICENSE](LICENSE) · Structure : [ARCHITECTURE.md](ARCHITECTURE.md).

### Comment ça marche

**DLL interne.** À l'injection, `DllMain` crée un thread (la logique ne tourne
jamais sur le loader lock) et `Entry::Load` suit un init ordonné fail-closed :
logger → crash recorders → attente de `serverbrowser.dll` → scan de patterns
(avec cache RVA `ZenWare.offsets`) → interfaces (`VEngineCvar007` d'abord
dans `vstdlib.dll`) → netvars → hooks `MinHook` → chargement de la config.
Pattern ou interface manquant = message box, pas de crash.

Chaque frame fait tourner deux chaînes :

- **Tick** (`ClientMode::CreateMove`) — prédiction, mouvement (BunnyHop →
  AutoStrafe → JumpStats → AutoShove), combat (Aimbot → TriggerBot →
  AutoPistol → NoSpread). L'original du moteur est appelé exactement une fois
  par tick.
- **Frame** (`EngineVGui::Paint`, `PAINT_UIPANELS` uniquement) — cvars
  thirdperson / fullbright / hide-hands, tick du killfeed et de l'hitmarker,
  ESP, radar, alertes, menu, crosshair, preview des grenades, overlay et dessin
  animé du killfeed / hitmarker / JumpStats.

Pas d'événements moteur by design — killfeed et hitmarker lisent les
transitions HP/alive. Tout le dessin passe par `G::Draw` sur
`MatSystemSurface` ; les entités sont lues uniquement par netvars avec check
de `ClassID` avant tout appel virtuel.

**Loader.** GUI Win32 avec splash, thème sombre/clair du système, 8 langues
et logo RGB animé. DLL, External et logo sont embarqués en ressources — un
seul fichier de sortie. Injection manual-map (relocs, imports, protection des
sections, effacement des headers) avec fallback `LoadLibrary`. Pas de réseau,
pas d'auto-update.

**External.** Lecture seule : snapshots `ReadProcessMemory` à 20 Hz, overlay
GDI click-through à 60 Hz, mouvement uniquement via `SendInput`. Le resolver
(signature + ancre + validation) retrouve les adresses à chaque lancement.

### Fonctionnalités

<details open>
<summary><b>Visuals</b> — ESP · Chams · Overlays</summary>

- **ESP** — coins (couleur d'équipe), barre HP encadrée + texte HP optionnel,
  pseudos ombrés, distance, armes avec balles du chargeur, items, communs,
  tous les infectés spéciaux dont Boomer (fallback par nom de classe), limite
  de distance ESP optionnelle, snaplines discrètes
- **Chams** — 5 palettes, couleurs propres ennemis / alliés / Tank,
  interrupteur through-walls, filtre fantômes et dormant
- **Monde** — NoFog, Fullbright, sliders FOV monde + viewmodel séparés,
  troisième personne avec distance, cacher les mains, crosshair perso, overlay
  FPS / position / HP, compteur de communs vivants
- **Intel** — radar 2D avec rotation yaw, liste des spectateurs, alertes Tank /
  Witch, killfeed lu avec cartes animées, hitmarker avec son, chiffres de
  dégâts, croix d'impact et stats de session
- **Équipe** — alerte de pin (Smoker/Hunter), alerte de revive, barre HP du
  Tank, panneau HP d'équipe, liste des SI proches avec son panneau, alerte de
  crachat, timers des grenades lancées (pipe / feu molotov)
- **Feedback** — flash rouge de dégâts, flèche vers le dernier attaquant,
  filtre de dégâts minimum, HUD d'arme avec balles (chargeur + réserve), alerte
  jaune `RELOADING` et rouge de munitions basses
- **Grenades** — preview de trajectoire (molotov / pipe / bile /
  lance-grenades) avec simulation de rebonds et marqueur d'atterrissage

</details>

<details>
<summary><b>Combat</b> — aide à la visée</summary>

- **Aimbot** — silent, priorité FOV / Distance, point tête / centre,
  lissage, visibles uniquement, touche, communs + tous les SI + Tank
- **Par arme** — FOV / lissage / hitbox propres par groupe (fusils, SMG,
  fusils à pompe, snipers, pistolets)
- **Assistants** — TriggerBot (tire sur le rayon post-aim, sans lag de tick),
  AutoShove (libère les coéquipiers pinnés), AutoPistol, NoSpread (whitelist
  de 11 armes, activé par défaut)

</details>

<details>
<summary><b>Movement</b> — bhop & strafes</summary>

- **Kit bhop** — BunnyHop perfect / legit, EdgeJump, EdgeBug, JumpBug,
  assistant LongJump, FastStop, Prestrafe, AutoDuck, délai de saut réglable
- **Strafe** — AutoStrafe legit / rage / w-only / directional
- **JumpStats** — panneau style KZ : distance, prestrafe, vitesse max, chute,
  hauteur de saut, temps en l'air, sync % en direct, compteur de strafes,
  compteurs JB / EB de session

</details>

<details>
<summary><b>Menu / Système</b></summary>

- Menu en jeu (`INSERT`) : 5 onglets (Visuals / Move / View / Combat / Misc),
  scroll par onglet, logo RGB, aides `?` (EN + RU), nuanciers, binds de
  touches, sliders, touche panique qui cache l'ESP aussitôt
- 8 langues avec changement en direct (bouton ou `F7`) :
  EN / RU / DE / ES / PT / PL / FR / ZH
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`),
  `bool / int / float / Color`, sliders float avec proxy int et resync
- Logger — `%TEMP%/ZenWare.log` migre vers `<gamedir>/ZenWare.log`
  (fallback par PID), crash records en `module+offset` avec breadcrumbs
- Cache d'offsets — `<gamedir>/ZenWare.offsets` (auto, supprime pour
  rescanner) ; `Tools/SigScan` vérifie les 15 patterns hors-ligne sur tes
  fichiers de jeu

</details>

### Compiler & jouer — local uniquement

**1. Compiler** (Visual Studio 2022 avec `Développement Desktop en C++`) :

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Jeu** — Steam → L4D2 → options de lancement :

```text
-insecure -windowed -console
```

**3. Map** — console en jeu :

```text
map c1m1_hotel
```

**4. Injecter** — lance ton loader compilé **en administrateur** → `INJECT`.

External seul (sans injection) : lance ton
`ZenWare.External/bin/Release/ZenWare.External.exe` local ou `START-ZenWare.bat`.

> Ne poste pas l'`exe` compilé dans les Releases publiques. Politique sources-uniquement : [SECURITY.md](SECURITY.md).

### Prérequis

| Quoi | Version |
|---|---|
| OS | Windows 10/11 x64 |
| Jeu | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Développement Desktop en C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Cible | `Release` · `Win32` (x86) |

### Touches

| Touche | Action |
|:---:|---|
| `INSERT` | Ouvrir / fermer le menu |
| `F11` | Décharger |
| `F7` | Langue EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (maintenu) | BunnyHop (défaut) |
| `MOUSE4` (maintenu) | Aimbot (défaut) |

Rebind : `Misc → Menu key / Aimbot key` — clic → `[press key]` → appuie sur la touche, `ESC` = off.
La touche panique cache les boîtes aussitôt (`Visuals → ESP`).

### Configs & logs

- Slots à côté du chemin DLL du jeu : `ZenWare.cfg`, `ZenWare2.cfg`,
  `ZenWare3.cfg` — `key=value` brut, 117 clés. Les valeurs de session
  (compteurs JB/EB) ne sont jamais sauvegardées.
- Log : `<gamedir>/left4dead2/ZenWare.log`. Un crash ressemble à
  `[!!!] EXCEPTION code=... at module+0x...` + dernier breadcrumb — cette paire
  suffit, pas de dumps.
- Cache de signatures : `<gamedir>/left4dead2/ZenWare.offsets` (RVA +
  empreinte du module) ; après une MAJ du jeu supprime-le ou lance
  `Verify-Signatures.bat` et corrige `Offsets.cpp` s'il dit `NOT FOUND`.

### Structure du projet

```text
ZenWare.cc/
├── ZenWare.sln                  solution (DLL + Loader + External)
├── ZenWare.DLL/                 module interne
│   ├── src/Entry/               ordre d'init, crash recorders, unload
│   ├── src/Hooks/               ClientMode / EngineVGui / ModelRender / WndProc …
│   ├── src/SDK/                 interfaces L4D2, entités, DrawManager, GameUtil
│   ├── src/Features/            Vars (réglages) + Aimbot/ESP/Menu/… + Config + Lang
│   └── src/Util/                Logger, Pattern, cache Offsets, NetVars, MinHook
├── ZenWare.Loader/              GUI loader — build local uniquement, sans auto-update
├── ZenWare.External/            overlay externe — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               vérificateur de signatures pour ta client.dll
├── .github/workflows/           check CI de compilation — sans upload de binaires
├── dist/                        sortie locale uniquement (git-ignored)
├── Build-SingleFile.ps1         builder local (sans Publish)
├── Verify-Signatures.bat        check de patterns en un clic
├── START-ZenWare.bat            lancement local de l'external
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
└── README_RU/DE/ES/PT/PL/FR/ZH.md   traductions de ce fichier
```

### Problèmes

| Symptôme | Solution |
|---|---|
| `XorString` / `FATAL ERROR` au démarrage | Signature cassée après MAJ du jeu → `Verify-Signatures.bat`, corrige `Offsets.cpp` |
| Crash en jeu | Ouvre le log → trouve `[!!!] EXCEPTION` → reporte `module+0x...` + dernier breadcrumb, sans dumps |
| Alerte antivirus | Dossier → exclusions (heuristique sur `WriteProcessMemory` / `CreateRemoteThread`) |
| Carrés au lieu de lettres | Appuie sur `F7` (fallback de police / langue) |
| External : pas de boîtes | Regarde les lignes du haut de l'overlay (diagnostic du resolver) et les signatures |
| Menu ouvert mais les clics vont au jeu | Ferme d'abord le menu du jeu (le cheat capte `ESC` uniquement avec son menu ouvert) |

## Mentions légales

| Sujet | Conditions |
|---|---|
| Licence | MIT — [LICENSE](LICENSE), `AS IS`, sans garantie |
| Tiers | [NOTICE.md](NOTICE.md) : MinHook, FontAwesome, headers Valve, base |
| Marques | Left 4 Dead 2 / Steam de Valve. Sans affiliation ni soutien |
| Distribution | Sources uniquement — [SECURITY.md](SECURITY.md), sans binaires |
| Usage | `-insecure` local uniquement. Pas de bypass VAC. Les banns sont pour toi |

<div align="center">

**Crédits** ·
Base : [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Moteur de hooks : MinHook (Tsuda Kageyu, style BSD) ·
Icônes : [FontAwesome 6 Free](https://fontawesome.com)

</div>
