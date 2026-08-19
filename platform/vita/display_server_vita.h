/**************************************************************************/
/*  display_server_vita.h                                                 */
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

#include "joypad_vita.h"

#include "servers/display/display_server.h"

#ifdef GLES3_ENABLED
#include "gl_manager_vita.h"
#endif

#include <psp2/touch.h>

// In 4.x the DisplayServer enums (WindowID, WindowMode, VSyncMode, Feature,
// Context, MouseMode, CursorShape, ...) live in the DisplayServerEnums
// namespace. Other platforms fully-qualify every reference; for this leaf
// platform header we pull the namespace in so the bare names below resolve to
// the exact same types the base's virtuals use (so overrides still match).
using namespace DisplayServerEnums;

// The Vita is a single-window, fixed-resolution device (960x544, no resize, no
// second screen exposed here). Most DisplayServer capabilities are therefore
// no-ops or fixed returns. The important parts are:
//   * standing up the GL context (GLManagerVita) so the Compatibility renderer
//     can initialize;
//   * pumping SceCtrl / SceTouch / SceMotion into Input every frame.
class DisplayServerVita : public DisplayServer {
	// GDCLASS not used: DisplayServer subclasses are registered manually.

	Point2i mouse_position;
	MouseButtonMask mouse_button_mask;

#ifdef GLES3_ENABLED
	GLManagerVita *gl_manager = nullptr;
#endif

	JoypadVita *joypad = nullptr;

	// Touch state (front panel).
	Vector2 last_touch_pos[SCE_TOUCH_MAX_REPORT];
	Vector2 front_panel_size;
	uint32_t last_touch_count = 0;
	void _process_touch();
	void _process_motion();

	static DisplayServerVita *get_singleton();

public:
	static void register_vita_driver();
	static Vector<String> get_rendering_drivers_func();
	static DisplayServer *create_func(const String &p_rendering_driver, WindowMode p_mode, VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, Context p_context, int64_t p_parent_window, Error &r_error);

	// --- Capabilities --------------------------------------------------------
	virtual bool has_feature(Feature p_feature) const override;
	virtual String get_name() const override { return "Vita"; }

	// --- Event pump (called from OS_Vita::run each frame) --------------------
	virtual void process_events() override;

	// --- Screen / window (all fixed to the single 960x544 surface) -----------
	virtual int get_screen_count() const override { return 1; }
	virtual int get_primary_screen() const override { return 0; }
	virtual Point2i screen_get_position(int p_screen = SCREEN_OF_MAIN_WINDOW) const override { return Point2i(); }
	virtual Size2i screen_get_size(int p_screen = SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = SCREEN_OF_MAIN_WINDOW) const override { return 220; }
	virtual float screen_get_refresh_rate(int p_screen = SCREEN_OF_MAIN_WINDOW) const override { return 60.0f; }

	virtual Vector<WindowID> get_window_list() const override { return { MAIN_WINDOW_ID }; }
	virtual WindowID get_window_at_screen_position(const Point2i &p_position) const override { return MAIN_WINDOW_ID; }

	virtual void window_attach_instance_id(ObjectID p_instance, WindowID p_window = MAIN_WINDOW_ID) override { window_attached_instance_id = p_instance; }
	virtual ObjectID window_get_attached_instance_id(WindowID p_window = MAIN_WINDOW_ID) const override { return window_attached_instance_id; }

