/**************************************************************************/
/*  os_vita.cpp                                                           */
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

#include "os_vita.h"

#include "display_server_vita.h"

#include "core/config/project_settings.h"
#include "core/os/main_loop.h"
#include "main/main.h"
#include "servers/audio/audio_server.h"

#include <dlfcn.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/rng.h>
#include <psp2/sysmodule.h>

OS_Vita *OS_Vita::get_singleton() {
	return static_cast<OS_Vita *>(OS::get_singleton());
}

OS_Vita::OS_Vita() {
	// Register the audio driver so AudioDriverManager can pick it up.
	AudioDriverManager::add_driver(&audio_driver);
}

OS_Vita::~OS_Vita() {}

void OS_Vita::initialize_core() {
	OS_Unix::initialize_core();
	// TODO: load Sce system modules needed early (net, ime) here if not done in main().
}

void OS_Vita::initialize() {
	// Register the core file/dir/net backends (FileAccessUnix, DirAccessUnix,
	// IP, ...). Desktop platforms do exactly this in OS::initialize(); without it
	// FileAccess::create returns null and the engine crashes opening project.godot.
	OS_Unix::initialize_core();

	// Register our DisplayServer so Main::setup2() can instantiate it via the
	// "opengl3" rendering driver we passed on the command line.
	DisplayServerVita::register_vita_driver();
}

void OS_Vita::finalize() {
	if (main_loop) {
		delete_main_loop();
	}
}

void OS_Vita::finalize_core() {
	OS_Unix::finalize_core();
}

void OS_Vita::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
}

void OS_Vita::delete_main_loop() {
	if (main_loop) {
		memdelete(main_loop);
	}
	main_loop = nullptr;
}

MainLoop *OS_Vita::get_main_loop() const {
	return main_loop;
}

void OS_Vita::run() {
	if (!main_loop) {
		return;
	}

	main_loop->initialize();

	while (!force_quit) {
		// DisplayServerVita::process_events() pumps SceCtrl / SceTouch / SceMotion
		// and feeds Input. It must run before Main::iteration().
		DisplayServer::get_singleton()->process_events();

		if (Main::iteration()) {
			break; // Engine requested quit.
		}
	}

	main_loop->finalize();
}

String OS_Vita::get_user_data_dir(const String &p_user_dir) const {
	// Mirror the 3.x behavior: ux0:/data/<godot-dir>/app_userdata/<name>.
	String base = get_data_path();
	if (!p_user_dir.is_empty()) {
		return base.path_join(p_user_dir);
	}
	return base.path_join("godot").path_join("app_userdata").path_join("__unknown");
}

// --- Dynamic libraries -------------------------------------------------------

Error OS_Vita::open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data) {
	p_library_handle = dlopen(p_path.utf8().get_data(), RTLD_NOW);
	if (!p_library_handle) {
		ERR_FAIL_V_MSG(ERR_CANT_OPEN, "Can't open dynamic library: " + p_path + ". Error: " + dlerror());
	}
	if (p_data && p_data->r_resolved_path) {
		*p_data->r_resolved_path = p_path;
	}
	return OK;
}

Error OS_Vita::close_dynamic_library(void *p_library_handle) {
	return dlclose(p_library_handle) == 0 ? OK : FAILED;
}

Error OS_Vita::get_dynamic_library_symbol_handle(void *p_library_handle, const String &p_name, void *&p_symbol_handle, bool p_optional) {
	const char *error;
	dlerror(); // Clear.
	p_symbol_handle = dlsym(p_library_handle, p_name.utf8().get_data());
	error = dlerror();
	if (error != nullptr) {
		ERR_FAIL_COND_V_MSG(!p_optional, ERR_CANT_RESOLVE, "Can't resolve symbol " + p_name + ". Error: " + error);
		return ERR_CANT_RESOLVE;
	}
	return OK;
}

void OS_Vita::alert(const String &p_alert, const String &p_title) {
	// No message box on device; log to the console (accessible via PrincessLog / udcd).
	sceClibPrintf("[%s] %s\n", p_title.utf8().get_data(), p_alert.utf8().get_data());
}

bool OS_Vita::_check_internal_feature_support(const String &p_feature) {
	return p_feature == "mobile" || p_feature == "vita" || p_feature == "etc2";
}

Error OS_Vita::get_entropy(uint8_t *r_buffer, int p_bytes) {
	// sceKernelGetRandomNumber caps at 64 bytes per call; loop for larger asks.
	int off = 0;
	while (off < p_bytes) {
		int chunk = MIN(p_bytes - off, 64);
		if (sceKernelGetRandomNumber(r_buffer + off, chunk) < 0) {
			return FAILED;
		}
		off += chunk;
	}
	return OK;
}
