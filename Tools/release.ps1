# ZenWare.cc - release helper.
# One command: bump the version everywhere, insert the CHANGELOG section, build the
# solution and the single-file loader, commit, tag, push and create the GitHub
# release in the repository's reference format.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/release.ps1 -Version 3.14.1 -NotesFile docs/notes-3.14.1.md
#   (add -DryRun to print the plan without changing anything)
#
# The notes file holds the CHANGELOG section for the release (English + Russian)
# and one extra line "ZH|..." with the Chinese summary for the release page.

param(
	[Parameter(Mandatory = $true)][string]$Version,
	[string]$NotesFile,
	[switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$u8 = New-Object System.Text.UTF8Encoding($false)

function ReadU8([string]$p) { [System.IO.File]::ReadAllText($p, $u8) }
function WriteU8([string]$p, [string]$t) { [System.IO.File]::WriteAllText($p, $t, $u8) }
function Step([string]$msg) { Write-Host "== $msg" }

$parts = $Version.Split('.')

if ($parts.Count -ne 3) { throw "Version must look like MAJOR.MINOR.PATCH, got '$Version'" }

$resPath = Join-Path $repo 'ZenWare.Loader\resource.h'
$varsPath = Join-Path $repo 'ZenWare.DLL\src\Features\Vars.h'
$changelogPath = Join-Path $repo 'CHANGELOG.md'
$notesTemplate = Join-Path $PSScriptRoot 'release-notes-template.md'

$res = ReadU8 $resPath
$old = [regex]::Match($res, 'ZENWARE_VER_STR "([^"]+)"').Groups[1].Value

if (-not $old) { throw 'cannot read ZENWARE_VER_STR from resource.h' }

$new = "$($parts[0]).$($parts[1]).$($parts[2])"

Step "bump version $old -> $new"

if (-not $DryRun) {
	$res = $res.Replace("#define ZENWARE_VER_MAJOR $([regex]::Match($res,'ZENWARE_VER_MAJOR (\d+)').Groups[1].Value)", "#define ZENWARE_VER_MAJOR $($parts[0])")
	$res = $res.Replace("#define ZENWARE_VER_MINOR $([regex]::Match($res,'ZENWARE_VER_MINOR (\d+)').Groups[1].Value)", "#define ZENWARE_VER_MINOR $($parts[1])")
	$res = $res.Replace("#define ZENWARE_VER_PATCH $([regex]::Match($res,'ZENWARE_VER_PATCH (\d+)').Groups[1].Value)", "#define ZENWARE_VER_PATCH $($parts[2])")
	$res = $res.Replace("ZENWARE_VER_STR `"$old`"", "ZENWARE_VER_STR `"$new`"")
	WriteU8 $resPath $res

	$vars = ReadU8 $varsPath
	$vars = $vars.Replace("kVersion = `"v$old`"", "kVersion = `"v$new`"")
	WriteU8 $varsPath $vars

	Get-ChildItem $repo -Filter 'README*.md' | ForEach-Object {
		$t = ReadU8 $_.FullName
		WriteU8 $_.FullName ($t.Replace("v$old", "v$new"))
	}
}

Step 'notes and changelog'

$zh = ''

if ($NotesFile) {
	if (-not (Test-Path $NotesFile)) { throw "notes file not found: $NotesFile" }

	$raw = (ReadU8 $NotesFile).Replace("`r`n", "`n")
	$zhAt = $raw.IndexOf('ZH|')

	if ($zhAt -ge 0) {
		$zh = $raw.Substring($zhAt + 3).Trim()
		$raw = $raw.Substring(0, $zhAt).TrimEnd("`n")
	}

	if (-not $DryRun) {
		$ch = ReadU8 $changelogPath
		$eol = if ($ch.Contains("`r`n")) { "`r`n" } else { "`n" }
		# Insert before the first existing "## [" section; works with or without
		# an [Unreleased] heading (the file's header block changed over time).
		$m = [regex]::Match($ch, '(?m)^## \[')

		if (-not $m.Success) { throw 'CHANGELOG.md has no "## [" section to insert before' }

		$ch = $ch.Insert($m.Index, ($raw.Replace("`n", $eol)) + $eol + $eol)
		WriteU8 $changelogPath $ch
	}
}

Step 'build solution and single-file loader'

if (-not $DryRun) {
	$msbuild = (& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1)

	if (-not $msbuild) { throw 'MSBuild not found (vswhere returned nothing)' }

	& $msbuild (Join-Path $repo 'ZenWare.sln') /p:Configuration=Release /p:Platform=x86 /m /v:m /nologo

	if ($LASTEXITCODE -ne 0) { throw "solution build failed with exit code $LASTEXITCODE" }

	& powershell -ExecutionPolicy Bypass -NoProfile -File (Join-Path $repo 'Build-SingleFile.ps1')
}

Step 'commit, tag, push'

if (-not $DryRun) {
	git -C $repo add -A
	git -C $repo commit -m "v$new"

	if ($LASTEXITCODE -ne 0) { throw 'git commit failed' }

	git -C $repo tag -a "v$new" -m "ZenWare.cc v$new"
	git -C $repo push origin main
	git -C $repo push origin "v$new"

	if ($LASTEXITCODE -ne 0) { throw 'git push failed' }
}

Step 'github release'

if (-not $DryRun) {
	$sha = (git -C $repo rev-parse --short HEAD).Trim()
	$date = (git -C $repo log -1 --format='%ci').Trim()

	$body = ReadU8 $notesTemplate
	$body = $body.Replace('{{TAG}}', "v$new").Replace('{{DATE}}', $date).Replace('{{SHA}}', $sha)

	# The template carries the three language headings; the section text comes from
	# the notes file (English + Russian bullets) and the ZH line.
	$en = ''
	$ru = ''
	$cyr = '[\u0400-\u04FF]'
	$notesBody = if ($NotesFile) { (ReadU8 $NotesFile) } else { '' }

	foreach ($line in ($notesBody.Replace("`r`n", "`n") -split "`n")) {
		$t = $line.Trim()

		if (-not $t -or $t.StartsWith('#') -or $t.StartsWith('ZH|')) { continue }

		if ($t -match $cyr) { $ru += ($t + "`r`n") } else { $en += ($t + "`r`n") }
	}

	$body = $body.Replace('{{EN}}', $en.TrimEnd()).Replace('{{RU}}', $ru.TrimEnd()).Replace('{{ZH}}', ("- " + $zh))

	$tmp = Join-Path $env:TEMP "zenware-release-v$new.md"
	WriteU8 $tmp $body

	gh release create "v$new" --title "ZenWare.cc v$new" --notes-file $tmp --verify-tag --latest
}

Step "done: v$new"
