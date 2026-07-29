# Configuration reference

A `config.txt` is created next to the NRO on first launch. The launcher can
also edit most of these options directly (Press **B** → **Options**).

## Options

| Key | Type | Default | Description |
|---|---|---|---|
| `screen_width` | int | `-1` | Render width. `-1` = auto by dock state (1920 docked / 1280 handheld). |
| `screen_height` | int | `-1` | Render height. `-1` = auto. |
| `gamedir` | string | `hl2` | Game to launch: `hl2`, `episodic`, or `ep2`. |
| `args` | string | (empty) | Extra Source command-line args (e.g. `+sv_cheats 1`). |
| `lang` | string | `english` | Source language name. Aliases like `en_US` are normalised on load. |
| `show_fps` | int (0/1) | `0` | Show the in-game FPS counter. |
| `gamepad` | int (0/1) | `1` | Enable gamepad input. |
| `touch_hud` | int (0/1) | `0` | Show the on-screen touch HUD (works with Joy-Con too). |
| `console` | int (0/1) | `1` | Enable the developer console (`~` key equivalent). |
| `skip_videos` | int (0/1) | `0` | **New.** Bypass startup videos (Valve logo, Bink intros). Faster boot. |
| `auto_boot` | int (0/1) | `0` | **New.** Skip the picker and load `gamedir` directly on launch. |
| `vsync` | int (0/1) | `1` | **New.** `0` disables vsync (uncapped framerate — useful for benchmarking). |

## Examples

### Faster boot, no picker

```
gamedir hl2
skip_videos 1
auto_boot 1
```

### Handheld-only, lower internal resolution

```
screen_width 960
screen_height 540
```

### Benchmark mode

```
vsync 0
show_fps 1
args +fps_max 0
```

## File format

* Whitespace-separated `key value` pairs, one per line.
* `#` starts a comment.
* String values may not contain spaces (use `_` or `-`).
* The file is rewritten on every Options change with an atomic temp-file +
  rename, so a crash mid-write can't corrupt it.

## Reset to defaults

In the launcher: open **Options** (press **B**) and press **X**. Your game
selection and language are preserved; everything else resets.

Manually: delete `config.txt`. It will be recreated with defaults on the next
launch.
