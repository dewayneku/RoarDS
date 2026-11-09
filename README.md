<p align="center"><img src="res/icon/roards_128x128.png"></p>
<h2 align="center"><b>RoarDS</b></h2>
<p align="center">
<a href="https://www.gnu.org/licenses/gpl-3.0" alt="License: GPLv3"><img src="https://img.shields.io/badge/License-GPL%20v3-%23ff554d.svg"></a>
</p>

RoarDS is a Nintendo DS and DSi emulator. It's a fork of [melonDS](https://github.com/melonDS-emu/melonDS)
by Arisotura and the melonDS team, focused on quality-of-life improvements that users have asked for.

**What's different from melonDS so far**

* Save/load state, undo state load, Open ROM and Quit are remappable hotkeys, and can be bound to a
  keyboard key or a controller button (*Config → Input and hotkeys → Save states & menu*).
* **Overlap** screen layout: the main screen fills the window and the other screen is shown small in
  its corner, handy with widescreen patches. *Swap screens* picks which screen is the main one.
* Custom screen gap of any size from 0 to 500 px (*View → Screen gap → Custom...*), applied instantly.
* Hotkeys to cycle the screen layout and screen sizing (*Input and hotkeys → General hotkeys*).
* Hotkeys respond to very quick taps, even with no game running.
* **Controller profiles**: save named sets of joystick mappings and switch between them
  (*Input and hotkeys → Controller profile*). A profile can be linked to a type of controller so it
  loads automatically whenever that controller is connected.

**Coming from melonDS?** On first run, RoarDS copies your settings from `melonDS.toml` into its own
`RoarDS.toml`. Your melonDS install and settings are never changed.

RoarDS is not affiliated with or endorsed by the melonDS team or Nintendo. Please report RoarDS bugs
[here](https://github.com/dewayneku/RoarDS/issues), not to melonDS.

The rest of this README comes from melonDS and still applies to RoarDS.

<hr>

## How to use

Firmware boot (not direct boot) requires a BIOS/firmware dump from an original DS or DS Lite.
DS firmwares dumped from a DSi or 3DS aren't bootable and only contain configuration data, thus they are only suitable when booting games directly.

### Possible firmware sizes

 * 128KB: DSi/3DS DS-mode firmware (reduced size due to lacking bootcode)
 * 256KB: regular DS firmware
 * 512KB: iQue DS firmware

DS BIOS dumps from a DSi or 3DS can be used with no compatibility issues. DSi BIOS dumps (in DSi mode) are not compatible. Or maybe they are. I don't know.

As for the rest, the interface should be pretty straightforward. If you have a question, don't hesitate to ask, though!

## How to build
See [BUILD.md](./BUILD.md) for build instructions.

## TODO LIST

 * better DSi emulation
 * better OpenGL rendering
 * netplay
 * the impossible quest of pixel-perfect 3D graphics
 * support for rendering screens to separate windows
 * emulating some fancy addons
 * other non-core shit (debugger, graphics viewers, etc)

### TODO LIST FOR LATER (low priority)

 * big-endian compatibility (Wii, etc)
 * LCD refresh time (used by some games for blending effects)
 * any feature you can eventually ask for that isn't outright stupid

## Credits

 * Martin for GBAtek, a good piece of documentation
 * Cydrak for the extra 3D GPU research
 * limittox for the icon
 * All of you comrades who have been testing melonDS, reporting issues, suggesting shit, etc

## Licenses

[![GNU GPLv3 Image](https://www.gnu.org/graphics/gplv3-127x51.png)](http://www.gnu.org/licenses/gpl-3.0.en.html)

melonDS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

### External
* Images used in the Input Config Dialog - see `src/frontend/qt_sdl/InputConfig/resources/LICENSE.md`
