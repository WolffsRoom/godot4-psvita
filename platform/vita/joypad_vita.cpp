/**************************************************************************/
/*  joypad_vita.cpp                                                       */
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

#include "joypad_vita.h"

struct VitaButtonMap {
	unsigned int sce_button;
	JoyButton godot_button;
};

// Vita face + shoulder + d-pad -> Godot's SDL-style JoyButton layout.
static const VitaButtonMap button_map[] = {
	{ SCE_CTRL_CROSS, JoyButton::A },
	{ SCE_CTRL_CIRCLE, JoyButton::B },
	{ SCE_CTRL_SQUARE, JoyButton::X },
	{ SCE_CTRL_TRIANGLE, JoyButton::Y },
	{ SCE_CTRL_SELECT, JoyButton::BACK },
	{ SCE_CTRL_START, JoyButton::START },
	{ SCE_CTRL_LTRIGGER, JoyButton::LEFT_SHOULDER },
	{ SCE_CTRL_RTRIGGER, JoyButton::RIGHT_SHOULDER },
	{ SCE_CTRL_UP, JoyButton::DPAD_UP },
	{ SCE_CTRL_DOWN, JoyButton::DPAD_DOWN },
	{ SCE_CTRL_LEFT, JoyButton::DPAD_LEFT },
	{ SCE_CTRL_RIGHT, JoyButton::DPAD_RIGHT },
};

JoypadVita::JoypadVita(Input *p_input) :
		input(p_input) {
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	// Godot sees a single, always-connected gamepad on device 0.
	input->joy_connection_changed(0, true, "PlayStation Vita", "");
}

JoypadVita::~JoypadVita() {}

void JoypadVita::process_joypads() {
	if (sceCtrlPeekBufferPositive(0, &pad, 1) < 0) {
		return;
	}

	for (const VitaButtonMap &b : button_map) {
		input->joy_button(0, b.godot_button, (pad.buttons & b.sce_button) != 0);
	}

	// Analog sticks: SceCtrl reports 0..255, centered at 128. Normalize to -1..1.
	auto norm = [](unsigned char v) -> float {
		return (float(v) - 128.0f) / 128.0f;
	};
	input->joy_axis(0, JoyAxis::LEFT_X, norm(pad.lx));
	input->joy_axis(0, JoyAxis::LEFT_Y, norm(pad.ly));
	input->joy_axis(0, JoyAxis::RIGHT_X, norm(pad.rx));
	input->joy_axis(0, JoyAxis::RIGHT_Y, norm(pad.ry));
}
