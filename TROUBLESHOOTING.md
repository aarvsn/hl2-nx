# Troubleshooting

## Black screen on launch

**Cause:** You're in applet mode (launched from the album icon without
holding R). Applet mode only gives the app a tiny memory pool and the Source
engine can't fit.

**Fix:** Use a title override. Hold **R** on any installed title (a real
game or a homebrew forwarder) and open the Homebrew Menu from there.

---

## "svcMapProcessCodeMemory is unavailable"

Same cause as above. Applet mode strips the syscalls this port needs.

---

## "PORT FILES MISSING"

The launcher can't find the Android Source engine libraries on your SD card.

**Fix:** Make sure these paths all exist under `/switch/hl2_nx/`:

```
lib/liblauncher.so
lib/libengine.so
lib/libclient.so
lib/libserver.so
lib/libtogl.so
assets/extras_dir.vpk
files/dejavusans.ttf
```

The `lib/` directory must contain **all 27** `.so` files from the APK's
`lib/arm64-v8a/`.

---

## "WRONG VERSION" / "USE STEAM_LEGACY OR ANDROID"

The 20th-anniversary Steam update changed the VPK format and broke this port.

**Fix:**

1. In Steam, right-click **Half-Life 2 → Properties → Betas**.
2. Pick **`steam_legacy`** from the drop-down.
3. Wait for Steam to download the older build.
4. Re-copy the `hl2/` and `platform/` directories to your SD card.

You can verify the patch version is correct by opening `hl2/steam.inf` —
`PatchVersion=` must be below `9352380`.

---

## "INCOMPLETE DATA" / "COPY ALL VPK PARTS"

VPK archives are split into `_000.vpk`, `_001.vpk`, … plus a `_dir.vpk`
index. The launcher checks every part's size against the index. If any part
is missing or truncated, the install is rejected.

**Fix:** Copy **every** `*_NNN.vpk` file from your game data — don't skip
the multi-GB ones.

---

## "20TH ANNIV" warning

You're using the 20th-anniversary data. The game will launch but you'll
likely hit missing-texture or assertion issues inside the engine. Switch to
`steam_legacy` (see above).

---

## No audio in the game

Possible causes, in order:

1. **`skip_videos` is on** — turn it off; the audio service is set up during
   video playback.
2. **A font is missing in `files/`** — the engine needs all five
   (`dejavusans.ttf`, `dejavusans-bold.ttf`, `DroidSansFallback.ttf`,
   `LiberationMono-Regular.ttf`, `Itim-Regular.otf`).
3. **`config.lang` doesn't match your data** — if you set `lang russian` but
   don't have `hl2/resource/gameui_russian.txt`, the engine may silently
   fail. The launcher filters the language list to what's installed, but a
   hand-edited `config.txt` can still break it.

---

## Game crashes on load

1. Enable `console 1` and `args +developer 1` to surface the error.
2. Flip `DEBUG_LOG 1` in `source/config.h`, rebuild, and read `debug.log`
   from the SD card.
3. If you see `unresolved import: <symbol>`, you have a library version
   mismatch — re-extract the APK.

---

## Performance is poor

* Docked mode is required for full clocks. Handheld caps the CPU at ~1 GHz.
* Try lowering `screen_width` / `screen_height` (e.g. `1280 720` docked).
* Set `skip_videos 1` if you don't care about the intro.
* The clock governor in this fork is more responsive than upstream — if
  you're still seeing hitches, your SD card's random-read perf may be the
  bottleneck (the engine streams VPK chunks aggressively). A UHS-I V30 or
  better card is strongly recommended.

---

## Launcher is unresponsive

The launcher uses SDL2's game controller API. If your Joy-Con isn't paired:

* Open **System Settings → Controllers and Sensors → Pair Controllers**.
* Or use the touch screen (the launcher supports `SDL_FINGERDOWN`).
* If the launcher was started in applet mode, restart with a title override.

---

## "EP1 REQUIRED" / "EP2 REQUIRED"

Episode One must be playable before Episode Two will launch — they share
the episodic libraries and the engine expects both installed.

---

## The picker says "20TH ANNIV" but I want to play

The 20th-anniversary VPKs *will* load, but you'll likely see missing
textures or hit assertion failures mid-level. Use the `steam_legacy` branch
(see above) and the warning will go away.

---

## Still stuck?

* Search the [Issues](../../issues) page — your problem may already be filed.
* If you open a new issue, attach:
  * Your `config.txt`
  * The output of `stat /switch/hl2_nx/lib/liblauncher.so` (so we can verify
    the APK version)
  * The launcher's status text (e.g. `WRONG VERSION`, `INCOMPLETE DATA`)
