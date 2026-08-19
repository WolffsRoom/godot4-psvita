/**************************************************************************/
/*  gl_manager_vita.cpp                                                   */
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

#include "gl_manager_vita.h"

#ifdef GLES3_ENABLED

#ifdef VITAGL
#include <vitaGL.h>
#endif

#include <psp2/gxm.h>

Error GLManagerVita::initialize() {
	if (initialized) {
		return OK;
	}

#ifdef VITAGL
	// Same tuning the 3.x port used. VGL_MODE_SHADER_PAIR avoids a SceLibKernel
	// crash seen with VGL_MODE_POSTPONED.
	vglSetSemanticBindingMode(VGL_MODE_SHADER_PAIR);
	vglSetParamBufferSize(4 * 1024 * 1024);
	vglUseTripleBuffering(GL_FALSE);
	vglInitWithCustomThreshold(0, VITA_SCREEN_WIDTH, VITA_SCREEN_HEIGHT,
			4 * 1024 * 1024, 0, 0, 0, SCE_GXM_MULTISAMPLE_4X);
	initialized = true;
	return OK;
#else
	// TODO: PVR_PSP2 EGL bring-up (libgpu_es4_ext + libIMGEGL). Not GLES3-capable;
	// present only for completeness.
	return ERR_UNAVAILABLE;
#endif
}

void GLManagerVita::release() {
	if (!initialized) {
		return;
	}
	// This VitaGL build exposes no public teardown (no vglEnd/vglTerm);
	// the context lives for the whole process. Nothing to release here.
	initialized = false;
}

void GLManagerVita::swap_buffers() {
#ifdef VITAGL
	vglSwapBuffers(GL_FALSE);
#endif
}

void GLManagerVita::make_current() {
	// VitaGL has a single implicit context; nothing to bind. Present so the
	// DisplayServer/renderer call sites match the desktop GLManager API.
}

void GLManagerVita::set_use_vsync(bool p_use) {
	use_vsync = p_use;
	// TODO: map to vglWaitVblankStart(p_use). VitaGL exposes this globally.
}

#endif // GLES3_ENABLED
