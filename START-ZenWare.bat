@echo off
setlocal
REM ZenWare one-file launcher: starts L4D2 (-insecure, windowed) then the External overlay.
REM Just double-click this file.

set STEAM=
for /f "tokens=2*" %%a in ('reg query "HKCU\Software\Valve\Steam" /v SteamPath 2^>nul') do set STEAM=%%b
if not defined STEAM set STEAM=C:\Program Files (x86)\Steam
set STEAM=%STEAM:/=\%

if not exist "%STEAM%\steam.exe" (
  echo [ZenWare] Steam.exe not found at "%STEAM%\steam.exe"
  echo Edit the STEAM path at the top of this file.
  pause
  exit /b 1
)

echo [ZenWare] Starting Left 4 Dead 2 (-insecure, windowed)...
start "" "%STEAM%\steam.exe" -applaunch 550 -insecure -windowed -console +map c1m1_hotel

echo [ZenWare] Waiting for left4dead2.exe ...
:waitgame
tasklist /FI "IMAGENAME eq left4dead2.exe" 2>nul | find /I "left4dead2.exe" >nul
if errorlevel 1 (
  timeout /t 2 /nobreak >nul
  goto waitgame
)

echo [ZenWare] Game found, waiting 8s for map load...
timeout /t 8 /nobreak >nul

if not exist "%~dp0ZenWare.External\bin\Release\ZenWare.External.exe" (
  echo [ZenWare] ZenWare.External.exe not found next to solution. Build Release first.
  pause
  exit /b 1
)

echo [ZenWare] Starting overlay...
start "" "%~dp0ZenWare.External\bin\Release\ZenWare.External.exe"
echo [ZenWare] Done. Hotkeys: INS=ESP F8=bhop F10=strafe F9=stats END=exit.
timeout /t 5 >nul
