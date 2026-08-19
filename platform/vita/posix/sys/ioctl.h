/* Vita stub: VitaSDK newlib has no <sys/ioctl.h>. ioctl() always fails, so
 * drivers/unix's optional uses (dir flags, pipe FIONREAD) degrade gracefully. */
#pragma once

#ifndef FIONREAD
#define FIONREAD 0x541B
#endif
#ifndef _IOR
#define _IOR(type, nr, size) (((type) << 8) | (nr))
#endif
#ifndef _IOW
#define _IOW(type, nr, size) (((type) << 8) | (nr))
#endif

static inline int ioctl(int, unsigned long, ...) { return -1; }
