<div align="center">

<img src="ZenWare.Loader/zenwareLOGO.png" width="170" alt="ZenWare logo" />

# ZenWare.cc (Deutsch)

**Interne + externe Trainingssoftware für Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.6`

[← Zurück zur Haupt-README](README.md)

</div>

---

> **Nur Sourcecode · Nur Training und lokale Server.**
> Keine fertigen `.exe` / `.dll` in diesem Repo oder den Releases — aus dem Sourcecode bauen,
> Spiel mit `-insecure` starten, eigene Map spielen (`map c1m1_hotel`).
> VAC-geschützte Server = Banngefahr. Nutzung auf eigenes Risiko.

## Deutsch

### Über das Projekt

Basiert auf [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base). Drei Teile, eine Solution (`ZenWare.sln`, `Release | Win32`):

| Teil | Ausgabe |
|---|---|
| `ZenWare.DLL` — internes Modul | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — GUI-Loader | lokales `dist/ZenWare.exe`, wird nie veröffentlicht, Releases-Pill |
| `ZenWare.External` — Read-only-Overlay | `bin/Release/ZenWare.External.exe` |

### Features

<details open>
<summary><b>Visuals</b> — ESP · Chams · Overlays</summary>

- **ESP** — Boxen, Lebensbalken, Namen, Distanz, Waffen/Items, Commons, alle Special Infected inkl. Boomer (Klassennamen-Fallback für fremde Builds)
- **Chams** — 5 Paletten, Verbündete / Gegner / alle SI, durch Wände
- **Welt** — NoFog, Welt- + Viewmodel-FOV, Thirdperson, eigenes Fadenkreuz, FPS-Overlay
- **Intel** — 2D-Radar, Zuschauerliste, Tank- / Witch-Alarme, Killfeed, Hitmarker (Hitsound + Schadenszahlen)
- **Teamplay** — Pin-Warnung, Revive-Alarm, Tank-HP-Balken, Team-HP-Panel, SI-in-der-Nähe-Liste
- **Feedback** — Trefferkreuz, Session-Stats (Treffer/Schüsse/Genauigkeit), Schadensblitz, Min-Schaden-Filter, Timer für geworfene Granaten
- **Wurfgeschosse** — Granaten-Flugbahn-Vorschau (Molotov / Pipe / Bile / Granatwerfer) mit Landemarkierung

</details>

<details>
<summary><b>Kampf</b> — Aim-Hilfe</summary>

- **Aimbot** — Silent, FOV- / Distanz-Priorität, Kopf / Mitte, Smoothing, nur sichtbar, Commons + alle SI
- **Pro Waffe** — eigenes FOV / Smoothing / Hitbox pro Gruppe (Gewehre, MP, Schrotflinten, Sniper, Pistolen)
- **Helfer** — TriggerBot, AutoShove, AutoPistol, NoSpread (umschaltbar)

</details>

<details>
<summary><b>Movement</b> — Bhop & Strafe</summary>

- **Bhop-Kit** — Perfect- / Legit-BunnyHop, EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- **Strafe** — Legit- / Rage- / W-only- / Directional-AutoStrafe

</details>

<details>
<summary><b>Menü / System</b></summary>

- `INSERT` Menü · `F11` Entladen · EN/RU/DE-Umschaltung (Button + `F7`) · animiertes RGB-Logo · `?` Hilfe-Icons · Tabs · Mausrad-Scroll
- Config — 3 Slots (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`, `bool / int / float / Color`)
- Logger — `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (Per-Pid-Fallback)
- Offset-Cache — `<gamedir>/ZenWare.offsets` (automatisch, löschen zum Neu-Scan)

</details>

### Bauen und starten — nur lokal

**1. Bauen**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Spiel vorbereiten** — Steam → L4D2 → Startoptionen:

```text
-insecure -windowed -console
```

**3. Lokale Map laden** — Konsole im Spiel:

```text
map c1m1_hotel
```

**4. Injecten** — lokal gebauten Loader **als Administrator** starten → `INJECT`.

Nur External (ohne Injection): lokale
`ZenWare.External/bin/Release/ZenWare.External.exe` oder `START-ZenWare.bat` starten.

> Die gebaute `exe` nicht in öffentliche GitHub-Releases hochladen. Source-only-Regel: [SECURITY.md](SECURITY.md).

### Voraussetzungen

| Punkt | Version |
|---|---|
| OS | Windows 10/11 x64 |
| Spiel | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Target | `Release` · `Win32` (x86) |

### Hotkeys

| Taste | Aktion |
|:---:|---|
| `INSERT` | Menü öffnen / schließen |
| `F11` | Entladen |
| `F7` | Sprache EN / RU / DE |
| `Space` (halten) | BunnyHop (Standard) |
| `MOUSE4` (halten) | Aimbot (Standard) |

Umbinden: `Misc → Menu key / Aimbot key` — klicken → `[press key]` → Taste drücken, `ESC` = aus.

### Projektstruktur

```text
ZenWare.cc/
├── ZenWare.sln                  Solution (DLL + Loader + External)
├── ZenWare.DLL/                 internes Modul (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/              GUI-Loader — nur lokaler Build, kein Auto-Update
├── ZenWare.External/            externes Overlay — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               Signaturprüfer für die lokale client.dll
├── .github/workflows/           CI-Compile-Check — kein Binary-Upload
├── dist/                        nur lokale Ausgabe (git-ignoriert)
├── Build-SingleFile.ps1         lokaler Builder (kein Publish)
├── Verify-Signatures.bat        Ein-Klick-Pattern-Check gegen die Spieldateien
├── START-ZenWare.bat            lokaler External-Starter
├── README.md · README_DE.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

### Problemlösung

| Symptom | Lösung |
|---|---|
| `XorString`-MessageBox beim Start | Signatur nach Spiel-Update kaputt → `Verify-Signatures.bat` doppelklicken, `Offsets.cpp` anpassen |
| Crash nach `Paint` | Log öffnen → `[!!!] EXCEPTION` suchen → `module+0x...` melden, keine Dumps |
| Antivirus-Alarm | Ordner → Ausnahmen (Heuristik auf `WriteProcessMemory` / `CreateRemoteThread`) |
| Quadrate statt Buchstaben | `F7` drücken (Font-Fallback / Sprache) |
| External: keine Boxen | Obere Overlay-Zeilen prüfen (Resolver-Diagnose) und Signaturen |
