# ZenWare.cc — Français

**Logiciel d'entraînement interne + externe pour Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.6`

> **Sources uniquement · Apprentissage et serveurs locaux uniquement.**
> Aucun `.exe` / `.dll` compilé dans ce dépôt ni dans les Releases — compile depuis les sources,
> lance le jeu avec `-insecure`, joue sur ta propre carte (`map c1m1_hotel`).
> Serveurs VAC = risque de bannissement. À tes risques et périls.

## À propos

Basé sur [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base). Trois parties, une solution (`ZenWare.sln`, `Release | Win32`) :

| Partie | Sortie |
|---|---|
| `ZenWare.DLL` — module interne | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — lanceur GUI | `dist/ZenWare.exe` local, jamais publié, pastille vers les releases |
| `ZenWare.External` — overlay lecture seule | `bin/Release/ZenWare.External.exe` |

## Fonctionnalités

<details open>
<summary><b>Visuels</b> — ESP · Chams · Overlays</summary>

- **ESP** — cadres, barre de PV, noms, distance, armes/objets, communs, tous les Infectés spéciaux incl. Boomer (repli sur nom de classe pour builds étrangers)
- **Chams** — 5 palettes, alliés / ennemis / tous les SI, à travers les murs
- **Monde** — NoFog, FOV monde + arme, troisième personne, réticule personnalisé, overlay FPS
- **Intel** — radar 2D, liste des spectateurs, alertes Tank / Witch, killfeed, hitmarker (son + chiffres de dégâts)
- **Équipe** — alerte plaquage, alerte réanimation, barre de PV du tank, panneau PV d'équipe, liste des SI proches
- **Retour** — croix des hits, stats de session (hits/tirs/précision), flash de dégâts, filtre dégâts min, minuteurs des grenades lancées
- **Grenades** — aperçu de trajectoire (molotov / pipe / bile / lance-grenades) avec marqueur d'atterrissage

</details>

<details>
<summary><b>Combat</b> — aide à la visée</summary>

- **Aimbot** — silencieux, priorité FOV / Distance, tête / corps, lissage, visibles seuls, communs + tous les SI
- **Par arme** — FOV / lissage / hitbox propres par groupe (fusils, SMG, pompes, snipers, pistolets)
- **Assistants** — TriggerBot, AutoShove, AutoPistol, NoSpread (activables)

</details>

<details>
<summary><b>Mouvement</b> — bhop et strafe</summary>

- **Kit bhop** — BunnyHop parfait / légit, EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- **Strafe** — AutoStrafe légit / rage / w-only / directionnel

</details>

<details>
<summary><b>Menu / Système</b></summary>

- Menu `INSERT` · déchargement `F11` · langues EN/RU/DE/ES/PT/PL/FR (bouton + `F7`) · logo RGB animé · icônes d'aide `?` · onglets · molette
- Config — 3 slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`, `bool / int / float / Color`)
- Logger — `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (repli par pid)
- Cache d'offsets — `<gamedir>/ZenWare.offsets` (auto, supprime pour forcer un rescan)

</details>

### Compiler et lancer — en local uniquement

**1. Compiler**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Préparer le jeu** — Steam → L4D2 → options de lancement :

```text
-insecure -windowed -console
```

**3. Charger une carte locale** — console du jeu :

```text
map c1m1_hotel
```

**4. Injecter** — lance ton loader compilé localement **en administrateur** → `INJECT`.

External seul (sans injection) : lance en local
`ZenWare.External/bin/Release/ZenWare.External.exe` ou `START-ZenWare.bat`.

> Ne téléverse pas l'`exe` compilé dans les Releases GitHub publiques. Politique sources uniquement : [SECURITY.md](SECURITY.md).

### Prérequis

| Item | Version |
|---|---|
| OS | Windows 10/11 x64 |
| Game | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Target | `Release` · `Win32` (x86) |

### Touches

| Key | Action |
|:---:|---|
| `INSERT` | Ouvrir / fermer le menu |
| `F11` | Décharger |
| `F7` | Langue EN / RU / DE / ES / PT / PL / FR |
| `Space` (maintenir) | BunnyHop (défaut) |
| `MOUSE4` (maintenir) | Aimbot (défaut) |

Réassigner : `Misc → Menu key / Aimbot key` — clic → `[press key]` → appuie sur une touche, `ESC` = off.

## Structure du projet

```text
ZenWare.cc/
├── ZenWare.sln                  solution (DLL + Loader + External)
├── ZenWare.DLL/                 module interne (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/              lanceur GUI — build local uniquement, sans auto-update
├── ZenWare.External/            overlay externe — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               vérificateur de signatures pour ton client.dll local
├── .github/workflows/           check de compilation CI — sans envoi de binaires
├── dist/                        sortie locale uniquement (git-ignored)
├── Build-SingleFile.ps1         builder local (sans Publish)
├── Verify-Signatures.bat        check des patterns en un clic vs tes fichiers de jeu
├── START-ZenWare.bat            lanceur local de l'external
├── README.md · README_DE.md · README_ES.md · README_PT.md · README_PL.md · README_FR.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

## Dépannage

| Symptôme | Solution |
|---|---|
| MessageBox `XorString` au démarrage | Signature cassée après une mise à jour du jeu → double-clic `Verify-Signatures.bat`, mets à jour `Offsets.cpp` |
| Crash après `Paint` | Ouvre le log → trouve `[!!!] EXCEPTION` → signale `module+0x...`, sans dumps |
| Alerte antivirus | Dossier → exclusions (heuristique sur `WriteProcessMemory` / `CreateRemoteThread`) |
| Carrés au lieu de lettres | Appuie sur `F7` (police de secours / langue) |
| External : pas de cadres | Vérifie les lignes du haut de l'overlay (diagnostic du resolver) et les signatures |
