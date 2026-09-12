# ZenWare.cc — Polski

**Wewnętrzne + zewnętrzne oprogramowanie treningowe do Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.6`

> **Tylko kod źródłowy · Tylko nauka i serwery lokalne.**
> Brak skompilowanych `.exe` / `.dll` w tym repo i w Releases — zbuduj ze źródeł,
> uruchom grę z `-insecure`, graj na własnej mapie (`map c1m1_hotel`).
> Serwery z VAC = ryzyko bana. Używasz na własne ryzyko.

## O projekcie

Na podstawie [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base). Trzy części, jedno rozwiązanie (`ZenWare.sln`, `Release | Win32`):

| Część | Wynik |
|---|---|
| `ZenWare.DLL` — moduł wewnętrzny | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` — GUI loader | lokalny `dist/ZenWare.exe`, nigdy nie publikowany, pasek do releases |
| `ZenWare.External` — nakładka tylko do odczytu | `bin/Release/ZenWare.External.exe` |

## Funkcje

<details open>
<summary><b>Wygląd</b> — ESP · Chams · Nakładki</summary>

- **ESP** — ramki, pasek HP, nazwy, dystans, bronie/przedmioty, zwykli, wszyscy Specjalni Zarażeni wł. Boomerem (fallback nazwy klasy dla obcych buildów)
- **Chams** — 5 palet, sojusznicy / wrogowie / wszyscy SI, przez ściany
- **Świat** — NoFog, FOV świata + broni, trzecia osoba, własny celownik, nakładka FPS
- **Intel** — radar 2D, lista widzów, alerty Tank / Witch, killfeed, hitmarker (dźwięk + liczby obrażeń)
- **Drużyna** — ostrzeżenie o pinie, alert reanimacji, pasek HP Tanka, panel HP drużyny, lista pobliskich SI
- **Feedback** — krzyżyk trafień, staty sesji (trafienia/strzały/celność), błysk obrażeń, filtr min. obrażeń, timery rzuconych granatów
- **Granaty** — podgląd toru lotu (mołotow / rura / żółć / granatnik) z markerem lądowania

</details>

<details>
<summary><b>Walka</b> — wspomaganie celowania</summary>

- **Aimbot** — cichy, priorytet FOV / dystansu, głowa / ciało, wygładzanie, tylko widoczni, zwykli + wszyscy SI
- **Na broń** — własne FOV / wygładzanie / hitbox na grupę (karabiny, PM, strzelby, snajperki, pistolety)
- **Pomocnicy** — TriggerBot, AutoShove, AutoPistol, NoSpread (przełączane)

</details>

<details>
<summary><b>Ruch</b> — bhop i strafe</summary>

- **Zestaw bhop** — BunnyHop idealny / legit, EdgeJump, EdgeBug, JumpBug, LongJump, FastStop, Prestrafe, AutoDuck, JumpStats, SpeedHUD
- **Strafe** — AutoStrafe legit / rage / w-only / kierunkowy

</details>

<details>
<summary><b>Menu / System</b></summary>

- Menu `INSERT` · wyładunek `F11` · języki EN/RU/DE/ES/PT/PL (przycisk + `F7`) · animowane logo RGB · ikonki pomocy `?` · zakładki · scroll myszy
- Config — 3 sloty (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`, `bool / int / float / Color`)
- Logger — `%TEMP%/ZenWare.log` → `<gamedir>/ZenWare.log` (fallback po pid)
- Cache offsetów — `<gamedir>/ZenWare.offsets` (auto, usuń by wymusić rescan)

</details>

### Budowa i uruchomienie — tylko lokalnie

**1. Zbuduj**

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Przygotuj grę** — Steam → L4D2 → opcje uruchamiania:

```text
-insecure -windowed -console
```

**3. Wczytaj lokalną mapę** — konsola gry:

```text
map c1m1_hotel
```

**4. Inject** — uruchom lokalnie zbudowany loader **jako administrator** → `INJECT`.

External-only (bez injectu): uruchom lokalny
`ZenWare.External/bin/Release/ZenWare.External.exe` lub `START-ZenWare.bat`.

> Nie wysyłaj zbudowanego `exe` na publiczne GitHub Releases. Polityka source-only: [SECURITY.md](SECURITY.md).

### Wymagania

| Item | Version |
|---|---|
| OS | Windows 10/11 x64 |
| Game | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Desktop development with C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Target | `Release` · `Win32` (x86) |

### Klawisze

| Key | Action |
|:---:|---|
| `INSERT` | Otwórz / zamknij menu |
| `F11` | Wyładuj |
| `F7` | Język EN / RU / DE / ES / PT / PL |
| `Space` (przytrzymaj) | BunnyHop (domyślnie) |
| `MOUSE4` (przytrzymaj) | Aimbot (domyślnie) |

Przebindowanie: `Misc → Menu key / Aimbot key` — klik → `[press key]` → naciśnij klawisz, `ESC` = wył.

## Struktura projektu

```text
ZenWare.cc/
├── ZenWare.sln                  rozwiązanie (DLL + Loader + External)
├── ZenWare.DLL/                 moduł wewnętrzny (+ MinHook, SDK, Features, Hooks)
├── ZenWare.Loader/              GUI loader — tylko lokalny build, bez auto-update
├── ZenWare.External/            nakładka zewnętrzna — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               weryfikator sygnatur dla twojego lokalnego client.dll
├── .github/workflows/           check kompilacji CI — bez wysyłania binarek
├── dist/                        tylko lokalny wynik (git-ignored)
├── Build-SingleFile.ps1         lokalny builder (bez Publish)
├── Verify-Signatures.bat        jednoklikowy check wzorców vs twoje pliki gry
├── START-ZenWare.bat            lokalny launcher externala
├── README.md · README_DE.md · README_ES.md · README_PT.md · README_PL.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
```

## Rozwiązywanie problemów

| Objaw | Rozwiązanie |
|---|---|
| MessageBox `XorString` na starcie | Sygnatura padła po updacie gry → 2× klik `Verify-Signatures.bat`, zaktualizuj `Offsets.cpp` |
| Crash po `Paint` | Otwórz log → znajdź `[!!!] EXCEPTION` → zgłoś `moduł+0x...`, bez dumpów |
| Alarm antywirusa | Folder → wykluczenia (heurystyka na `WriteProcessMemory` / `CreateRemoteThread`) |
| Kwadraty zamiast liter | Naciśnij `F7` (fallback fontu / język) |
| External: brak ramek | Sprawdź górne linie nakładki (diagnostyka resolvera) i sygnatury |
