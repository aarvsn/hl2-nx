# Changelog

All notable changes to this fork are documented here.
Format roughly follows [Keep a Changelog](https://keepachangelog.com/).

## [1.1.0] — aarvsn fork

### Added
- **`skip_videos`** config option: bypass startup videos for a faster boot.
- **`auto_boot`** config option: skip the picker and load `gamedir` directly.
- **`vsync`** config option: uncap framerate for benchmarking.
- In-launcher **help overlay** (press Y anywhere).
- In-launcher **reset-to-defaults** (press X in Options).
- Atomic config-file writes (temp + rename) so a crash can't corrupt
  `config.txt`.
- In-file documentation comments auto-written to `config.txt` on save.
- GitHub Actions CI that builds the NRO and publishes tagged releases.
- `BUILDING.md`, `CONFIG.md`, `TROUBLESHOOTING.md`, and a much-expanded
  `README.md`.
- `scripts/build.sh` convenience wrapper (`release` / `debug` / `clean` /
  `run`).
- Footer hint on the launcher's main view so newcomers see the controls.

### Changed
- Makefile upgraded to `-O3 -flto=auto -ffast-math -fno-plt
  -fno-stack-protector`, `--gc-sections --as-needed`, parallel build by
  default.
- Picker rendering rewritten around a pre-baked **glyph atlas** — HUD text
  is now ~12 textured quads per frame instead of ~700 fill-rects.
- Box-art textures pre-loaded once with blend mode + linear scaling set
  up front, eliminating per-frame state changes.
- Clock governor: 100 ms sampling, 1.5 s window, wider hysteresis bands
  (10 / 45 instead of 5 / 12) — reacts to scene changes faster without
  thrashing the APM mode.
- `start_clock_thread` now detaches the thread instead of leaking its
  handle.
- Shared helpers (`lang_equal`, `file_contains_text`, `file_exists`,
  `strlcpy_`) consolidated into `util.c` / `util.h`.

### Fixed
- Pre-existing `-Wsign-compare` in `so_relocate` (loop counter promoted to
  `size_t`).
- Picker's "platform_misc_dir" check used the right filename (no regression
  introduced in this fork; called out because it was a footgun).

## [1.0.3] — upstream baseline

The state of the repository immediately before this fork. See the original
release notes for details.
