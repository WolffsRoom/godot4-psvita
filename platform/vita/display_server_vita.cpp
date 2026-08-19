/**************************************************************************/
/*  display_server_vita.cpp                                               */
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

#include "display_server_vita.h"

#include "core/input/input.h"
#include "core/os/os.h"

#ifdef GLES3_ENABLED
#include "drivers/gles3/rasterizer_gles3.h"
#endif

#include <psp2/motion.h>

static const Size2i VITA_RESOLUTION = Size2i(960, 544);

DisplayServerVita *DisplayServerVita::get_singleton() {
	return static_cast<DisplayServerVita *>(DisplayServer::get_singleton());
}

void DisplayServerVita::register_vita_driver() {
	register_create_function("vita", create_func, get_rendering_drivers_func);
}

Vector<String> DisplayServerVita::get_rendering_drivers_func() {
	Vector<String> drivers;
#ifdef GLES3_ENABLED
	drivers.push_back("opengl3");
#endif
	return drivers;
}

DisplayServer *DisplayServerVita::create_func(const String &p_rendering_driver, WindowMode p_mode, VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, Context p_context, int64_t p_parent_window, Error &r_error) {
	DisplayServer *ds = memnew(DisplayServerVita(p_rendering_driver, p_mode, p_vsync_mode, p_flags, VITA_RESOLUTION, r_error));
	if (r_error != OK) {
		OS::get_singleton()->alert(
				"Your video card driver does not support the selected renderer.\n"
				"On Vita only the OpenGL Compatibility renderer (backed by VitaGL) is available.",
				"Unable to initialize video driver");
	}
	return ds;
}

DisplayServerVita::DisplayServerVita(const String &p_rendering_driver, WindowMode p_mode, VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i &p_resolution, Error &r_error) {
	r_error = ERR_UNAVAILABLE;

#ifdef GLES3_ENABLED
	if (p_rendering_driver == "opengl3") {
		gl_manager = memnew(GLManagerVita);
		if (gl_manager->initialize() != OK) {
			memdelete(gl_manager);
			gl_manager = nullptr;
			ERR_FAIL_MSG("Failed to initialize VitaGL context.");
		}

		// Hand the live context to the Compatibility rasterizer.
		RasterizerGLES3::make_current(false);
		r_error = OK;
	}
#endif

	// Touch panel geometry (front panel is the input surface Godot uses).
	SceTouchPanelInfo info;
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &info);
	front_panel_size = Vector2(info.maxAaX, info.maxAaY);

	// Motion / gyro.
	sceMotionStartSampling();

	joypad = memnew(JoypadVita(Input::get_singleton()));

	Input::get_singleton()->set_emulate_mouse_from_touch(true);
}

DisplayServerVita::~DisplayServerVita() {
	if (joypad) {
		memdelete(joypad);
	}
#ifdef GLES3_ENABLED
	if (gl_manager) {
		memdelete(gl_manager);
	}
#endif
}

bool DisplayServerVita::has_feature(Feature p_feature) const {
	switch (p_feature) {
		case FEATURE_TOUCHSCREEN:
		case FEATURE_VIRTUAL_KEYBOARD:
			return true;
		default:
			return false;
	}
}

// --- Frame ------------------------------------------------------------------

void DisplayServerVita::process_events() {
	if (joypad) {
		joypad->process_joypads();
	}
	_process_touch();
	_process_motion();
	Input::get_singleton()->flush_buffered_events();
}

void DisplayServerVita::swap_buffers() {
#ifdef GLES3_ENABLED
	if (gl_manager) {
		gl_manager->swap_buffers();
	}
#endif
}

// --- Touch / motion ---------------------------------------------------------

void DisplayServerVita::_process_touch() {
	SceTouchData touch;
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

	// (Same index/drag bookkeeping as the 3.x port; ported to 4.x InputEvent API.)
	// TODO: port the new-touch / lost-touch / drag logic from the 3.x os_vita.cpp
	// process_touch(), replacing InputEventScreenTouch->instance() with
	// Ref<InputEventScreenTouch> ev; ev.instantiate(); and parse via
	// Input::get_singleton()->parse_input_event(ev);
	(void)touch;
}

void DisplayServerVita::_process_motion() {
	SceMotionState motion_state;
	if (sceMotionGetState(&motion_state) < 0) {
		return;
	}
	Input *in = Input::get_singleton();
	in->set_accelerometer(Vector3(motion_state.acceleration.x, motion_state.acceleration.y, motion_state.acceleration.z));
	in->set_gyroscope(Vector3(motion_state.angularVelocity.x, motion_state.angularVelocity.y, motion_state.angularVelocity.z));
	in->set_gravity(Vector3(motion_state.basicOrientation.x, motion_state.basicOrientation.y, motion_state.basicOrientation.z));
}

// --- Screen / window sizing (fixed) -----------------------------------------

Size2i DisplayServerVita::screen_get_size(int p_screen) const { return VITA_RESOLUTION; }
Rect2i DisplayServerVita::screen_get_usable_rect(int p_screen) const { return Rect2i(Point2i(), VITA_RESOLUTION); }
Size2i DisplayServerVita::window_get_size(WindowID p_window) const { return VITA_RESOLUTION; }
Size2i DisplayServerVita::window_get_size_with_decorations(WindowID p_window) const { return VITA_RESOLUTION; }
Size2i DisplayServerVita::window_get_max_size(WindowID p_window) const { return VITA_RESOLUTION; }
Size2i DisplayServerVita::window_get_min_size(WindowID p_window) const { return VITA_RESOLUTION; }

// --- VSync ------------------------------------------------------------------

void DisplayServerVita::window_set_vsync_mode(VSyncMode p_vsync_mode, WindowID p_window) {
	vsync_mode = p_vsync_mode;
#ifdef GLES3_ENABLED
	if (gl_manager) {
		gl_manager->set_use_vsync(p_vsync_mode != VSYNC_DISABLED);
	}
#endif
}

DisplayServerEnums::VSyncMode DisplayServerVita::window_get_vsync_mode(WindowID p_window) const {
	return vsync_mode;
}

// --- Virtual keyboard (SceIme) ----------------------------------------------

void DisplayServerVita::virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect, VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end) {
	// TODO: port the SceIme setup from the 3.x os_vita.cpp show_virtual_keyboard()
	// and route SCE_IME_EVENT_* back through Input as key events.
}

void DisplayServerVita::virtual_keyboard_hide() {
	// TODO: sceImeClose().
}

int DisplayServerVita::virtual_keyboard_get_height() const {
	return 0; // TODO: return the SceIme reported height.
}
