# Building hl2-nx from source

You need a working [devkitPro](https://devkitpro.org/) toolchain with the
Switch (devkitA64) packages. The build has been tested on Linux, macOS, and
WSL2.

## 1. Install devkitPro

### Linux / WSL2

Follow the [devkitPro wiki](https://devkitpro.org/wiki/Getting_Started)
(`pacman`-based install on Debian/Ubuntu, Arch, Fedora, etc.).

Minimum required pacman packages:

```bash
sudo dkp-pacman -S \
  devkitA64 \
  libnx \
  switch-tools \
  switch-sdl2 \
  switch-sdl2_image \
  switch-mesa \
  switch-libdrm_nouveau \
  switch-zlib \
  switch-ffmpeg \
  switch-bz2 \
  switch-dav1d
```

After install, set the env var:

```bash
echo 'export DEVKITPRO=/opt/devkitpro' >> ~/.bashrc
echo 'export DEVKITARM=$DEVKITPRO/devkitA64' >> ~/.bashrc
echo 'export PATH=$DEVKITPRO/tools/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

### macOS

Use the devkitPro [Homebrew tap](https://github.com/devkitPro/pacman/releases)
(the `.pkg` installer is the easiest). Set the same env vars as above but
with `/opt/devkitpro` adjusted if needed.

### Windows

Use WSL2 (Ubuntu). Native Windows builds are not supported.

## 2. Clone & build

```bash
git clone https://github.com/aarvsn/hl2-nx.git
cd hl2-nx
make -j$(nproc)
```

A successful build produces:

```
hl2_nx.nro      <- copy this to /switch/hl2_nx/ on the SD card
hl2_nx.elf      <- debug symbols (not needed for normal use)
hl2_nx.nacp     <- metadata
```

## 3. Useful make targets

| Target | Action |
|---|---|
| `make` | Build the NRO |
| `make -j$(nproc)` | Parallel build (the default in this fork) |
| `make clean` | Remove `build/` and all NRO/ELF/NACP artefacts |
| `make run` | Build + print the path you should copy the NRO to |
| `make size` | Build + show the final NRO size |

## 4. Debug build

`DEBUG_LOG` lives in [`source/config.h`](source/config.h). Flip it to `1`
to enable nxlink + `debug.log` writes. **Slow** — don't ship this.

## 5. CI build

Every push and pull request is built by the GitHub Actions workflow in
[`.github/workflows/build.yml`](.github/workflows/build.yml). Tagged releases
push a `hl2_nx.nro` to the GitHub Releases page automatically.

```bash
git tag v1.1.0
git push origin v1.1.0
# -> release appears within ~3 minutes
```

## 6. Build flags reference

This fork's `Makefile` adds the following optimisation flags on top of the
original `-O2`:

| Flag | Why |
|---|---|
| `-O3` | Aggressive inlining + loop transforms |
| `-ffast-math -funsafe-math-optimizations` | The Source engine already assumes IEEE-leans; safe and faster |
| `-flto=auto` | Link-time optimisation across all TUs (smaller + faster) |
| `-fno-plt` | Indirect calls via GOT directly, no PLT hop |
| `-fno-stack-protector` | Source modules don't honour it anyway; saves a TLS load per function |
| `-ffunction-sections -fdata-sections` | Pair with `--gc-sections` to drop unused code |
| `-Wl,--gc-sections -Wl,--as-needed -Wl,-O1` | Drop unreferenced functions/data (huge win on a shim this small) |

If you need to debug a crash and `-O3` is hiding the bug, override on the
command line:

```bash
make CFLAGS="-O0 -g -Wall -fPIE -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -D__SWITCH__"
```

## 7. Reproducible builds

The build is reproducible given the same devkitPro package versions. To pin
them, capture the output of:

```bash
dkp-pacman -Q | grep -E '^(devkitA64|libnx|switch-)' > deps.lock
```

…and reinstall from `deps.lock` on the next machine:

```bash
sudo dkp-pacman -S --needed $(awk '{print $1}' deps.lock)
```
