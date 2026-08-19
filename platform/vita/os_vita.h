/**************************************************************************/
/*  os_vita.h                                                             */
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

#pragma once

#include "drivers/unix/os_unix.h"

#include "audio_driver_vita.h"

class MainLoop;

// In Godot 4.x the OS is split from the windowing/input layer (DisplayServer).
// OS_Vita therefore only covers: process lifetime, paths, time, dynamic libs,
// and driving the main loop. Windowing, GL swap and input live in
// DisplayServerVita. We inherit the POSIX backends from OS_Unix (VitaSDK's
// newlib is POSIX-ish enough for file/dir/thread/time).
class OS_Vita : public OS_Unix {
	MainLoop *main_loop = nullptr;
	AudioDriverVita audio_driver;

	bool force_quit = false;

protected:
	virtual void initialize() override;
	virtual void initialize_core() override;
	virtual void finalize() override;
	virtual void finalize_core() override;

	virtual void set_main_loop(MainLoop *p_main_loop) override;
	virtual void delete_main_loop() override;

	virtual void initialize_joypads() override {} // Handled in DisplayServerVita.

public:
	OS_Vita();
	~OS_Vita();

	static OS_Vita *get_singleton();

	// --- Main loop -----------------------------------------------------------
	virtual MainLoop *get_main_loop() const override;
	void run();

	// --- Identity ------------------------------------------------------------
	virtual String get_name() const override { return "Vita"; }
	virtual String get_distribution_name() const override { return "PlayStation Vita"; }
	virtual String get_version() const override { return ""; }
	virtual String get_model_name() const override { return "Sony PlayStation Vita"; }

	// --- Filesystem paths (ux0:/) --------------------------------------------
	virtual String get_data_path() const override { return "ux0:/data"; }
	virtual String get_config_path() const override { return "ux0:/data"; }
	virtual String get_cache_path() const override { return "ux0:/data"; }
	virtual String get_user_data_dir(const String &p_user_dir) const override;

	// --- Dynamic libraries (dlopen via VitaSDK) ------------------------------
	virtual Error open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data = nullptr) override;
	virtual Error close_dynamic_library(void *p_library_handle) override;
	virtual Error get_dynamic_library_symbol_handle(void *p_library_handle, const String &p_name, void *&p_symbol_handle, bool p_optional = false) override;

	// --- Misc ----------------------------------------------------------------
	virtual Error shell_open(const String &p_uri) override { return ERR_UNAVAILABLE; }
	virtual void alert(const String &p_alert, const String &p_title = "ALERT!") override;
	virtual bool _check_internal_feature_support(const String &p_feature) override;

	// Only pure-virtual left unimplemented by OS_Unix on this platform.
	virtual Error get_entropy(uint8_t *r_buffer, int p_bytes) override;

	// TODO(4.x port): if OS_Unix's execute()/kill() don't link against VitaSDK,
	// override them here to return ERR_UNAVAILABLE (no process spawning on Vita).
};
