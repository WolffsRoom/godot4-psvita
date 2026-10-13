#ifndef DODGE_BOOT_SPLASH_H
#define DODGE_BOOT_SPLASH_H
/* Returns 1 when the dedicated visual thread was started, 0 on failure. */
int dodge_boot_splash_start(void);
/* Wait for one full six-second 30 FPS cycle and join before Godot GXM starts. */
void dodge_boot_splash_stop(void);
#endif