	virtual void window_set_rect_changed_callback(const Callable &p_callable, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual void window_set_window_event_callback(const Callable &p_callable, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual void window_set_input_event_callback(const Callable &p_callable, WindowID p_window = MAIN_WINDOW_ID) override { input_event_callback = p_callable; }
	virtual void window_set_input_text_callback(const Callable &p_callable, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual void window_set_drop_files_callback(const Callable &p_callable, WindowID p_window = MAIN_WINDOW_ID) override {}

	virtual void window_set_title(const String &p_title, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual int window_get_current_screen(WindowID p_window = MAIN_WINDOW_ID) const override { return 0; }
	virtual void window_set_current_screen(int p_screen, WindowID p_window = MAIN_WINDOW_ID) override {}

	virtual Point2i window_get_position(WindowID p_window = MAIN_WINDOW_ID) const override { return Point2i(); }
	virtual Point2i window_get_position_with_decorations(WindowID p_window = MAIN_WINDOW_ID) const override { return Point2i(); }
	virtual void window_set_position(const Point2i &p_position, WindowID p_window = MAIN_WINDOW_ID) override {}

	virtual void window_set_transient(WindowID p_window, WindowID p_parent) override {}

	virtual void window_set_max_size(const Size2i p_size, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual Size2i window_get_max_size(WindowID p_window = MAIN_WINDOW_ID) const override;
	virtual void window_set_min_size(const Size2i p_size, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual Size2i window_get_min_size(WindowID p_window = MAIN_WINDOW_ID) const override;
	virtual void window_set_size(const Size2i p_size, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual Size2i window_get_size(WindowID p_window = MAIN_WINDOW_ID) const override;
	virtual Size2i window_get_size_with_decorations(WindowID p_window = MAIN_WINDOW_ID) const override;

	virtual void window_set_mode(WindowMode p_mode, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual WindowMode window_get_mode(WindowID p_window = MAIN_WINDOW_ID) const override { return WINDOW_MODE_FULLSCREEN; }

	virtual bool window_is_maximize_allowed(WindowID p_window = MAIN_WINDOW_ID) const override { return false; }
	virtual void window_set_flag(WindowFlags p_flag, bool p_enabled, WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual bool window_get_flag(WindowFlags p_flag, WindowID p_window = MAIN_WINDOW_ID) const override { return false; }

	virtual void window_request_attention(WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual void window_move_to_foreground(WindowID p_window = MAIN_WINDOW_ID) override {}
	virtual bool window_is_focused(WindowID p_window = MAIN_WINDOW_ID) const override { return true; }

	virtual bool window_can_draw(WindowID p_window = MAIN_WINDOW_ID) const override { return true; }
	virtual bool can_any_window_draw() const override { return true; }

	virtual void window_set_vsync_mode(VSyncMode p_vsync_mode, WindowID p_window = MAIN_WINDOW_ID) override;
	virtual VSyncMode window_get_vsync_mode(WindowID p_window = MAIN_WINDOW_ID) const override;

	// --- GL presentation -----------------------------------------------------
	virtual void swap_buffers() override;

	// --- Mouse / cursor (no hardware mouse; touch emulates it) ---------------
	virtual void mouse_set_mode(MouseMode p_mode) override {}
	virtual MouseMode mouse_get_mode() const override { return MOUSE_MODE_HIDDEN; }
	virtual Point2i mouse_get_position() const override { return mouse_position; }
	virtual BitField<MouseButtonMask> mouse_get_button_state() const override { return mouse_button_mask; }

	// --- Clipboard (unsupported) ---------------------------------------------
	virtual void clipboard_set(const String &p_text) override {}
	virtual String clipboard_get() const override { return String(); }

	// --- Virtual keyboard (SceIme) -------------------------------------------
	virtual bool has_hardware_keyboard() const override { return false; }
	virtual void virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect = Rect2(), VirtualKeyboardType p_type = KEYBOARD_TYPE_DEFAULT, int p_max_length = -1, int p_cursor_start = -1, int p_cursor_end = -1) override;
	virtual void virtual_keyboard_hide() override;
	virtual int virtual_keyboard_get_height() const override;

	// --- Cursor shapes (no-op) -----------------------------------------------
	virtual void cursor_set_shape(CursorShape p_shape) override {}
	virtual CursorShape cursor_get_shape() const override { return CURSOR_ARROW; }
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, CursorShape p_shape = CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override {}

	DisplayServerVita(const String &p_rendering_driver, WindowMode p_mode, VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i &p_resolution, Error &r_error);
	~DisplayServerVita();

private:
	ObjectID window_attached_instance_id;
	Callable input_event_callback;
	VSyncMode vsync_mode = VSYNC_ENABLED;
};
