#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "config.h"
#include "util.h"
#include "picker.h"

#define NUM_GAMES 3
#define BASE_W 1920
#define BASE_H 1080
#define IMG_W 450
#define IMG_H 675
#define PULSE_PERIOD 2.0f
#define PULSE_MAX_SCALE 1.12f
#define STICK_THRESHOLD 16000
#define STICK_DEADZONE 8000
#define OPTIONS_INDEX NUM_GAMES
#define NUM_OPTION_ROWS 8
#define MAX_LANG_OPTIONS 32
#define LANG_VISIBLE_ROWS 9

// Glyph atlas: instead of poking ~35 pixels per character per frame through
// SDL_RenderFillRect (which costs one GL draw call each), bake every glyph
// once into a single 5x7 monochrome texture and let the renderer blit it.
// A 16-wide atlas covers the printable set we use, so the whole HUD text
// path becomes ~12 textured quads per frame instead of ~700 fill-rects.
#define GLYPH_W 5
#define GLYPH_H 7
#define ATLAS_COLS 16

typedef enum {
  PICKER_MAIN,
  PICKER_OPTIONS,
  PICKER_LANGUAGE,
  PICKER_HELP
} PickerView;

typedef struct {
  int playable;
  const char *status;
  const char *detail;
} GameInstall;

typedef struct {
  const char *label;
  const char *value;
} LanguageOption;

typedef struct {
  char c;
  unsigned char rows[7];
} Glyph;

static const char *const game_dirs[NUM_GAMES] = { "hl2", "episodic", "ep2" };
static const char *const image_paths[NUM_GAMES] = {
  "romfs:/hl2.jpg",
  "romfs:/hl2ep1.jpg",
  "romfs:/hl2ep2.jpg",
};

static const LanguageOption language_options[] = {
  { "ENGLISH", "english" },
  { "GERMAN", "german" },
  { "FRENCH", "french" },
  { "ITALIAN", "italian" },
  { "KOREAN", "koreana" },
  { "SPANISH", "spanish" },
  { "S CHINESE", "schinese" },
  { "T CHINESE", "tchinese" },
  { "RUSSIAN", "russian" },
  { "THAI", "thai" },
  { "JAPANESE", "japanese" },
  { "PORTUGUESE", "portuguese" },
  { "POLISH", "polish" },
  { "DANISH", "danish" },
  { "DUTCH", "dutch" },
  { "FINNISH", "finnish" },
  { "NORWEGIAN", "norwegian" },
  { "SWEDISH", "swedish" },
  { "ROMANIAN", "romanian" },
  { "TURKISH", "turkish" },
  { "HUNGARIAN", "hungarian" },
  { "CZECH", "czech" },
  { "BRAZILIAN", "brazilian" },
  { "BULGARIAN", "bulgarian" },
  { "GREEK", "greek" },
  { "UKRAINIAN", "ukrainian" },
};

