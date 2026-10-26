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
#include <stdlib.h>
#include <string.h>
#include <vita2d.h>

#ifdef VITA_BOOT_ANIMATION_ENABLED
#include "boot_splash.h"
#endif
#include "diag.h"
#include "main.h"
#include "so_util.h"

// Keep 192 MiB permanently available to Godot. The loader reads the temporary
// .so image inside this heap, then frees it after mapping the RX/RW segments.
// With ATTRIBUTE2=12 and CapUnlocker this still leaves roughly 110 MiB of
// USER_RW before mapping the 46 MiB runtime. Buckshot's main scene exceeded
// the former 160 MiB heap while system and CDRAM pools still had free memory.
int _newlib_heap_size_user = 256 * 1024 * 1024;

// Main-thread stack. 2 MB still overflowed during static init (libstdc++ locale
// init appears to recurse). 8 MB here is a DIAGNOSTIC: if it still overflows the
// recursion is unbounded (needs a real fix); if it completes it was deep-finite.
unsigned int sceUserMainThreadStackSize = 8 * 1024 * 1024;

extern so_default_dynlib default_dynlib[];
extern int default_dynlib_size;

static so_module godot_mod;

enum capunlocker_mode {
	CAPUNLOCKER_DISABLED,
	CAPUNLOCKER_OPTIONAL,
	CAPUNLOCKER_REQUIRED,
};

static enum capunlocker_mode read_capunlocker_mode(void) {
	char mode[32] = { 0 };
	FILE *f = fopen("app0:capunlocker.cfg", "r");
	if (!f) {
		return CAPUNLOCKER_OPTIONAL;
	}
	fgets(mode, sizeof(mode), f);
	fclose(f);
	if (strncmp(mode, "disabled", 8) == 0) return CAPUNLOCKER_DISABLED;
	if (strncmp(mode, "required", 8) == 0) return CAPUNLOCKER_REQUIRED;
	return CAPUNLOCKER_OPTIONAL;
}

static int probe_capunlocker(void) {
	SceUID thread_id = sceKernelGetThreadId();
	int old_mask = sceKernelGetThreadCpuAffinityMask(thread_id);
	int result = sceKernelChangeThreadCpuAffinityMask(thread_id, SCE_KERNEL_CPU_MASK_SYSTEM);
	if (result >= 0 && old_mask >= 0) {
		sceKernelChangeThreadCpuAffinityMask(thread_id, old_mask);
	}
	return result;
}

static void show_capunlocker_required(void) {
	if (vita2d_init() <= 0) return;
	vita2d_pgf *font = vita2d_load_default_pgf();
	if (!font) {
		vita2d_fini();
		return;
	}
	for (int frame = 0; frame < 600; frame++) {
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_pgf_draw_text(font, 52, 150, RGBA8(255, 255, 255, 255), 1.25f, "CapUnlocker necessario");
		vita2d_pgf_draw_text(font, 52, 210, RGBA8(220, 220, 220, 255), 0.85f, "Instale CapUnlocker.skprx em *KERNEL,");
		vita2d_pgf_draw_text(font, 52, 245, RGBA8(220, 220, 220, 255), 0.85f, "reinicie o PS Vita e abra o jogo novamente.");
		vita2d_pgf_draw_text(font, 52, 315, RGBA8(190, 190, 190, 255), 0.72f, "Detalhes: ux0:data/.../logs/godot_loader.log");
		vita2d_end_drawing();
		vita2d_swap_buffers();
	}
	vita2d_wait_rendering_done();
	vita2d_free_pgf(font);
	vita2d_fini();
}

static void log_memory(const char *stage) {
	SceKernelFreeMemorySizeInfo info = { 0 };
	info.size = sizeof(info);
	int result = sceKernelGetFreeMemorySize(&info);
	debugPrintf("[memory] stage=%s result=0x%08x user_free=%u cdram_free=%u phycont_free=%u heap=%u\n",
			stage, (unsigned int)result, info.size_user, info.size_cdram,
			info.size_phycont, (unsigned int)_newlib_heap_size_user);
}

