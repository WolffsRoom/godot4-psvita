/*
 * Godot 4.x Vita loader
 * ---------------------
 * The engine is built as a PIC shared object (libgodot.so) to sidestep the
 * vita-elf-create `-q`/--emit-relocs "invalid operation" that a 31 MB executable
 * hits (Thumb branch veneers). This small loader IS a normal Vita executable
 * (small enough that -q works), links all the libraries the engine imports, and
 * at runtime: loads libgodot.so, relocates it, resolves its 645 imports against
 * default_dynlib[], then calls the engine's main().
 *
 * Requires the kubridge kernel plugin (kubridge.skprx) installed on the console
 * (config.txt *KERNEL) — it provides kuKernelAllocMemBlock for executable memory.
 */

#include <vitasdk.h>
#include <kubridge.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "diag.h"
#include "main.h"
#include "so_util.h"

// newlib heap. Kept moderate: so_load needs ~46 MB for the .so file buffer plus
// ~46 MB for its RX/RW segments; too big a heap starves those and so_load fails.
int _newlib_heap_size_user = 128 * 1024 * 1024;

// Main-thread stack. 2 MB still overflowed during static init (libstdc++ locale
// init appears to recurse). 8 MB here is a DIAGNOSTIC: if it still overflows the
// recursion is unbounded (needs a real fix); if it completes it was deep-finite.
unsigned int sceUserMainThreadStackSize = 8 * 1024 * 1024;

extern so_default_dynlib default_dynlib[];
extern int default_dynlib_size;

static so_module godot_mod;

static void log_line(const char *s) {
	sceClibPrintf("%s", s);
	FILE *f = fopen("ux0:data/godot_loader.log", "a");
	if (f) {
		fputs(s, f);
		fclose(f);
	}
}

int debugPrintf(const char *fmt, ...) {
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	log_line(buf);
	return 0;
}

void fatal_error(const char *fmt, ...) {
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	log_line("FATAL: ");
	log_line(buf);
	log_line("\n");
	sceKernelExitProcess(0);
	while (1) {
	}
}

int main(int argc, char *argv[]) {
	diag("[main] reached main()\n");

	// Fresh log per run.
	sceIoRemove("ux0:data/godot_loader.log");

	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	// Build stamp — lets us confirm from godot_loader.log which eboot actually
	// ran (VitaGL instrumentation + fixed log path live in this build).
	debugPrintf("Godot Vita loader BUILD=DIAG3 (vitaGL: FILE_LOG+SHARK+GLSL_TRANSLATOR, log=ux0:data/vitaGL.log)\n");
	debugPrintf("Godot Vita loader: loading app0:libgodot.so\n");

	if (so_load(&godot_mod, "app0:libgodot.so", LOAD_ADDRESS) < 0) {
		fatal_error("so_load failed (is libgodot.so in the VPK?)");
	}
	debugPrintf("so_load OK, relocating...\n");
	so_relocate(&godot_mod);

	debugPrintf("resolving %d imports...\n", (int)(default_dynlib_size / sizeof(so_default_dynlib)));
	if (so_resolve(&godot_mod, default_dynlib, default_dynlib_size, 0) < 0) {
		fatal_error("so_resolve failed: an import is missing from default_dynlib");
	}

	so_flush_caches(&godot_mod);
	so_initialize(&godot_mod);

	int (*godot_main)(int, char **) = (void *)so_symbol(&godot_mod, "main");
	if (!godot_main) {
		fatal_error("entry 'main' not found in libgodot.so");
	}

	debugPrintf("calling engine main()...\n");
	godot_main(argc, argv);

	debugPrintf("engine main() returned; exiting.\n");
	sceKernelExitProcess(0);
	return 0;
}
