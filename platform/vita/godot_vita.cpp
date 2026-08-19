/**************************************************************************/
/*  godot_vita.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "main/main.h"
#include "os_vita.h"

#include <stdio.h>

#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/sysmodule.h>

// Raw sceIo bring-up log (no dependency on the engine's own logging), so we can
// see how far Main::setup/start got even without a game pack.
static void vlog(const char *s) {
	SceUID fd = sceIoOpen("ux0:data/godot_engine.log", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		unsigned n = 0;
		while (s[n]) {
			n++;
		}
		sceIoWrite(fd, s, n);
		sceIoClose(fd);
	}
}

#ifdef VITAGL
#include <vitaGL.h>
// VitaGL needs a large newlib heap for GXM buffers + textures.
int _newlib_heap_size_user = 256 * 1024 * 1024;
#endif

int main(int argc, char *argv[]) {
	// Bump clocks to the shipping-title maximum. The Vita is a 512 MB / SGX543
	// device; every MHz matters for a modern engine.
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	OS_Vita os;

	// Godot 4.x: launch straight into the packaged project. On device the game
	// data + PCK live inside the VPK at app0:.
	// TODO: replace hard-coded args with real CLI parsing once bootstrapping works.
	const char *args[] = {
		"--path", "app0:/game_data",
		"--rendering-driver", "opengl3",
		"--rendering-method", "gl_compatibility",
		"--verbose",
	};

	sceIoRemove("ux0:data/godot_engine.log");
	// Capture Godot's own stdout/stderr (with --verbose) so we can see exactly
	// why Main::setup succeeds or fails.
	freopen("ux0:data/godot_stdout.log", "w", stdout);
	freopen("ux0:data/godot_stderr.log", "w", stderr);
	vlog("engine: before Main::setup\n");
	Error err = Main::setup("godot", sizeof(args) / sizeof(args[0]), (char **)args);
	fflush(stdout);
	fflush(stderr);
	if (err != OK) {
		vlog("engine: Main::setup FAILED — see godot_stderr.log for the reason\n");
		return 255;
	}
	vlog("engine: Main::setup OK (servers + renderer up)\n");

	// Godot 4.8: Main::start() returns an int (EXIT_SUCCESS / EXIT_FAILURE), NOT
	// a bool. EXIT_SUCCESS (== 0) means "started, run the main loop". Assigning it
	// to a bool inverts the meaning (0 -> false), which silently skipped os.run().
	int start_ret = Main::start();
	fflush(stdout);
	fflush(stderr);
	bool started = (start_ret == EXIT_SUCCESS);
	vlog(started ? "engine: Main::start OK -> running main loop\n" : "engine: Main::start FAILED\n");
	if (started) {
		os.run(); // The OS drives the main loop.
	}
	vlog("engine: main loop ended, cleanup\n");
	Main::cleanup();
	fflush(stdout);
	fflush(stderr);

	sceKernelExitProcess(0);
	return 0;
}
