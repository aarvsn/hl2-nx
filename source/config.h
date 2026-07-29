/* config.h -- global configuration and config file handling
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

// memory reserved for loading the Source modules (sum of the LOAD segments of
// the 25 preloaded ARM64 .so files, with headroom for page alignment).
// everything else becomes the newlib heap, which the game allocates from
// since its malloc/free imports resolve to newlib's.
#define SO_MEMORY_MB 96

// where the game data lives on the SD card. must contain hl2/, platform/,
// lib/ (the APK's arm64-v8a libraries) and assets/extras_dir.vpk.
#define DEFAULT_INSTALL_ROOT "/switch/hl2_nx"

#define CONFIG_NAME "config.txt"
#define LOG_NAME "debug.log"

// per-line SD card writes cost boot time; set to 1 when investigating a crash
#define DEBUG_LOG 0

extern int screen_width;
extern int screen_height;

typedef struct {
  int screen_width;
  int screen_height;
  char install_root[256];
  char gamedir[64];
  char args[256];
  char lang[32];
  int show_fps;
  int gamepad;
  int touch_hud;
  int console;
  int skip_videos;   // 1 = bypass startup videos for faster boot
  int auto_boot;     // 1 = skip picker on launch, load config.gamedir directly
  int vsync;         // 1 (default) = vsync on, 0 = uncap for benchmarking
} Config;

extern Config config;

int read_config(const char *file);
int write_config(const char *file);
void config_set_defaults(void);

#endif
