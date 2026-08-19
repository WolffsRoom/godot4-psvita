/*
 * Startup diagnostic markers.
 *
 * The first boot crashed BEFORE main() with no log — i.e. during C++ static
 * constructors. These markers write to ux0:data/godot_diag.log using RAW sceIo
 * (no malloc, so they work even before the heap is up) from the earliest and
 * latest constructor priorities, plus main() calls diag() first thing.
 *
 * Interpreting the resulting log:
 *   (empty)                 -> Sce imports themselves are broken (even sceIo
 *                              fails) -> toolchain/-q or FSELF import table.
 *   "[ctor first]" only     -> a later library constructor crashed (allocating
 *                              before the heap was initialized — init order).
 *   "[ctor first]"+"[last]" -> all constructors ran; crash is in/after main.
 *   ...+"[main]"            -> reached main(); crash is in so_load or later.
 */

#include <psp2/io/fcntl.h>

#include "diag.h"

static unsigned slen(const char *s) {
	unsigned n = 0;
	while (s[n]) {
		n++;
	}
	return n;
}

void diag(const char *s) {
	SceUID fd = sceIoOpen("ux0:data/godot_diag.log",
			SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, s, slen(s));
		sceIoClose(fd);
	}
}

__attribute__((constructor(101))) static void diag_first(void) {
	sceIoRemove("ux0:data/godot_diag.log");
	diag("[ctor first] earliest constructor ran (Sce imports work)\n");
}

__attribute__((constructor(65535))) static void diag_last(void) {
	diag("[ctor last] all static constructors completed\n");
}
