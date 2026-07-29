<div align="center">

# Half-Life 2 Nintendo Switch Port

A lightweight Nintendo Switch loader for the Android version of Half-Life 2.

**No game files are included. You must own a legal copy of the game.**

</div>

---

## Features

- Runs the Android ARM64 version of Half-Life 2 on Nintendo Switch.
- Supports:
  - Half-Life 2
  - Episode One
  - Episode Two
- Configurable launcher.
- Optional auto boot, video skipping, FPS counter and VSync.
- Open source.

---

## Quick Start

Download the latest `hl2_nx.nro` from **Releases**.

Place the files like this:

```text
/switch/hl2_nx/
├── hl2_nx.nro
├── config.txt
├── lib/
├── lib/episodic/
├── assets/
├── files/
├── hl2/
├── episodic/
├── ep2/
└── platform/
```

You will need:

- Android Source Engine APK libraries
- Half-Life 2 game files
- Episode One/Two libraries (optional)

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) if you're unsure which versions are supported.

---

## Launching

The game requires full application memory.

Launch it by:

- Holding **R** while opening any installed game to access Homebrew Menu, or
- Using a forwarder.

Applet (Album) mode is **not supported**.

---

## Configuration

`config.txt` is created automatically on first launch.

Example:

```text
screen_width -1
screen_height -1
gamedir hl2
lang english
show_fps 0
gamepad 1
touch_hud 0
console 1
skip_videos 0
auto_boot 0
vsync 1
```

See **CONFIG.md** for all options.

---

## Building

```bash
git clone https://github.com/aarvsn/hl2-nx.git
cd hl2-nx
make -j$(nproc)
```

Requires **devkitPro (devkitA64)**.

See **BUILDING.md** for setup instructions.

---

## Controls

### Launcher

| Button | Action |
|--------|--------|
| D-Pad / Left Stick | Navigate |
| A | Launch |
| B | Options |
| X | Reset options |
| Y | Help |
| Start | Exit |

The in-game controls follow the Android version's default controller layout.

---

## Troubleshooting

Common issues:

- Black screen → Launch using title override (not Album mode).
- Wrong version → Use the Steam `steam_legacy` branch.
- Incomplete data → Copy every `.vpk` file.
- No audio → Verify all required files were copied.

See **TROUBLESHOOTING.md** for more information.

---

## Credits

- nillerusr
- TheOfficialFloW
- fgsfds
- Masagrator
- Andy Nguyen
- aarvsn

---

## License

Licensed under the MIT License.

This project is not affiliated with Valve. No game assets are distributed with this repository.