/* Vita stub: VitaSDK newlib has no <sys/xattr.h>. Extended attributes are
 * unsupported on Vita; these no-op stubs (Linux-style signatures — the branch
 * drivers/unix/file_access_unix.cpp takes on non-Apple/BSD) let it compile. */
#pragma once

#include <sys/types.h>

static inline ssize_t getxattr(const char *, const char *, void *, size_t) { return -1; }
static inline int setxattr(const char *, const char *, const void *, size_t, int) { return -1; }
static inline int removexattr(const char *, const char *) { return -1; }
static inline ssize_t listxattr(const char *, char *, size_t) { return -1; }
