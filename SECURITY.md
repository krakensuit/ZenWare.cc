# Security / Distribution policy

## Source-only

This repository distributes **source code only**. There are no prebuilt binaries,
no `.exe` / `.dll` assets in Releases and no CI artifacts with binaries.

- Do not upload built `exe` / `dll` to public GitHub Releases.
- Local builds stay on your machine (`dist/` is git-ignored).
- Share built files only privately, never via public Releases.

## Intended use

- Education and local servers only, game launched with `-insecure`.
- Do not use on VAC-secured servers. Bans are your own responsibility.
- No support for bypassing VAC or any anti-cheat. Such issues will be closed.

## Reporting

- Bugs in offsets after a game update: run `Tools/SigScan` on your local
  `left4dead2/bin/client.dll` and report `module+offset` with the log line `[!!!] EXCEPTION`.
- Do not attach game binaries, dumps, or personal data.
- Do not report your own game bans as security issues.
