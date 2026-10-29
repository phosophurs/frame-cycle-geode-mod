# Frame Cycle — Geometry Dash Geode mod

This repository contains the source and GitHub Actions workflow for a Windows Geometry Dash 2.2081 mod.

## Intended controls
- **P** cycles Normal, 2, 4, 6, 8, and 16 frame modes.
- **Z** shows the current mode popup.
- The default mode is 6 frames; buffering is enabled by default.
- Keybinds and popup behaviour are configurable in Geode settings.
- A button on the pause screen opens the mod settings.

## Build and download
1. Open the **Actions** tab and choose **Build Frame Cycle Geode mod**.
2. Open the latest run and wait for it to finish.
3. If it succeeds, download the **FrameCycle-Windows-geode** artifact.
4. Unzip it and copy the `.geode` file into your Geometry Dash Geode mods folder, usually `%LOCALAPPDATA%\GeometryDash\geode\mods`.

The first build may reveal source/API errors. This project has not been confirmed to compile yet; if the build fails, its logs will be used to correct it.
