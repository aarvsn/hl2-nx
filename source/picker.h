#ifndef __PICKER_H__
#define __PICKER_H__

// Run the full game-selection UI. Returns 1 to launch, 0 to exit.
int picker_run(void);

// Lightweight pre-flight check used by the auto-boot path: returns 1 if the
// configured config.gamedir is installed and playable, 0 if it isn't but
// the picker should still appear, or -1 if the picker must run because the
// configured game isn't installed at all.
int picker_quick_check(void);

#endif
