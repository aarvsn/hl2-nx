/* util.c -- misc utility functions
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "util.h"
#include "config.h"

#if DEBUG_LOG

static int s_nxlinkSock = -1;

static void initNxLink(void) {
  if (R_FAILED(socketInitializeDefault()))
    return;
  s_nxlinkSock = nxlinkStdio();
  if (s_nxlinkSock < 0)
    socketExit();
}

static void deinitNxLink(void) {
  if (s_nxlinkSock >= 0) {
    close(s_nxlinkSock);
    socketExit();
    s_nxlinkSock = -1;
  }
}

void userAppInit(void) {
  initNxLink();
}

void userAppExit(void) {
  deinitNxLink();
}

#endif

int debugPrintf(const char *text, ...) {
#if DEBUG_LOG
  va_list list;

  FILE *f = fopen(LOG_NAME, "a");
  if (f) {
    va_start(list, text);
    vfprintf(f, text, list);
    va_end(list);
    fclose(f);
  }

  va_start(list, text);
  vprintf(text, list);
  va_end(list);
#endif
  return 0;
}

// boost the CPU to 1785MHz while loading
void cpu_boost(int on) {
  appletSetCpuBoostMode(on ? ApmCpuBoostMode_FastLoad : ApmCpuBoostMode_Normal);
}

int ret0(void) { return 0; }

int retm1(void) { return -1; }

// --- shared file helpers ---------------------------------------------------

int file_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

// Stream-search a binary file for a literal needle. Used by main.c and
// picker.c to confirm episodic libserver.so is the real one (not a stub
// shipped in older APKs). 8 KB ring buffer covers the longest needle we
// care about with plenty of headroom.
int file_contains_text(const char *path, const char *needle) {
  unsigned char buf[8192];
  size_t needle_len = strlen(needle);
  size_t keep = 0;
  FILE *f;

  if (needle_len == 0 || needle_len >= sizeof(buf))
    return 0;

  f = fopen(path, "rb");
  if (!f)
    return 0;

  for (;;) {
    size_t got = fread(buf + keep, 1, sizeof(buf) - keep, f);
    size_t total = keep + got;

    if (total >= needle_len) {
      for (size_t i = 0; i <= total - needle_len; i++) {
        if (!memcmp(buf + i, needle, needle_len)) {
          fclose(f);
          return 1;
        }
      }
    }

    if (got == 0)
      break;

    keep = total < needle_len - 1 ? total : needle_len - 1;
    memmove(buf, buf + total - keep, keep);
  }

  fclose(f);
  return 0;
}
