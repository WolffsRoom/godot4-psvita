/**************************************************************************/
/*  vita_posix_compat.c                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* POSIX functions that VitaSDK's newlib DECLARES (so drivers/unix compiles)
 * but does NOT implement (so it fails to link). Providing them here resolves
 * the link. Most are process-spawning / symlink / signal features Vita lacks,
 * stubbed to fail gracefully; posix_memalign is implemented for real.
 *
 * NB: with `-Wl,-q` (--emit-relocs) an UNDEFINED symbol surfaces as the opaque
 * "final link failed: invalid operation" instead of "undefined reference", so
 * every one of these had to be resolved before the engine could link at all.  */

#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

/* Normally provided by crtbegin.o for C++ static-destructor registration
 * (__cxa_atexit). The so-loader build links -nostdlib (no crt objects), so we
 * define it here. */
void *__dso_handle = (void *)&__dso_handle;

/* Real implementation: Godot uses this for aligned allocations. */
int posix_memalign(void **memptr, size_t alignment, size_t size) {
	void *p = memalign(alignment, size);
	if (!p) {
		return ENOMEM;
	}
	*memptr = p;
	return 0;
}

/* --- Unsupported on Vita: fail gracefully -------------------------------- */

ssize_t readlink(const char *, char *, size_t) {
	errno = ENOSYS;
	return -1;
}

int symlink(const char *, const char *) {
	errno = ENOSYS;
	return -1;
}

int dup2(int, int) {
	errno = ENOSYS;
	return -1;
}

int mkfifo(const char *, mode_t) {
	errno = ENOSYS;
	return -1;
}

/* Process spawning — Vita has no fork/exec model. */
int execvp(const char *, char *const[]) {
	errno = ENOSYS;
	return -1;
}

pid_t waitpid(pid_t, int *, int) {
	errno = ENOSYS;
	return -1;
}

FILE *popen(const char *, const char *) {
	errno = ENOSYS;
	return NULL;
}

int pclose(FILE *) {
	errno = ENOSYS;
	return -1;
}

/* NOTE: getrlimit lives in vita_cpp_compat.cpp — VitaSDK declares it without
 * extern "C", so the C++ caller needs a C++-mangled definition, not this C one. */

/* Signals: Vita has no POSIX signal delivery; accept and no-op. */
struct sigaction;
int sigaction(int, const struct sigaction *, struct sigaction *) {
	return 0;
}

/* VitaGL calls the wrapper name; the SDK/db only exports the raw NID. Bridge. */
struct SceAppMgrBudgetInfo;
extern int _sceAppMgrGetBudgetInfo(struct SceAppMgrBudgetInfo *info);
int sceAppMgrGetBudgetInfo(struct SceAppMgrBudgetInfo *info) {
	return _sceAppMgrGetBudgetInfo(info);
}
