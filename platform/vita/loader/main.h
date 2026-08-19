#ifndef __MAIN_H__
#define __MAIN_H__

// Fixed base address the .so is mapped at (standard for scene so-loaders).
#define LOAD_ADDRESS 0x98000000

// Provided by main.c; used by so_util.c for logging / fatal errors.
int debugPrintf(const char *fmt, ...);
void fatal_error(const char *fmt, ...);

#endif