static const Glyph glyphs[] = {
  { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
  { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
  { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
  { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
  { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
  { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
  { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
  { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
  { 'I', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F } },
  { 'J', { 0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E } },
  { 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
  { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
  { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
  { 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
  { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
  { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
  { 'Q', { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } },
  { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
  { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
  { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
  { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
  { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
  { 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 } },
  { 'X', { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
  { 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
  { 'Z', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } },
  { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
  { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
  { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
  { '3', { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E } },
  { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
  { '5', { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E } },
  { '6', { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
  { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
  { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
  { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E } },
  { '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } },
  { '_', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F } },
  { '/', { 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10 } },
  { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
  { ':', { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 } },
  { '+', { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 } },
  { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { '!', { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 } },
  { '(', { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 } },
  { ')', { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 } },
  { '*', { 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00 } },
};

// ---- glyph atlas state ---------------------------------------------------
typedef struct {
  SDL_Texture *tex;
  // For each char in the printable set used by the UI, store its index
  // inside the atlas (column), or -1 if not present (renders as space).
  int8_t index[128];
} GlyphAtlas;

static int glyph_index(char c) {
  for (unsigned i = 0; i < sizeof(glyphs) / sizeof(*glyphs); i++) {
    if (glyphs[i].c == c)
      return (int)i;
  }
  return -1;
}

static void atlas_build(GlyphAtlas *a, SDL_Renderer *ren) {
  // Single 80x112 RGBA texture holding up to 16 glyphs per row, in the order
  // of the glyphs[] table.  White on transparent so the renderer can tint
  // via SDL_SetTextureColorMod on the fly.
  const int rows_total = (sizeof(glyphs) / sizeof(*glyphs) + ATLAS_COLS - 1) / ATLAS_COLS;
  const int atlas_w = ATLAS_COLS * GLYPH_W;
  const int atlas_h = rows_total * GLYPH_H;

  SDL_Surface *s = SDL_CreateRGBSurface(0, atlas_w, atlas_h, 32,
                                        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
  if (!s) { a->tex = NULL; return; }
  SDL_FillRect(s, NULL, 0);

  Uint32 white = SDL_MapRGBA(s->format, 255, 255, 255, 255);
  for (unsigned i = 0; i < sizeof(glyphs) / sizeof(*glyphs); i++) {
    int col = i % ATLAS_COLS;
    int row = i / ATLAS_COLS;
    for (int y = 0; y < GLYPH_H; y++) {
      for (int x = 0; x < GLYPH_W; x++) {
        if (glyphs[i].rows[y] & (1 << (4 - x))) {
          Uint32 *p = (Uint32 *)((Uint8 *)s->pixels +
                       (row * GLYPH_H + y) * s->pitch +
                       (col * GLYPH_W + x) * 4);
          *p = white;
        }
      }
    }
  }

  a->tex = SDL_CreateTextureFromSurface(ren, s);
  SDL_FreeSurface(s);

  for (int i = 0; i < 128; i++) a->index[i] = -1;
  for (unsigned i = 0; i < sizeof(glyphs) / sizeof(*glyphs); i++) {
    char c = glyphs[i].c;
    if ((unsigned char)c < 128)
      a->index[(unsigned char)c] = (int8_t)i;
  }
}

static void atlas_free(GlyphAtlas *a) {
  if (a->tex) SDL_DestroyTexture(a->tex);
  a->tex = NULL;
}

// ---- install scan helpers (mostly unchanged, deduplicated) ---------------

static int language_index_for_value(const char *value) {
  for (unsigned i = 0; i < sizeof(language_options) / sizeof(*language_options); i++) {
    if (lang_equal(value, language_options[i].value))
      return (int)i;
  }
  return 0;
}

static int language_file_exists(const char *dir, const char *stem, const char *lang) {
  char path[256];
  snprintf(path, sizeof(path), "%s/resource/%s_%s.txt", dir, stem, lang);
  return file_exists(path);
}

static int language_available(const char *lang) {
  if (!strcmp(lang, "english"))
    return 1;

  return language_file_exists("hl2", "valve", lang) ||
         language_file_exists("hl2", "gameui", lang) ||
         language_file_exists("hl2", "replay", lang) ||
         language_file_exists("hl2", "closecaption", lang) ||
         language_file_exists("episodic", "episodic", lang) ||
         language_file_exists("episodic", "gameui", lang) ||
         language_file_exists("episodic", "closecaption", lang) ||
         language_file_exists("ep2", "ep2", lang) ||
         language_file_exists("ep2", "gameui", lang) ||
         language_file_exists("ep2", "closecaption", lang) ||
         language_file_exists("platform", "gameui", lang) ||
         language_file_exists("platform", "valve", lang) ||
         language_file_exists("platform", "vgui", lang);
}

static void build_language_list(int *langs, int *count) {
  int current = language_index_for_value(config.lang);
  *count = 0;

  for (unsigned i = 0; i < sizeof(language_options) / sizeof(*language_options); i++) {
    if ((int)i == current || language_available(language_options[i].value)) {
      if (*count < MAX_LANG_OPTIONS)
        langs[(*count)++] = (int)i;
    }
  }
}

static int selected_language_row(const int *langs, int count) {
  int current = language_index_for_value(config.lang);
  for (int i = 0; i < count; i++) {
    if (langs[i] == current)
      return i;
  }
  return 0;
}

static int has_episodic_libs(void) {
  return file_exists("lib/episodic/libclient.so") &&
         file_contains_text("lib/episodic/libserver.so", "sk_zombie_soldier_health");
}

static long file_size(const char *path) {
  struct stat st;
  if (stat(path, &st) < 0 || !S_ISREG(st.st_mode))
    return -1;
  return (long)st.st_size;
}

static int has_size(const char *path, long expected) {
  long size = file_size(path);
  return size == expected;
}

static uint16_t read_le16(const unsigned char *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_le32(const unsigned char *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static const unsigned char *skip_vpk_string(const unsigned char *p,
                                            const unsigned char *end,
                                            int *empty) {
  const unsigned char *start = p;
  while (p < end && *p)
    p++;
  if (p >= end)
    return NULL;
  *empty = p == start;
  return p + 1;
}

static int vpk_archives_complete(const char *dir_path) {
  unsigned char header[28];
  unsigned char *tree = NULL;
  FILE *f = fopen(dir_path, "rb");
  int complete = 0;
  uint16_t max_archive = 0;
  int saw_archive = 0;
  uint32_t required_size[1025] = { 0 };

  if (!f)
    return 0;

  if (fread(header, 1, sizeof(header), f) < 12)
    goto out;

  uint32_t signature = read_le32(header);
  uint32_t version = read_le32(header + 4);
  uint32_t tree_size = read_le32(header + 8);
  long tree_offset = version == 1 ? 12 : 28;

  if (signature != 0x55aa1234 || (version != 1 && version != 2) ||
      tree_size == 0 || tree_size > 8 * 1024 * 1024)
    goto out;

  tree = malloc(tree_size);
  if (!tree)
    goto out;

  if (fseek(f, tree_offset, SEEK_SET) != 0 ||
      fread(tree, 1, tree_size, f) != tree_size)
    goto out;

  const unsigned char *p = tree;
  const unsigned char *end = tree + tree_size;

  for (;;) {
    int empty = 0;
    p = skip_vpk_string(p, end, &empty);
    if (!p)
      goto out;
    if (empty)
      break;

    for (;;) {
      p = skip_vpk_string(p, end, &empty);
      if (!p)
        goto out;
      if (empty)
        break;

      for (;;) {
        p = skip_vpk_string(p, end, &empty);
        if (!p)
          goto out;
        if (empty)
          break;

        if (end - p < 18)
          goto out;

        p += 4;
        uint16_t preload = read_le16(p);
        p += 2;
        uint16_t archive = read_le16(p);
        p += 2;
        uint32_t entry_offset = read_le32(p);
        p += 4;
        uint32_t entry_length = read_le32(p);
        p += 4;
        uint16_t terminator = read_le16(p);
        p += 2;

        if (terminator != 0xffff || end - p < preload)
          goto out;

        if (archive != 0x7fff) {
          if (archive > 1024)
            goto out;
          if (!saw_archive || archive > max_archive)
            max_archive = archive;
          saw_archive = 1;

          if (entry_offset > UINT32_MAX - entry_length)
            goto out;
          uint32_t entry_end = entry_offset + entry_length;
          if (entry_end > required_size[archive])
            required_size[archive] = entry_end;
        }

        p += preload;
      }
    }
  }

  complete = 1;
  if (saw_archive) {
    char prefix[256];
    char archive_path[272];
    snprintf(prefix, sizeof(prefix), "%s", dir_path);
    char *suffix = strstr(prefix, "_dir.vpk");
    if (!suffix)
      goto out;
    *suffix = '\0';

    for (uint16_t i = 0; i <= max_archive; i++) {
      snprintf(archive_path, sizeof(archive_path), "%s_%03u.vpk", prefix, i);
      if (file_size(archive_path) < (long)required_size[i]) {
        complete = 0;
        break;
      }
    }
  }

out:
  free(tree);
  fclose(f);
  return complete;
}

static int missing_any(const char *const *paths, int count) {
  for (int i = 0; i < count; i++) {
    if (!file_exists(paths[i]))
      return 1;
  }
  return 0;
}

static int read_patch_version(void) {
  char line[128];
  FILE *f = fopen("hl2/steam.inf", "r");
  if (!f)
    return -1;

  while (fgets(line, sizeof(line), f)) {
    int version = -1;
    if (sscanf(line, "PatchVersion=%d", &version) == 1) {
      fclose(f);
      return version;
    }
  }

  fclose(f);
  return -1;
}

static void scan_installs(GameInstall install[NUM_GAMES]) {
  const long hl2_pak_dir_bad_size = 7514;
  const long hl2_textures_dir_bad_size = 224277;
  const long hl2_sound_misc_dir_bad_size = 127659;
  const long hl2_misc_dir_bad_size = 718054;
  const long platform_misc_dir_bad_size = 15065;
  const long ep1_pak_dir_anniversary_size = 237254;
  const long ep2_pak_dir_anniversary_size = 474399;
  static const char *const port_files[] = {
    "lib/liblauncher.so",
    "lib/libengine.so",
    "lib/libclient.so",
    "lib/libserver.so",
    "lib/libtogl.so",
    "assets/extras_dir.vpk",
    "files/dejavusans.ttf",
  };
  static const char *const hl2_files[] = {
    "hl2/gameinfo.txt",
    "hl2/steam.inf",
    "hl2/hl2_pak_dir.vpk",
    "hl2/hl2_textures_dir.vpk",
    "hl2/hl2_sound_misc_dir.vpk",
    "hl2/hl2_misc_dir.vpk",
    "platform/platform_misc_dir.vpk",
  };
  static const char *const ep1_files[] = {
    "episodic/gameinfo.txt",
    "episodic/ep1_pak_dir.vpk",
  };
  static const char *const ep2_files[] = {
    "ep2/gameinfo.txt",
    "ep2/ep2_pak_dir.vpk",
  };
  int episodic_libs = 0;

  for (int i = 0; i < NUM_GAMES; i++) {
    install[i].playable = 0;
    install[i].status = "NOT INSTALLED";
    install[i].detail = "MISSING DATA";
  }

  if (missing_any(port_files, sizeof(port_files) / sizeof(*port_files))) {
    for (int i = 0; i < NUM_GAMES; i++) {
      install[i].status = "PORT FILES MISSING";
      install[i].detail = "CHECK LIB ASSETS FILES";
    }
    return;
  }

  if (missing_any(hl2_files, sizeof(hl2_files) / sizeof(*hl2_files))) {
    install[0].detail = "COPY HL2 DATA";
    install[1].status = "HL2 REQUIRED";
    install[1].detail = "COPY HL2 DATA FIRST";
    install[2].status = "HL2 REQUIRED";
    install[2].detail = "COPY HL2 DATA FIRST";
    return;
  }

  if (read_patch_version() >= 9352380 ||
      has_size("hl2/hl2_pak_dir.vpk", hl2_pak_dir_bad_size) ||
      has_size("hl2/hl2_textures_dir.vpk", hl2_textures_dir_bad_size) ||
      has_size("hl2/hl2_sound_misc_dir.vpk", hl2_sound_misc_dir_bad_size) ||
      has_size("hl2/hl2_misc_dir.vpk", hl2_misc_dir_bad_size) ||
      has_size("platform/platform_misc_dir.vpk", platform_misc_dir_bad_size)) {
    install[0].status = "WRONG VERSION";
    install[0].detail = "USE STEAM_LEGACY OR ANDROID";
    install[1].status = "HL2 WRONG VERSION";
    install[1].detail = "FIX HL2 DATA FIRST";
    install[2].status = "HL2 WRONG VERSION";
    install[2].detail = "FIX HL2 DATA FIRST";
    return;
  }

  if (!vpk_archives_complete("hl2/hl2_pak_dir.vpk") ||
      !vpk_archives_complete("hl2/hl2_textures_dir.vpk") ||
      !vpk_archives_complete("hl2/hl2_sound_misc_dir.vpk") ||
      !vpk_archives_complete("hl2/hl2_misc_dir.vpk") ||
      !vpk_archives_complete("platform/platform_misc_dir.vpk")) {
    install[0].status = "INCOMPLETE DATA";
    install[0].detail = "COPY ALL VPK PARTS";
    install[1].status = "HL2 INCOMPLETE";
    install[1].detail = "FIX HL2 DATA FIRST";
    install[2].status = "HL2 INCOMPLETE";
    install[2].detail = "FIX HL2 DATA FIRST";
    return;
  }

  install[0].playable = 1;
  install[0].status = "";
  install[0].detail = "";
  episodic_libs = has_episodic_libs();

  if (!missing_any(ep1_files, sizeof(ep1_files) / sizeof(*ep1_files))) {
    int anniversary = has_size("episodic/ep1_pak_dir.vpk", ep1_pak_dir_anniversary_size);
    if (!episodic_libs) {
      install[1].status = "PORT FILES WRONG";
      install[1].detail = "NEED EPISODIC DLLS";
    } else if (!vpk_archives_complete("episodic/ep1_pak_dir.vpk")) {
      install[1].status = "INCOMPLETE DATA";
      install[1].detail = "COPY ALL EP1 VPK PARTS";
    } else {
      install[1].playable = 1;
      install[1].status = anniversary ? "20TH ANNIV" : "";
      install[1].detail = anniversary ? "WARNING 20TH ANNIVERSARY DATA" : "";
    }
  } else {
    install[1].detail = "COPY EPISODIC DATA";
  }

  if (!missing_any(ep2_files, sizeof(ep2_files) / sizeof(*ep2_files))) {
    int anniversary = has_size("ep2/ep2_pak_dir.vpk", ep2_pak_dir_anniversary_size);
    if (!episodic_libs) {
      install[2].status = "PORT FILES WRONG";
      install[2].detail = "NEED EPISODIC DLLS";
    } else if (!vpk_archives_complete("ep2/ep2_pak_dir.vpk")) {
      install[2].status = "INCOMPLETE DATA";
      install[2].detail = "COPY ALL EP2 VPK PARTS";
    } else if (!install[1].playable) {
      install[2].status = "EP1 REQUIRED";
      install[2].detail = "FIX EPISODIC DATA";
    } else {
      install[2].playable = 1;
      install[2].status = anniversary ? "20TH ANNIV" : "";
      install[2].detail = anniversary ? "WARNING 20TH ANNIVERSARY DATA" : "";
    }
  } else {
    install[2].detail = "COPY EP2 DATA";
  }
}

// Lightweight check used by the auto-boot path.  Runs scan_installs and
// returns 1 if the configured game is playable, -1 if it isn't, 0 if we
// can't tell (and should defer to the picker UI).
int picker_quick_check(void) {
  GameInstall install[NUM_GAMES];
  scan_installs(install);
  for (int i = 0; i < NUM_GAMES; i++) {
    if (!strcmp(config.gamedir, game_dirs[i]))
      return install[i].playable ? 1 : -1;
  }
  return 0;
}

static int selected_from_config(void) {
  for (int i = 0; i < NUM_GAMES; i++) {
    if (!strcmp(config.gamedir, game_dirs[i]))
      return i;
  }
  return 0;
}

static int move_selection(int selected, int dir) {
  if (dir > 0) {
    if (selected == OPTIONS_INDEX)
      return 0;
    if (selected == NUM_GAMES - 1)
      return OPTIONS_INDEX;
    return selected + 1;
  }

  if (selected == OPTIONS_INDEX)
    return NUM_GAMES - 1;
  if (selected == 0)
    return NUM_GAMES - 1;
  return selected - 1;
}

// ---- layout (pure helpers, easy for the compiler to inline) -------------

static float ui_scale(int w, int h) {
  float sx = (float)w / (float)BASE_W;
  float sy = (float)h / (float)BASE_H;
  return sx < sy ? sx : sy;
}

static SDL_Rect base_rect(int i, int w, int h) {
  float s = ui_scale(w, h);
  int iw = (int)((float)IMG_W * s + 0.5f);
  int ih = (int)((float)IMG_H * s + 0.5f);
  int gap = (w - NUM_GAMES * iw) / (NUM_GAMES + 1);
  SDL_Rect r = { gap + i * (iw + gap), (h - ih) / 2, iw, ih };
  return r;
}

static void draw_border(SDL_Renderer *ren, const SDL_Rect *r, int w, int h,
                        unsigned char cr, unsigned char cg, unsigned char cb) {
  int thick = (int)(4.0f * ui_scale(w, h) + 0.5f);
  if (thick < 2) thick = 2;

  SDL_SetRenderDrawColor(ren, cr, cg, cb, 230);
  for (int t = 0; t < thick; t++) {
    SDL_Rect b = {
      r->x - thick + t,
      r->y - thick + t,
      r->w + (thick - t) * 2,
      r->h + (thick - t) * 2
    };
    SDL_RenderDrawRect(ren, &b);
  }
}

// ---- text drawing via glyph atlas ----------------------------------------

static int text_width(const char *text, int scale) {
  int len = (int)strlen(text);
  return len > 0 ? (len * 6 - 1) * scale : 0;
}

static void draw_text(SDL_Renderer *ren, const GlyphAtlas *atlas,
                      int x, int y, int scale, const char *text,
                      unsigned char cr, unsigned char cg, unsigned char cb, unsigned char ca) {
  if (!atlas || !atlas->tex) return;
  SDL_SetTextureColorMod(atlas->tex, cr, cg, cb);
  SDL_SetTextureAlphaMod(atlas->tex, ca);

  for (const char *p = text; *p; p++, x += 6 * scale) {
    int idx = -1;
    unsigned char uc = (unsigned char)*p;
    if (uc < 128) idx = atlas->index[uc];
    if (idx < 0) {
      // fall back to space for unknown chars (e.g. punctuation outside the set)
      idx = atlas->index[' '];
      if (idx < 0) continue;
    }
    int col = idx % ATLAS_COLS;
    int row = idx / ATLAS_COLS;
    SDL_Rect src = { col * GLYPH_W, row * GLYPH_H, GLYPH_W, GLYPH_H };
    SDL_Rect dst = { x, y, GLYPH_W * scale, GLYPH_H * scale };
    SDL_RenderCopy(ren, atlas->tex, &src, &dst);
  }
}

static void draw_centered_text(SDL_Renderer *ren, const GlyphAtlas *atlas,
                               int cx, int y, int scale, const char *text,
                               unsigned char cr, unsigned char cg, unsigned char cb, unsigned char ca) {
  draw_text(ren, atlas, cx - text_width(text, scale) / 2, y, scale, text, cr, cg, cb, ca);
}

static void draw_outline(SDL_Renderer *ren, const SDL_Rect *r, int thick,
                         unsigned char cr, unsigned char cg, unsigned char cb, unsigned char ca) {
  SDL_SetRenderDrawColor(ren, cr, cg, cb, ca);
  for (int i = 0; i < thick; i++) {
    SDL_Rect o = { r->x - i, r->y - i, r->w + i * 2, r->h + i * 2 };
    SDL_RenderDrawRect(ren, &o);
  }
}

static int rect_contains(const SDL_Rect *r, int x, int y) {
  return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

static SDL_Rect options_button_rect(int w, int h) {
  float s = ui_scale(w, h);
  int bw = (int)(230.0f * s + 0.5f);
  int bh = (int)(56.0f * s + 0.5f);
  int margin = (int)(38.0f * s + 0.5f);
  if (bw < 156) bw = 156;
  if (bh < 38) bh = 38;
  if (margin < 24) margin = 24;
  SDL_Rect r = { w - bw - margin, margin, bw, bh };
  return r;
}

static SDL_Rect centered_panel_rect(int w, int h, float bw, float bh) {
  float s = ui_scale(w, h);
  int pw = (int)(bw * s + 0.5f);
  int ph = (int)(bh * s + 0.5f);
  if (pw > w - 80) pw = w - 80;
  if (ph > h - 80) ph = h - 80;
  if (pw < 560) pw = 560;
  if (ph < 360) ph = 360;
  SDL_Rect r = { (w - pw) / 2, (h - ph) / 2, pw, ph };
  return r;
}

static void draw_options_button(SDL_Renderer *ren, const GlyphAtlas *atlas,
                                int w, int h, int active) {
  float s = ui_scale(w, h);
  int scale = s < 0.75f ? 2 : 3;
  SDL_Rect r = options_button_rect(w, h);

  SDL_SetRenderDrawColor(ren, 20, 20, 28, 220);
  SDL_RenderFillRect(ren, &r);
  draw_outline(ren, &r, active ? 4 : 2,
               active ? 255 : 120, active ? 170 : 120, active ? 0 : 125, 235);
  draw_centered_text(ren, atlas, r.x + r.w / 2, r.y + (r.h - 7 * scale) / 2,
                     scale, "OPTIONS", 235, 235, 220, 255);
}

// ---- options menu --------------------------------------------------------

static const char *option_label(int row) {
  static const char *const labels[NUM_OPTION_ROWS] = {
    "SHOW FPS",
    "LANGUAGE",
    "GAMEPAD",
    "TOUCH HUD",
    "CONSOLE",
    "SKIP VIDEOS",
    "AUTO BOOT",
    "VSYNC",
  };
  return labels[row];
}

static int option_enabled(int row) {
  switch (row) {
  case 0: return config.show_fps;
  case 2: return config.gamepad;
  case 3: return config.touch_hud;
  case 4: return config.console;
  case 5: return config.skip_videos;
  case 6: return config.auto_boot;
  case 7: return config.vsync;
  default: return 0;
  }
}

// Row 1 (LANGUAGE) is a sub-menu trigger; row 7 (RESET) is implicit via the
// X button on the controller.  All other rows are toggles.
static void toggle_option(int row) {
  switch (row) {
  case 0: config.show_fps    = !config.show_fps;    break;
  case 2: config.gamepad     = !config.gamepad;     break;
  case 3: config.touch_hud   = !config.touch_hud;   break;
  case 4: config.console     = !config.console;     break;
  case 5: config.skip_videos = !config.skip_videos; break;
  case 6: config.auto_boot   = !config.auto_boot;   break;
  case 7: config.vsync       = !config.vsync;       break;
  default: return;
  }
  write_config(CONFIG_NAME);
}

static void set_language_from_row(const int *langs, int row) {
  snprintf(config.lang, sizeof(config.lang), "%s", language_options[langs[row]].value);
  write_config(CONFIG_NAME);
}

static void cycle_language(const int *langs, int count, int dir) {
  int row;
  if (count <= 0)
    return;
  row = selected_language_row(langs, count);
  row = (row + dir + count) % count;
  set_language_from_row(langs, row);
}

static void draw_toggle_box(SDL_Renderer *ren, const SDL_Rect *box, int enabled) {
  SDL_SetRenderDrawColor(ren, 14, 14, 18, 240);
  SDL_RenderFillRect(ren, box);
  draw_outline(ren, box, 2, 210, 210, 205, 230);
  if (enabled) {
    SDL_Rect fill = { box->x + box->w / 4, box->y + box->h / 4,
                      box->w / 2, box->h / 2 };
    SDL_SetRenderDrawColor(ren, 255, 170, 0, 255);
    SDL_RenderFillRect(ren, &fill);
  }
}

static SDL_Rect option_row_rect(const SDL_Rect *panel, int row, int w, int h) {
  float s = ui_scale(w, h);
  int row_h = (int)(74.0f * s + 0.5f);
  int gap_x = (int)(40.0f * s + 0.5f);
  int top = panel->y + (int)(128.0f * s + 0.5f);
  if (row_h < 48) row_h = 48;
  if (gap_x < 24) gap_x = 24;
  SDL_Rect r = { panel->x + gap_x, top + row * row_h,
                 panel->w - gap_x * 2, row_h - 8 };
  return r;
}

static void draw_options_menu(SDL_Renderer *ren, const GlyphAtlas *atlas,
                              int w, int h, int selected,
                              const int *langs, int lang_count) {
  float s = ui_scale(w, h);
  int scale = s < 0.75f ? 2 : 3;
  int box = (int)(30.0f * s + 0.5f);
  SDL_Rect full = { 0, 0, w, h };
  // Panel is taller now that we have 8 rows + a footer hint.
  SDL_Rect panel = centered_panel_rect(w, h, 920.0f, 820.0f);

  if (box < 22) box = 22;

  SDL_SetRenderDrawColor(ren, 0, 0, 0, 150);
  SDL_RenderFillRect(ren, &full);
  SDL_SetRenderDrawColor(ren, 14, 14, 20, 245);
  SDL_RenderFillRect(ren, &panel);
  draw_outline(ren, &panel, 3, 255, 170, 0, 235);
  draw_centered_text(ren, atlas, panel.x + panel.w / 2,
                     panel.y + (int)(42.0f * s + 0.5f),
                     scale + 1, "OPTIONS", 255, 205, 80, 255);

  for (int i = 0; i < NUM_OPTION_ROWS; i++) {
    SDL_Rect row = option_row_rect(&panel, i, w, h);
    int active = i == selected;

    if (active) {
      SDL_SetRenderDrawColor(ren, 52, 38, 22, 220);
      SDL_RenderFillRect(ren, &row);
      draw_outline(ren, &row, 2, 255, 170, 0, 235);
    }

    draw_text(ren, atlas, row.x + (int)(24.0f * s + 0.5f),
              row.y + (row.h - 7 * scale) / 2,
              scale, option_label(i), 230, 230, 220, 255);

    if (i == 1) {
      int lang_row = selected_language_row(langs, lang_count);
      const char *label = language_options[langs[lang_row]].label;
      int value_w = text_width(label, scale) + (int)(42.0f * s + 0.5f);
      SDL_Rect value = { row.x + row.w - value_w - (int)(24.0f * s + 0.5f),
                         row.y + (row.h - box) / 2, value_w, box };
      SDL_SetRenderDrawColor(ren, 20, 20, 28, 230);
      SDL_RenderFillRect(ren, &value);
      draw_outline(ren, &value, 2, 180, 180, 175, 220);
      draw_centered_text(ren, atlas, value.x + value.w / 2,
                         value.y + (value.h - 7 * scale) / 2,
                         scale, label, 235, 235, 225, 255);
    } else {
      int enabled = option_enabled(i);
      SDL_Rect cb = { row.x + row.w - box - (int)(96.0f * s + 0.5f),
                      row.y + (row.h - box) / 2, box, box };
      draw_toggle_box(ren, &cb, enabled);
      draw_text(ren, atlas, cb.x + cb.w + (int)(18.0f * s + 0.5f),
                row.y + (row.h - 7 * scale) / 2,
                scale, enabled ? "ON" : "OFF",
                enabled ? 255 : 150, enabled ? 205 : 150, enabled ? 80 : 155, 255);
    }
  }

  // Footer hint
  const char *hint = "X RESET TO DEFAULTS    Y HELP    B BACK";
  draw_centered_text(ren, atlas, panel.x + panel.w / 2,
                     panel.y + panel.h - (int)(52.0f * s + 0.5f),
                     scale, hint, 160, 160, 160, 220);
}

static void keep_language_visible(int selected, int *scroll, int count) {
  int max_scroll = count > LANG_VISIBLE_ROWS ? count - LANG_VISIBLE_ROWS : 0;
  if (selected < *scroll)
    *scroll = selected;
  if (selected >= *scroll + LANG_VISIBLE_ROWS)
    *scroll = selected - LANG_VISIBLE_ROWS + 1;
  if (*scroll > max_scroll)
    *scroll = max_scroll;
  if (*scroll < 0)
    *scroll = 0;
}

static void refresh_language_state(int *langs, int *count, int *selected, int *scroll) {
  build_language_list(langs, count);
  *selected = selected_language_row(langs, *count);
  keep_language_visible(*selected, scroll, *count);
}

static SDL_Rect language_row_rect(const SDL_Rect *panel, int visible_row, int w, int h) {
  float s = ui_scale(w, h);
  int row_h = (int)(58.0f * s + 0.5f);
  int gap_x = (int)(42.0f * s + 0.5f);
  int top = panel->y + (int)(116.0f * s + 0.5f);
  if (row_h < 42) row_h = 42;
  if (gap_x < 24) gap_x = 24;
  SDL_Rect r = { panel->x + gap_x, top + visible_row * row_h,
                 panel->w - gap_x * 2, row_h - 6 };
  return r;
}

static void draw_language_menu(SDL_Renderer *ren, const GlyphAtlas *atlas,
                               int w, int h, int selected,
                               int scroll, const int *langs, int lang_count) {
  float s = ui_scale(w, h);
  int scale = s < 0.75f ? 2 : 3;
  int box = (int)(26.0f * s + 0.5f);
  SDL_Rect full = { 0, 0, w, h };
  SDL_Rect panel = centered_panel_rect(w, h, 820.0f, 820.0f);
  int current = selected_language_row(langs, lang_count);
  int visible = lang_count < LANG_VISIBLE_ROWS ? lang_count : LANG_VISIBLE_ROWS;

  if (box < 20) box = 20;

  SDL_SetRenderDrawColor(ren, 0, 0, 0, 165);
  SDL_RenderFillRect(ren, &full);
  SDL_SetRenderDrawColor(ren, 14, 14, 20, 248);
  SDL_RenderFillRect(ren, &panel);
  draw_outline(ren, &panel, 3, 255, 170, 0, 235);
  draw_centered_text(ren, atlas, panel.x + panel.w / 2,
                     panel.y + (int)(40.0f * s + 0.5f),
                     scale + 1, "LANGUAGE", 255, 205, 80, 255);

  for (int i = 0; i < visible; i++) {
    int lang_row = scroll + i;
    SDL_Rect row = language_row_rect(&panel, i, w, h);
    int active = lang_row == selected;
    int picked = lang_row == current;

    if (active) {
      SDL_SetRenderDrawColor(ren, 52, 38, 22, 220);
      SDL_RenderFillRect(ren, &row);
      draw_outline(ren, &row, 2, 255, 170, 0, 235);
    }

    SDL_Rect cb = { row.x + (int)(20.0f * s + 0.5f),
                    row.y + (row.h - box) / 2, box, box };
    draw_toggle_box(ren, &cb, picked);
    draw_text(ren, atlas, cb.x + cb.w + (int)(28.0f * s + 0.5f),
              row.y + (row.h - 7 * scale) / 2,
              scale, language_options[langs[lang_row]].label,
              picked ? 255 : 230, picked ? 205 : 230, picked ? 80 : 220, 255);
  }
}

// ---- help overlay --------------------------------------------------------

static void draw_help_menu(SDL_Renderer *ren, const GlyphAtlas *atlas, int w, int h) {
  float s = ui_scale(w, h);
  int scale = s < 0.75f ? 2 : 3;
  SDL_Rect full = { 0, 0, w, h };
  SDL_Rect panel = centered_panel_rect(w, h, 1100.0f, 760.0f);

  SDL_SetRenderDrawColor(ren, 0, 0, 0, 165);
  SDL_RenderFillRect(ren, &full);
  SDL_SetRenderDrawColor(ren, 14, 14, 20, 248);
  SDL_RenderFillRect(ren, &panel);
  draw_outline(ren, &panel, 3, 255, 170, 0, 235);
  draw_centered_text(ren, atlas, panel.x + panel.w / 2,
                     panel.y + (int)(40.0f * s + 0.5f),
                     scale + 1, "HELP", 255, 205, 80, 255);

  static const char *const lines[] = {
    "CONTROLS",
    " DPAD / STICK     NAVIGATE",
    " A                SELECT GAME",
    " B                SELECT GAME / OPEN OPTIONS",
    " X                RESET OPTIONS TO DEFAULTS",
    " Y                TOGGLE THIS HELP",
    " START            EXIT",
    "",
    "GETTING STARTED",
    " 1 COPY HL2 GAME DATA TO /SWITCH/HL2_NX/",
    " 2 COPY APK LIBS TO /SWITCH/HL2_NX/LIB/",
    " 3 COPY ASSETS/EXTRAS_DIR.VPK TO ASSETS/",
    " 4 COPY FONTS FROM APK ASSETS/ TO FILES/",
    " 5 EP1 EP2 OPTIONAL COPY EPISODIC LIBS",
    "",
    "TIPS",
    " USE STEAM_LEGACY BRANCH HL2 DATA ONLY",
    " NEW 20TH ANNIVERSARY DATA IS NOT SUPPORTED",
    " DO NOT RUN IN APPLETS ALBUM MODE",
    " HOLD R ON ANY TITLE TO LAUNCH AS OVERRIDE",
  };

  int y = panel.y + (int)(100.0f * s + 0.5f);
  int lh = (int)(26.0f * s + 0.5f);
  if (lh < 18) lh = 18;
  for (unsigned i = 0; i < sizeof(lines) / sizeof(*lines); i++) {
    const char *ln = lines[i];
    unsigned char r = 235, g = 235, b = 220;
    if (ln[0] != ' ' && ln[0] != '\0') { r = 255; g = 205; b = 80; }
    draw_text(ren, atlas, panel.x + (int)(60.0f * s + 0.5f), y, scale, ln, r, g, b, 235);
    y += lh;
  }

  draw_centered_text(ren, atlas, panel.x + panel.w / 2,
                     panel.y + panel.h - (int)(52.0f * s + 0.5f),
                     scale, "PRESS Y OR B TO CLOSE", 160, 160, 160, 220);
}

static void save_selection(int selected) {
  snprintf(config.gamedir, sizeof(config.gamedir), "%s", game_dirs[selected]);
  write_config(CONFIG_NAME);
}

// Refuse to save auto_boot if the chosen game isn't playable -- otherwise
// the user could soft-lock themselves out of the picker on next boot.
static int can_auto_boot(const GameInstall *install) {
  for (int i = 0; i < NUM_GAMES; i++) {
    if (!strcmp(config.gamedir, game_dirs[i]))
      return install[i].playable ? 1 : 0;
  }
  return 0;
}

int picker_run(void) {
  int result = -1;
  int win_w = appletGetOperationMode() == AppletOperationMode_Console ? 1920 : 1280;
  int win_h = appletGetOperationMode() == AppletOperationMode_Console ? 1080 : 720;
  GameInstall install[NUM_GAMES];

  scan_installs(install);

  if (R_FAILED(romfsInit()))
    return -1;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
    goto out_romfs;

  if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG))
    goto out_sdl;

  // SDL hints: batch draws, prefer GPU compositing, drop the compositor pass.
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, config.vsync ? "1" : "0");
  SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "1");

  SDL_Window *win = SDL_CreateWindow("Half-Life 2", SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED, win_w, win_h,
                                     SDL_WINDOW_FULLSCREEN);
  if (!win)
    goto out_img;

  SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
      SDL_RENDERER_ACCELERATED | (config.vsync ? SDL_RENDERER_PRESENTVSYNC : 0));
  if (!ren)
    goto out_win;

  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

  int screen_w = win_w;
  int screen_h = win_h;
  SDL_GetRendererOutputSize(ren, &screen_w, &screen_h);
  if (screen_w <= 0 || screen_h <= 0) {
    screen_w = win_w;
    screen_h = win_h;
  }

  // Pre-load the box-art textures once.  Set blend mode + scaling filter so
  // they look right at every dock state without per-frame state changes.
  SDL_Texture *tex[NUM_GAMES] = { 0 };
  for (int i = 0; i < NUM_GAMES; i++) {
    SDL_Surface *s = IMG_Load(image_paths[i]);
    if (s) {
      tex[i] = SDL_CreateTextureFromSurface(ren, s);
      if (tex[i]) {
        SDL_SetTextureBlendMode(tex[i], SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex[i], SDL_ScaleModeLinear);
      }
      SDL_FreeSurface(s);
    }
  }

  GlyphAtlas atlas;
  atlas_build(&atlas, ren);

  SDL_GameController *ctrl = NULL;
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      ctrl = SDL_GameControllerOpen(i);
      if (ctrl) break;
    }
  }

  int selected = selected_from_config();
  int last_game_selected = selected;
  int option_selected = 0;
  int lang_options[MAX_LANG_OPTIONS];
  int lang_count = 0;
  int language_selected = 0;
  int language_scroll = 0;
  PickerView view = PICKER_MAIN;
  float pulse_time = 0.0f;
  Uint32 prev_ms = SDL_GetTicks();
  bool running = true;
  bool stick_moved_x = false;
  bool stick_moved_y = false;

  refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);

  // Skip straight to OPTIONS the first time the user boots with no config
  // (write_config will have been called by main, but we still want to greet
  // them with the picker, not the bare menu).
  while (running) {
    Uint32 now_ms = SDL_GetTicks();
    float dt = (now_ms - prev_ms) * 0.001f;
    prev_ms = now_ms;

    pulse_time += dt;
    if (pulse_time >= PULSE_PERIOD)
      pulse_time -= PULSE_PERIOD;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_QUIT:
        result = 0;
        running = false;
        break;

      case SDL_CONTROLLERBUTTONDOWN:
        switch (ev.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
          if (view == PICKER_MAIN) {
            selected = move_selection(selected, -1);
            if (selected < NUM_GAMES)
              last_game_selected = selected;
            pulse_time = 0.0f;
          } else if (view == PICKER_OPTIONS) {
            if (option_selected == 1) {
              cycle_language(lang_options, lang_count, -1);
              refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
            } else {
              toggle_option(option_selected);
            }
          } else if (view == PICKER_LANGUAGE) {
            language_selected -= LANG_VISIBLE_ROWS;
            if (language_selected < 0) language_selected = 0;
            keep_language_visible(language_selected, &language_scroll, lang_count);
          }
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
          if (view == PICKER_MAIN) {
            selected = move_selection(selected, 1);
            if (selected < NUM_GAMES)
              last_game_selected = selected;
            pulse_time = 0.0f;
          } else if (view == PICKER_OPTIONS) {
            if (option_selected == 1) {
              cycle_language(lang_options, lang_count, 1);
              refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
            } else {
              toggle_option(option_selected);
            }
          } else if (view == PICKER_LANGUAGE) {
            language_selected += LANG_VISIBLE_ROWS;
            if (language_selected >= lang_count) language_selected = lang_count - 1;
            keep_language_visible(language_selected, &language_scroll, lang_count);
          }
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
          if (view == PICKER_MAIN) {
            if (selected < NUM_GAMES)
              last_game_selected = selected;
            selected = OPTIONS_INDEX;
            pulse_time = 0.0f;
          } else if (view == PICKER_OPTIONS) {
            option_selected = (option_selected + NUM_OPTION_ROWS - 1) % NUM_OPTION_ROWS;
          } else if (view == PICKER_LANGUAGE) {
            language_selected = (language_selected + lang_count - 1) % lang_count;
            keep_language_visible(language_selected, &language_scroll, lang_count);
          }
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
          if (view == PICKER_MAIN) {
            if (selected == OPTIONS_INDEX) {
              selected = last_game_selected;
              pulse_time = 0.0f;
            }
          } else if (view == PICKER_OPTIONS) {
            option_selected = (option_selected + 1) % NUM_OPTION_ROWS;
          } else if (view == PICKER_LANGUAGE) {
            language_selected = (language_selected + 1) % lang_count;
            keep_language_visible(language_selected, &language_scroll, lang_count);
          }
          break;
        case SDL_CONTROLLER_BUTTON_X:
          // Reset to factory defaults (keeps gamedir/lang).
          if (view == PICKER_OPTIONS) {
            int saved_auto = config.auto_boot;
            char saved_game[64];
            char saved_lang[32];
            strlcpy_(saved_game, config.gamedir, sizeof(saved_game));
            strlcpy_(saved_lang, config.lang, sizeof(saved_lang));
            config_set_defaults();
            strlcpy_(config.gamedir, saved_game, sizeof(config.gamedir));
            strlcpy_(config.lang, saved_lang, sizeof(config.lang));
            (void)saved_auto;
            write_config(CONFIG_NAME);
          }
          break;
        case SDL_CONTROLLER_BUTTON_Y:
          // Toggle help overlay from anywhere.
          if (view == PICKER_HELP) view = PICKER_MAIN;
          else if (view == PICKER_MAIN) view = PICKER_HELP;
          break;
        case SDL_CONTROLLER_BUTTON_B:
          if (view == PICKER_MAIN && selected == OPTIONS_INDEX) {
            view = PICKER_OPTIONS;
            option_selected = 0;
          } else if (view == PICKER_MAIN && install[selected].playable) {
            save_selection(selected);
            result = 1;
            running = false;
          } else if (view == PICKER_OPTIONS) {
            if (option_selected == 1) {
              refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
              view = PICKER_LANGUAGE;
            } else {
              toggle_option(option_selected);
            }
          } else if (view == PICKER_LANGUAGE) {
            set_language_from_row(lang_options, language_selected);
            refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
            view = PICKER_OPTIONS;
          } else if (view == PICKER_HELP) {
            view = PICKER_MAIN;
          }
          break;
        case SDL_CONTROLLER_BUTTON_START:
          if (view != PICKER_MAIN) {
            view = PICKER_MAIN;
            break;
          }
          result = 0;
          running = false;
          break;
        case SDL_CONTROLLER_BUTTON_A:
          if (view == PICKER_LANGUAGE) {
            view = PICKER_OPTIONS;
          } else if (view == PICKER_OPTIONS) {
            view = PICKER_MAIN;
          } else if (view == PICKER_HELP) {
            view = PICKER_MAIN;
          } else {
            result = 0;
            running = false;
          }
          break;
        }
        break;

      case SDL_CONTROLLERAXISMOTION:
        if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
          if (!stick_moved_x && ev.caxis.value < -STICK_THRESHOLD) {
            stick_moved_x = true;
            if (view == PICKER_MAIN) {
              selected = move_selection(selected, -1);
              if (selected < NUM_GAMES)
                last_game_selected = selected;
              pulse_time = 0.0f;
            } else if (view == PICKER_OPTIONS) {
              if (option_selected == 1) {
                cycle_language(lang_options, lang_count, -1);
                refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
              } else {
                toggle_option(option_selected);
              }
            } else if (view == PICKER_LANGUAGE) {
              language_selected -= LANG_VISIBLE_ROWS;
              if (language_selected < 0) language_selected = 0;
              keep_language_visible(language_selected, &language_scroll, lang_count);
            }
          } else if (!stick_moved_x && ev.caxis.value > STICK_THRESHOLD) {
            stick_moved_x = true;
            if (view == PICKER_MAIN) {
              selected = move_selection(selected, 1);
              if (selected < NUM_GAMES)
                last_game_selected = selected;
              pulse_time = 0.0f;
            } else if (view == PICKER_OPTIONS) {
              if (option_selected == 1) {
                cycle_language(lang_options, lang_count, 1);
                refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
              } else {
                toggle_option(option_selected);
              }
            } else if (view == PICKER_LANGUAGE) {
              language_selected += LANG_VISIBLE_ROWS;
              if (language_selected >= lang_count) language_selected = lang_count - 1;
              keep_language_visible(language_selected, &language_scroll, lang_count);
            }
          } else if (ev.caxis.value > -STICK_DEADZONE && ev.caxis.value < STICK_DEADZONE) {
            stick_moved_x = false;
          }
        } else if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
          if (!stick_moved_y && ev.caxis.value < -STICK_THRESHOLD) {
            stick_moved_y = true;
            if (view == PICKER_MAIN) {
              if (selected < NUM_GAMES)
                last_game_selected = selected;
              selected = OPTIONS_INDEX;
              pulse_time = 0.0f;
            } else if (view == PICKER_OPTIONS) {
              option_selected = (option_selected + NUM_OPTION_ROWS - 1) % NUM_OPTION_ROWS;
            } else if (view == PICKER_LANGUAGE) {
              language_selected = (language_selected + lang_count - 1) % lang_count;
              keep_language_visible(language_selected, &language_scroll, lang_count);
            }
          } else if (!stick_moved_y && ev.caxis.value > STICK_THRESHOLD) {
            stick_moved_y = true;
            if (view == PICKER_MAIN) {
              if (selected == OPTIONS_INDEX) {
                selected = last_game_selected;
                pulse_time = 0.0f;
              }
            } else if (view == PICKER_OPTIONS) {
              option_selected = (option_selected + 1) % NUM_OPTION_ROWS;
            } else if (view == PICKER_LANGUAGE) {
              language_selected = (language_selected + 1) % lang_count;
              keep_language_visible(language_selected, &language_scroll, lang_count);
            }
          } else if (ev.caxis.value > -STICK_DEADZONE && ev.caxis.value < STICK_DEADZONE) {
            stick_moved_y = false;
          }
        }
        break;

      case SDL_FINGERDOWN: {
        int tx = (int)(ev.tfinger.x * (float)screen_w);
        int ty = (int)(ev.tfinger.y * (float)screen_h);
        if (view == PICKER_MAIN) {
          SDL_Rect opt = options_button_rect(screen_w, screen_h);
          if (rect_contains(&opt, tx, ty)) {
            if (selected < NUM_GAMES)
              last_game_selected = selected;
            selected = OPTIONS_INDEX;
            view = PICKER_OPTIONS;
            option_selected = 0;
            break;
          }

          for (int i = 0; i < NUM_GAMES; i++) {
            SDL_Rect hit = base_rect(i, screen_w, screen_h);
            if (rect_contains(&hit, tx, ty)) {
              selected = i;
              last_game_selected = i;
              pulse_time = 0.0f;
              if (install[i].playable) {
                save_selection(selected);
                result = 1;
                running = false;
              }
              break;
            }
          }
        } else if (view == PICKER_OPTIONS) {
          SDL_Rect panel = centered_panel_rect(screen_w, screen_h, 920.0f, 820.0f);
          int handled = 0;
          for (int i = 0; i < NUM_OPTION_ROWS; i++) {
            SDL_Rect row = option_row_rect(&panel, i, screen_w, screen_h);
            if (rect_contains(&row, tx, ty)) {
              option_selected = i;
              if (i == 1) {
                refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
                view = PICKER_LANGUAGE;
              } else {
                toggle_option(i);
              }
              handled = 1;
              break;
            }
          }
          if (!handled && !rect_contains(&panel, tx, ty))
            view = PICKER_MAIN;
        } else if (view == PICKER_LANGUAGE) {
          SDL_Rect panel = centered_panel_rect(screen_w, screen_h, 820.0f, 820.0f);
          int visible = lang_count < LANG_VISIBLE_ROWS ? lang_count : LANG_VISIBLE_ROWS;
          int handled = 0;
          for (int i = 0; i < visible; i++) {
            int row_index = language_scroll + i;
            SDL_Rect row = language_row_rect(&panel, i, screen_w, screen_h);
            if (row_index < lang_count && rect_contains(&row, tx, ty)) {
              language_selected = row_index;
              set_language_from_row(lang_options, language_selected);
              refresh_language_state(lang_options, &lang_count, &language_selected, &language_scroll);
              view = PICKER_OPTIONS;
              handled = 1;
              break;
            }
          }
          if (!handled && !rect_contains(&panel, tx, ty))
            view = PICKER_OPTIONS;
        }
        break;
      }
      }
    }

    // ---- render ----
    float t = pulse_time / PULSE_PERIOD;
    float scale = 1.0f + (PULSE_MAX_SCALE - 1.0f) * 0.5f *
                  (1.0f - cosf(t * 6.28318530718f));
    float s = ui_scale(screen_w, screen_h);
    int status_scale = s < 0.75f ? 2 : 3;
    int detail_scale = status_scale;
    int status_y = screen_h - (int)(58.0f * s + 0.5f);
    int detail_y = (int)(28.0f * s + 0.5f);
    if (detail_y < 18) detail_y = 18;

    SDL_SetRenderDrawColor(ren, 10, 10, 15, 255);
    SDL_RenderClear(ren);

    for (int i = 0; i < NUM_GAMES; i++) {
      SDL_Rect base = base_rect(i, screen_w, screen_h);
      SDL_Rect dst = base;
      int active = i == selected;

      if (active && install[i].playable) {
        dst.w = (int)((float)base.w * scale);
        dst.h = (int)((float)base.h * scale);
        dst.x = base.x + (base.w - dst.w) / 2;
        dst.y = base.y + (base.h - dst.h) / 2;
        draw_border(ren, &dst, screen_w, screen_h, 255, 170, 0);
      } else if (active) {
        draw_border(ren, &dst, screen_w, screen_h, 220, 70, 55);
      }

      if (tex[i]) {
        if (install[i].playable) {
          SDL_SetTextureColorMod(tex[i], active ? 255 : 155, active ? 255 : 155, active ? 255 : 155);
          SDL_SetTextureAlphaMod(tex[i], 255);
        } else {
          SDL_SetTextureColorMod(tex[i], 65, 65, 65);
          SDL_SetTextureAlphaMod(tex[i], 190);
        }
        SDL_RenderCopy(ren, tex[i], NULL, &dst);
      }

      if (!install[i].playable) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 115);
        SDL_RenderFillRect(ren, &dst);
      }

      if (install[i].status[0]) {
        draw_centered_text(ren, &atlas, base.x + base.w / 2, status_y, status_scale,
                           install[i].status,
                           install[i].playable ? 255 : 235,
                           install[i].playable ? 185 : 95,
                           install[i].playable ? 80 : 75, 255);
      }
    }

    if (selected < NUM_GAMES && install[selected].detail[0]) {
      draw_centered_text(ren, &atlas, screen_w / 2, detail_y, detail_scale,
                         install[selected].detail, 255, 185, 80, 255);
    }

    // Footer hint on the main view (so newcomers don't get lost).
    if (view == PICKER_MAIN) {
      const char *hint = can_auto_boot(install)
        ? "A SELECT    B OPTIONS    Y HELP    START EXIT"
        : "B SELECT    Y HELP    START EXIT";
      draw_centered_text(ren, &atlas, screen_w / 2,
                         screen_h - (int)(28.0f * s + 0.5f),
                         s < 0.75f ? 2 : 3, hint, 150, 150, 150, 220);
    }

    draw_options_button(ren, &atlas, screen_w, screen_h,
                        selected == OPTIONS_INDEX && view == PICKER_MAIN);

    if (view == PICKER_OPTIONS)
      draw_options_menu(ren, &atlas, screen_w, screen_h, option_selected, lang_options, lang_count);
    else if (view == PICKER_LANGUAGE)
      draw_language_menu(ren, &atlas, screen_w, screen_h, language_selected, language_scroll,
                         lang_options, lang_count);
    else if (view == PICKER_HELP)
      draw_help_menu(ren, &atlas, screen_w, screen_h);

    SDL_RenderPresent(ren);
  }

  atlas_free(&atlas);
  if (ctrl)
    SDL_GameControllerClose(ctrl);
  for (int i = 0; i < NUM_GAMES; i++) {
    if (tex[i])
      SDL_DestroyTexture(tex[i]);
  }
  SDL_DestroyRenderer(ren);
out_win:
  SDL_DestroyWindow(win);
out_img:
  IMG_Quit();
out_sdl:
  SDL_Quit();
out_romfs:
  romfsExit();
  return result;
}