static void log_line(const char *s) {
	sceClibPrintf("%s", s);
	FILE *f = fopen(VITA_LOG_ROOT "/logs/godot_loader.log", "a");
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
    sceIoMkdir(VITA_LOG_ROOT, 0777);
    sceIoMkdir(VITA_LOG_ROOT "/logs", 0777);
	diag("[main] reached main()\n");

	// Fresh log per run.
	sceIoRemove(VITA_LOG_ROOT "/logs/godot_loader.log");

	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	enum capunlocker_mode cap_mode = read_capunlocker_mode();
	int cap_result = cap_mode == CAPUNLOCKER_DISABLED ? -1 : probe_capunlocker();
	int cap_available = cap_mode != CAPUNLOCKER_DISABLED && cap_result >= 0;
	setenv("GODOT_VITA_CAPUNLOCKER", cap_available ? "1" : "0", 1);
	debugPrintf("[capunlocker] mode=%s available=%d probe_rc=0x%08x audio_cpu3=%d\n",
			cap_mode == CAPUNLOCKER_REQUIRED ? "required" : (cap_mode == CAPUNLOCKER_OPTIONAL ? "optional" : "disabled"),
			cap_available, (unsigned int)cap_result, cap_available);
	if (cap_mode == CAPUNLOCKER_REQUIRED && !cap_available) {
		show_capunlocker_required();
		fatal_error("CapUnlocker required but CPU3 affinity was denied (0x%08x). Install CapUnlocker.skprx under *KERNEL and reboot.", (unsigned int)cap_result);
	}

	// Build stamp — lets us confirm from godot_loader.log which eboot actually
	// ran (VitaGL instrumentation + fixed log path live in this build).
	debugPrintf("BUILD_ID: %s\n", VITA_RUNTIME_BUILD_ID);
	debugPrintf("RUNTIME_SHA256: %s\n", VITA_RUNTIME_SHA256);
	debugPrintf("VITAGL_SHA256: %s\n", VITA_GL_SHA256);
	debugPrintf("Godot Vita loader: loading app0:libgodot.so\n");
	log_memory("before_so_load");
#ifdef VITA_BOOT_ANIMATION_ENABLED
	/* Optional native boot animation, played while loading and relocating.
	 * Its Vita2D context is released before the Godot renderer starts. */
	int boot_started = dodge_boot_splash_start();
	debugPrintf("[boot-anim] enabled=%d\n", boot_started);
#endif

	if (so_load(&godot_mod, "app0:libgodot.so", LOAD_ADDRESS) < 0) {
#ifdef VITA_BOOT_ANIMATION_ENABLED
		dodge_boot_splash_stop();
#endif
		fatal_error("so_load failed (is libgodot.so in the VPK?)");
	}
	debugPrintf("so_load OK, relocating...\n");
	log_memory("after_so_load");
	so_relocate(&godot_mod);

	debugPrintf("resolving %d imports...\n", (int)(default_dynlib_size / sizeof(so_default_dynlib)));
	if (so_resolve(&godot_mod, default_dynlib, default_dynlib_size, 0) < 0) {
#ifdef VITA_BOOT_ANIMATION_ENABLED
		dodge_boot_splash_stop();
#endif
		fatal_error("so_resolve failed: an import is missing from default_dynlib");
	}

#ifdef VITA_BOOT_ANIMATION_ENABLED
	/* Play at least one six-second cycle, then tear down Vita2D before
	 * the engine initializes its native GXM resources. */
	dodge_boot_splash_stop();
	debugPrintf("[boot-anim] handoff to engine\n");
#endif
	so_flush_caches(&godot_mod);
	so_initialize(&godot_mod);

	int (*godot_main)(int, char **) = (void *)so_symbol(&godot_mod, "main");
	if (!godot_main) {
		fatal_error("entry 'main' not found in libgodot.so");
	}

	debugPrintf("calling engine main()...\n");
	log_memory("before_engine_main");
	godot_main(argc, argv);

	debugPrintf("engine main() returned; exiting.\n");
	sceKernelExitProcess(0);
	return 0;
}
