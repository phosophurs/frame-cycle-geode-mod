@echo off
setlocal
where geode >nul 2>nul
if errorlevel 1 (
  echo Geode CLI was not found. Install the Geode SDK/CLI first:
  echo https://docs.geode-sdk.org/getting-started/
  pause
  exit /b 1
)
geode build
if errorlevel 1 (
  echo Build failed. Copy the full error output and send it to ChatGPT.
  pause
  exit /b 1
)
echo Build finished. Copy the generated .geode file into Geometry Dash\geode\mods.
pause
