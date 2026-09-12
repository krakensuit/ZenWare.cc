# ZenWare.cc — Polski

**Wewnętrzne + zewnętrzne oprogramowanie treningowe do Left 4 Dead 2**

`x86` · `C++17` · `Visual Studio 2022` · `MinHook` · `v3.8`

> **Tylko kod źródłowy · Tylko nauka i serwery lokalne.**
> Brak skompilowanych `.exe` / `.dll` w tym repo i w Releases — zbuduj ze źródeł,
> uruchom grę z `-insecure`, graj na własnej mapie (`map c1m1_hotel`).
> Serwery z VAC = ryzyko bana. Używasz na własne ryzyko.

🇬🇧 [English](README.md) · 🇷🇺 [Русский](README_RU.md) · 🇩🇪 [Deutsch](README_DE.md) · 🇪🇸 [Español](README_ES.md) · 🇵🇹 [Português](README_PT.md) · 🇫🇷 [Français](README_FR.md) · 🇨🇳 [中文](README_ZH.md)

## O projekcie

ZenWare.cc to piaskownica treningowa do Left 4 Dead 2 (silnik Source, 2009):
render, predykcja, ruch i sieć — na własnym PC, na własnym serwerze, z
`-insecure`. Bez omijania VAC, bez oficjalnych serwerów, bez gotowych
binarek.

Kod wyrósł z [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base)
do trzech narzędzi w jednym rozwiązaniu (`ZenWare.sln`, `Release | Win32`):

| Część | Tryb | Co robi | Wynik |
|---|---|---|---|
| `ZenWare.DLL` | Internal (wstrzykiwany) | Pełny cheat: narożnikowy ESP, chamsy, aimbot, ruch, menu w grze | `bin/Release/ZenWare.dll` (x86, /MT) |
| `ZenWare.Loader` | GUI loader, tylko lokalny build | Inject manual-map / LoadLibrary, skórki, 8 języków | lokalny `dist/ZenWare.exe`, nigdy nie publikowany |
| `ZenWare.External` | External (bez injectu) | Odczyt RPM + nakładka GDI + bhop/strafe przez `SendInput` | `bin/Release/ZenWare.External.exe` |

Nowy w temacie? Zacznij od **External** — nigdy nie zapisuje do pamięci gry,
najbezpieczniejsza droga do nauki pipeline'u.
Szczegóły: [SECURITY.md](SECURITY.md) · Obcy kod: [NOTICE.md](NOTICE.md) ·
Licencja: [LICENSE](LICENSE) · Budowa: [ARCHITECTURE.md](ARCHITECTURE.md).

### Jak to działa

**Wewnętrzna DLL.** Przy wstrzyknięciu `DllMain` tworzy wątek (logika nigdy
nie działa na loader locku), a `Entry::Load` idzie po uporządkowanej ścieżce
fail-closed: logger → rejestratory crashy → czekanie na `serverbrowser.dll` →
skan sygnatur (z cache RVA `ZenWare.offsets`) → interfejsy (`VEngineCvar007`
najpierw z `vstdlib.dll`) → netvary → hooki `MinHook` → wczytanie configu.
Brak sygnatury lub interfejsu = message box, nie crash.

Na każdą klatkę działają dwa łańcuchy:

- **Tick** (`ClientMode::CreateMove`) — predykcja, ruch (BunnyHop →
  AutoStrafe → JumpStats → AutoShove), walka (Aimbot → TriggerBot →
  AutoPistol → NoSpread). Oryginał silnika wołany jest dokładnie raz na tick.
- **Klatka** (`EngineVGui::Paint`, tylko `PAINT_UIPANELS`) — cvary
  thirdperson / fullbright / hide-hands, tick killfeeda i hitmarkera, ESP,
  radar, alerty, menu, celownik, podgląd granatów, nakładka i animowane rysowanie
  killfeeda / hitmarkera / JumpStats.

Bez eventów silnika z założenia — killfeed i hitmarker odczytują zmiany
HP/alive. Całe rysowanie idzie przez `G::Draw` na `MatSystemSurface`; encje
czytane są tylko przez netvary ze sprawdzeniem `ClassID` przed wołaniem virtuali.

**Loader.** GUI Win32 ze splashem, motywem ciemnym/jasnym z systemu, 8
językami i animowanym logo RGB. DLL, External i logo są wbudowane jako zasoby —
jeden plik wynikowy. Inject manual-map (relokacje, importy, ochrona sekcji,
czyszczenie nagłówków) z fallbackiem do `LoadLibrary`. Bez sieci, bez
auto-aktualizacji.

**External.** Tylko odczyt: snapshoty `ReadProcessMemory` 20 Hz, nakładka GDI
click-through 60 Hz, ruch tylko przez `SendInput`. Resolver (sygnatura +
kotwica + walidacja) odnajduje adresy przy każdym starcie.

### Funkcje

<details open>
<summary><b>Visuals</b> — ESP · Chamsy · Nakładki</summary>

