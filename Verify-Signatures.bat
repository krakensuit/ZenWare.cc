@echo off
setlocal
REM Verify-Signatures: one-click pattern check against YOUR local game files.
REM 1. Finds Steam install via registry.
REM 2. Builds Tools\SigScan if needed (needs VS 2022 / Build Tools).
REM 3. Runs SigScan on client.dll + engine.dll + vguimatsurface.dll.
REM Exit code 0 = all patterns OK, 2 = broken/ambiguous (update Offsets.cpp).

set ROOT=%~dp0
if "%ROOT:~-1%"=="\" set ROOT=%ROOT:~0,-1%

REM --- find MSBuild ---
set MSBUILD=
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"`) do set MSBUILD=%%i
)
if not defined MSBUILD if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" set MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
if not defined MSBUILD if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe

REM --- build SigScan if missing ---
set SIGSCAN=%ROOT%\Tools\SigScan\bin\SigScan.exe
if not exist "%SIGSCAN%" (
  echo [Verify] SigScan.exe not found, building...
  if not defined MSBUILD (
    echo [Verify] MSBuild not found. Install VS 2022 (Build Tools are enough).
    pause
    exit /b 1
  )
  "%MSBUILD%" "%ROOT%\Tools\SigScan\SigScan.vcxproj" -p:Configuration=Release -p:Platform=Win32 -v:m -nologo
  if errorlevel 1 (
    echo [Verify] SigScan build failed.
    pause
    exit /b 1
  )
)
if not exist "%SIGSCAN%" (
  echo [Verify] SigScan.exe still missing after build: %SIGSCAN%
  pause
  exit /b 1
)

REM --- find game dir ---
set STEAM=
for /f "tokens=2*" %%a in ('reg query "HKCU\Software\Valve\Steam" /v SteamPath 2^>nul') do set STEAM=%%b
if not defined STEAM set STEAM=C:\Program Files (x86)\Steam
set STEAM=%STEAM:/=\%
set BINDIR=%STEAM%\steamapps\common\Left 4 Dead 2\left4dead2\bin
if not exist "%BINDIR%\client.dll" (
  echo [Verify] client.dll not found at "%BINDIR%\client.dll"
  echo Edit STEAM path at the top of this file if Steam lives elsewhere.
  pause
  exit /b 1
)

echo [Verify] Checking patterns against:
echo [Verify]   %BINDIR%\client.dll
"%SIGSCAN%" "%BINDIR%\client.dll" "%BINDIR%\engine.dll" "%BINDIR%\vguimatsurface.dll"
set RC=%ERRORLEVEL%
echo.
if %RC%==0 (
  echo [Verify] ALL PATTERNS OK - Offsets.cpp matches your game build.
) else (
  echo [Verify] BROKEN PATTERNS - update Offsets.cpp, see lines above.
  echo [Verify] Tip: broken pattern after a game update = new signature needed.
)
pause
exit /b %RC%
