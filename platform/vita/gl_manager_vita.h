/**************************************************************************/
/*  gl_manager_vita.h                                                     */
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

#ifdef GLES3_ENABLED

#include "core/error/error_list.h"

// Vita has a single, fixed 960x544 framebuffer.
#define VITA_SCREEN_WIDTH 960
#define VITA_SCREEN_HEIGHT 544

// Thin wrapper around VitaGL's context. In Godot 4.x the Compatibility renderer
// (drivers/gles3) expects the platform to own the GL context and expose a
// swap_buffers() + make_current(). VitaGL manages GXM itself, so there is no
// EGL display/surface to track when built with VITAGL.
//
// This replaces the 3.x ContextEGL_Vita. If a PVR_PSP2 EGL path is ever wanted,
// add the eglInitialize/eglCreateContext bookkeeping behind #ifndef VITAGL like
// the 3.x port did.
class GLManagerVita {
	bool initialized = false;

public:
	Error initialize();
	void release();

	void swap_buffers();
	void make_current();

	int get_window_width() const { return VITA_SCREEN_WIDTH; }
	int get_window_height() const { return VITA_SCREEN_HEIGHT; }

	// vsync: VitaGL swaps on the GXM display queue; interval maps to
	// vglWaitVblankStart(). Kept for DisplayServer::set_vsync_mode parity.
	void set_use_vsync(bool p_use);
	bool is_using_vsync() const { return use_vsync; }

	GLManagerVita() {}
	~GLManagerVita() { release(); }

private:
	bool use_vsync = true;
};

#endif // GLES3_ENABLED