- **ESP** — narożniki (kolor drużyny), pasek HP z ramką + opcjonalny tekst HP,
  nicki z cieniem, dystans, bronie z amunicją magazynka, itemy, zwykłe,
  wszyscy specialni z Boomerem (fallback po nazwie klasy), opcjonalny limit
  dystansu ESP, przygaszone snapline'y
- **Chamsy** — 5 palet, własne kolory wrogów / sojuszników / Tanka, przełącznik
  through-walls, filtr duchów i dormant
- **Świat** — NoFog, Fullbright, osobne suwaki FOV świata + viewmodelu, widok
  z trzeciej osoby z dystansem, chowanie rąk, własny celownik, nakładka FPS /
  pozycji / HP, licznik żywych zwykłych
- **Intel** — radar 2D z rotacją yaw, lista widzów, alarmy Tank / Witch,
  odczytywany killfeed z animowanymi kartami, hitmarker z dźwiękiem, liczby
  obrażeń, krzyżyk trafienia i statystyki sesji
- **Drużyna** — ostrzeżenie o pinie (Smoker/Hunter), alarm revive, pasek HP
  Tanka, panel HP drużyny, lista pobliskich SI z własnym panelem, alarm plucia,
  timery rzuconych granatów (rura / ogień mołotowa)
- **Feedback** — czerwony flash obrażeń, strzałka do ostatniego atakującego,
  filtr minimalnych obrażeń, HUD broni z amunicją (magazynek + zapas), żółte
  `RELOADING` i czerwone ostrzeżenie o małej amunicji
- **Granaty** — podgląd trajektorii (mołotow / rura / żółć /
  granatnik) z symulacją odbić i markerem lądowania

</details>

<details>
<summary><b>Combat</b> — pomoc w celowaniu</summary>

- **Aimbot** — silent, priorytet FOV / dystans, punkt głowa / środek,
  wygładzanie, tylko widoczni, bind klawisza, zwykłe + wszystkie SI + Tank
- **Na broń** — własne FOV / wygładzanie / hitbox na grupę (karabiny, SMG,
  strzelby, snajperki, pistolety)
- **Pomocnicy** — TriggerBot (strzela na promieniu post-aim, bez laga ticka),
  AutoShove (uwalnia przypiętych), AutoPistol, NoSpread (whitelista 11 broni,
  domyślnie włączony)

</details>

<details>
<summary><b>Movement</b> — bhop i strafy</summary>

- **Zestaw bhop** — BunnyHop perfect / legit, EdgeJump, EdgeBug, JumpBug,
  pomocnik LongJump, FastStop, Prestrafe, AutoDuck, regulowane opóźnienie skoku
- **Strafe** — AutoStrafe legit / rage / w-only / directional
- **JumpStats** — panel w stylu KZ: dystans, prestrafe, max prędkość, upadek,
  wysokość skoku, czas w powietrzu, sync % na żywo, liczba strafów, liczniki
  JB / EB z sesji

</details>

<details>
<summary><b>Menu / System</b></summary>

- Menu w grze (`INSERT`): 5 zakładek (Visuals / Move / View / Combat / Misc),
  scroll na zakładkę, logo RGB, pomoce `?` (EN + RU), próbki kolorów, bindy,
  suwaki, klawisz paniki chowa ESP natychmiast
- 8 języków z przełączaniem na żywo (przycisk lub `F7`):
  EN / RU / DE / ES / PT / PL / FR / ZH
- Config — 3 sloty (`ZenWare.cfg` / `ZenWare2.cfg` / `ZenWare3.cfg`),
  `bool / int / float / Color`, suwaki float z proxy int i resync
- Logger — `%TEMP%/ZenWare.log` wędruje do `<gamedir>/ZenWare.log`
  (fallback per-PID), rekordy crashy jako `module+offset` z breadcrumbami
- Cache offsetów — `<gamedir>/ZenWare.offsets` (auto, usuń by przeskanować);
  `Tools/SigScan` weryfikuje 15 sygnatur offline na twoich plikach gry

</details>

### Budowanie i gra — tylko lokalnie

**1. Zbuduj** (Visual Studio 2022 z `Programowanie aplikacji klasycznych w C++`):

```powershell
powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
```

**2. Gra** — Steam → L4D2 → opcje startu:

```text
-insecure -windowed -console
```

**3. Mapa** — konsola w grze:

```text
map c1m1_hotel
```

**4. Inject** — uruchom własny loader **jako administrator** → `INJECT`.

Tylko external (bez injectu): uruchom lokalny
`ZenWare.External/bin/Release/ZenWare.External.exe` albo `START-ZenWare.bat`.

> Nie wrzucaj zbudowanego `exe` do publicznych Releases. Polityka tylko-źródła: [SECURITY.md](SECURITY.md).

### Wymagania

