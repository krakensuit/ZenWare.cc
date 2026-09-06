# ZenWare single-file builder.
# Sobiraet DLL + External + Loader i kladet ODIN file v dist\ dlya otpravki drugu.
# Vnutri loadera uzhe vshity ZenWare.dll, ZenWare.External.exe i logotip -
# na chuzhom PK oni sami raspakuyutsya v %TEMP% pri nazhatii knopok.
#
# Zapusk: powershell -ExecutionPolicy Bypass -File Build-SingleFile.ps1
param(
	[string]$Configuration = "Release",
	[string]$OutName = "ZenWare.exe",
	[switch]$Publish
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

# --- nayti MSBuild ---
$msbuild = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
	$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
}
if (-not $msbuild) {
	foreach ($p in @(
		"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
		"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
	)) { if (Test-Path $p) { $msbuild = $p; break } }
}
if (-not $msbuild) { throw "MSBuild ne nayden. Postav VS 2022 (Build Tools hvatit)." }
"MSBuild: $msbuild"

# --- sobrat vse (poryadok DLL -> External -> Loader uzhe propisan v ZenWare.sln) ---
& $msbuild "$root\ZenWare.sln" -p:Configuration=$Configuration -p:Platform=x86 -m -v:m -nologo
if ($LASTEXITCODE -ne 0) { throw "Sborka upala." }

$loader = "$root\ZenWare.Loader\bin\$Configuration\ZenWare.Loader.exe"
if (!(Test-Path $loader)) { throw "Net vyhodnogo loadera: $loader" }
$kb = [math]::Round((Get-Item $loader).Length / 1KB)
if ((Get-Item $loader).Length -lt 2097152) { throw "Loader podozritelno mal ($kb KB) - resursy ne vshilis. Soberi DLL i External v $Configuration." }
"Loader: $kb KB (DLL + External + logo vnutri)"

# --- odin file v dist ---
$dist = "$root\dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item $loader "$dist\$OutName" -Force
$howto = @(
	"ZenWare - kak zapustit (tolko svoy lokalny server!):",
	"1. Zapusti igru (knopka v loadere ili Steam).",
	"2. V konsoli igry: map c1m1_hotel (ili lyubaya lokalnaya karta).",
	"3. V loadere vyberi rezhim (tabletka sverhu): EXTERNAL ili INTERNAL,",
	"   nazhmi bolshuyu knopku. Vse nuzhnoe raspakuetsya samo.",
	"4. Menyu v igre: INSERT. Vygruzka chita: F11. F7 - smena yazyka RU/EN.",
	"5. Obnovleniya priletayut sami pri zapuske loadera (sprosit).",
	"5. EXTERNAL: esli net boksov - sfotkay 3 verhnie stroki overleya i prishli.",
	"6. Esli v menu/vmesto bukv kvadratiki - nazhmi F7 (smena yazyka)."
)
$howto | Set-Content "$dist\KAK-ZAPUSTIT.txt" -Encoding UTF8

"Gotovo: $dist\$OutName - etot ODIN file i kiday drugu."
if ($Publish) {
	$m = Select-String -Pattern 'define ZENWARE_VER_STR "([^"]+)"' -Path "$root\ZenWare.Loader\resource.h"
	if (-not $m) { throw "Ne nashel versiyu v resource.h" }
	$tag = "v" + $m.Matches[0].Groups[1].Value
	$gh = Get-Command gh -ErrorAction SilentlyContinue
	if (-not $gh) { throw "Net GitHub CLI (gh). Postav: winget install GitHub.cli, zatem gh auth login. Ili sozday reliz v web: tag $tag + asset dist\$OutName" }
	& gh release create $tag "$dist\$OutName" --title $tag --notes "ZenWare $tag single-file (loader + DLL + external + logo)"
	if ($LASTEXITCODE -ne 0) { throw "gh release create upal (mozhet, takoy tag uzhe est?)" }
	"Opublikovano: $tag - druzya poluchat obnovu avtomaticheski pri zapuske."
}
