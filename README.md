# Frame Cycle — Geometry Dash Geode mod

Windows mod for Geometry Dash 2.2081, built with Geode.

## Controls
- **P** cycles **Normal → 2 → 4 → 6 → 8 → 16** frame input delay.
- **Z** shows the current mode popup.
- Both keybinds can be changed in Geode settings.
- Input delay is enabled by default; **Normal** passes jump input straight to the game.
- A button on the pause screen opens this mod's settings.

## How the delay works
In a numbered mode, jump presses and releases are queued and delivered to the game after the selected number of game ticks. Press and release are both queued, so a quick tap remains a quick tap but happens later. Other buttons are not delayed.

## Build and install
1. Open the **Actions** tab and select **Build Frame Cycle Geode mod**.
2. Wait for the latest run to finish.
3. If the build succeeds, download the **FrameCycle-Windows-geode** artifact.
4. Unzip it and copy the \`.geode\` file into \`%LOCALAPPDATA%\\GeometryDash\\geode\\mods\`.

The source has been rewritten to use delayed jump events rather than holding a button down and delaying its release. Verify the in-game timing against the reference video after building.