| Co | Wersja |
|---|---|
| OS | Windows 10/11 x64 |
| Gra | Steam + Left 4 Dead 2 |
| IDE | Visual Studio 2022 + `Programowanie aplikacji klasycznych w C++` |
| SDK | Windows SDK `10.0.26100.0` |
| Cel | `Release` · `Win32` (x86) |

### Klawisze

| Klawisz | Akcja |
|:---:|---|
| `INSERT` | Otwórz / zamknij menu |
| `F11` | Wyładuj |
| `F7` | Język EN / RU / DE / ES / PT / PL / FR / ZH |
| `Space` (przytrzymaj) | BunnyHop (domyślnie) |
| `MOUSE4` (przytrzymaj) | Aimbot (domyślnie) |

Przebindowanie: `Misc → Menu key / Aimbot key` — klik → `[press key]` → wciśnij klawisz, `ESC` = off.
Klawisz paniki chowa boksy natychmiast (`Visuals → ESP`).

### Configi i logi

- Sloty obok ścieżki DLL gry: `ZenWare.cfg`, `ZenWare2.cfg`,
  `ZenWare3.cfg` — zwykłe `key=value`, 117 kluczy. Wartości sesyjne
  (liczniki JB/EB) nigdy nie są zapisywane.
- Log: `<gamedir>/left4dead2/ZenWare.log`. Crash wygląda tak:
  `[!!!] EXCEPTION code=... at module+0x...` + ostatni breadcrumb — ta para
  wystarcza, bez dumpów.
- Cache sygnatur: `<gamedir>/left4dead2/ZenWare.offsets` (RVA + odcisk
  modułu); po aktualizacji gry usuń albo uruchom `Verify-Signatures.bat`
  i napraw `Offsets.cpp` gdy pokaże `NOT FOUND`.

### Struktura projektu

```text
ZenWare.cc/
├── ZenWare.sln                  rozwiązanie (DLL + Loader + External)
├── ZenWare.DLL/                 moduł wewnętrzny
│   ├── src/Entry/               kolejność inita, rejestratory crashy, unload
│   ├── src/Hooks/               ClientMode / EngineVGui / ModelRender / WndProc …
│   ├── src/SDK/                 interfejsy L4D2, encje, DrawManager, GameUtil
│   ├── src/Features/            Vars (ustawienia) + Aimbot/ESP/Menu/… + Config + Lang
│   └── src/Util/                Logger, Pattern, cache Offsets, NetVars, MinHook
├── ZenWare.Loader/              GUI loader — tylko lokalny build, bez auto-update
├── ZenWare.External/            nakładka zewnętrzna — ESP, Memory, Movement, Overlay
├── Tools/SigScan/               weryfikator sygnatur dla twojej client.dll
├── .github/workflows/           check CI kompilacji — bez uploadu binarek
├── dist/                        tylko lokalny wynik (git-ignored)
├── Build-SingleFile.ps1         lokalny builder (bez Publish)
├── Verify-Signatures.bat        check sygnatur jednym klikiem
├── START-ZenWare.bat            lokalny start externala
├── README.md · LICENSE · NOTICE.md · SECURITY.md · CHANGELOG.md · ARCHITECTURE.md
└── README_RU/DE/ES/PT/PL/FR/ZH.md   tłumaczenia tego pliku
```

### Problemy

| Objaw | Rozwiązanie |
|---|---|
| `XorString` / `FATAL ERROR` przy starcie | Sygnatura padła po update gry → `Verify-Signatures.bat`, napraw `Offsets.cpp` |
| Crash w grze | Otwórz log → znajdź `[!!!] EXCEPTION` → zgłoś `module+0x...` + ostatni breadcrumb, bez dumpów |
| Alarm antywirusa | Folder → wykluczenia (heurystyka na `WriteProcessMemory` / `CreateRemoteThread`) |
| Kwadraty zamiast liter | Wciśnij `F7` (fallback fontu / język) |
| External: brak boksów | Sprawdź górne linie nakładki (diagnostyka resolvera) i sygnatury |
| Menu otwarte, a kliki lecą do gry | Najpierw zamknij menu gry (cheat łapie `ESC` tylko z własnym menu otwartym) |

## Kwestie prawne

| Temat | Warunki |
|---|---|
| Licencja | MIT — [LICENSE](LICENSE), `AS IS`, bez gwarancji |
| Obcy kod | [NOTICE.md](NOTICE.md): MinHook, FontAwesome, nagłówki Valve, baza |
| Znaki towarowe | Left 4 Dead 2 / Steam należą do Valve. Bez powiązania i poparcia |
| Dystrybucja | Tylko źródła — [SECURITY.md](SECURITY.md), bez binarek |
| Użycie | Tylko lokalny `-insecure`. Bez omijania VAC. Bany są twoje |

<div align="center">

**Podziękowania** ·
Baza: [Lak3/l4d2-internal-base](https://github.com/Lak3/l4d2-internal-base) ·
Silnik hooków: MinHook (Tsuda Kageyu, styl BSD) ·
Ikony: [FontAwesome 6 Free](https://fontawesome.com)

</div>
