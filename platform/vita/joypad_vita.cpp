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
#include <stdio.h>

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
	int mode_rc = sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	// Vita provides SDL/Godot-standard button indices directly. Avoid remapping
	// them through an unrelated controller's SDL mapping database.
	Dictionary joy_info;
	joy_info["mapping_handled"] = true;
	input->joy_connection_changed(0, true, "PlayStation Vita", "VITA-NATIVE-CTRL", joy_info);
	fprintf(stderr, "[vita-pad] init driver=1 mode_rc=%d device=0 native_mapping=1\n", mode_rc);
	fflush(stderr);
}

JoypadVita::~JoypadVita() {}

void JoypadVita::process_joypads() {
	const int samples = sceCtrlPeekBufferPositive(0, &pad, 1);
	static unsigned int frames = 0;
	static unsigned int previous_buttons = 0;
	static unsigned char previous_lx = 128;
	static unsigned char previous_ly = 128;
	static bool first_sample = true;

	if (samples <= 0) {
		if ((++frames % 300) == 1) {
			fprintf(stderr, "[vita-pad] read samples=%d (no valid input sample)\n", samples);
			fflush(stderr);
		}
		return;
	}
	frames++;
	if (first_sample || pad.buttons != previous_buttons ||
		((frames % 300) == 0) ||
		((int)pad.lx - (int)previous_lx > 32) || ((int)previous_lx - (int)pad.lx > 32) ||
		((int)pad.ly - (int)previous_ly > 32) || ((int)previous_ly - (int)pad.ly > 32)) {
		fprintf(stderr, "[vita-pad] sample=%d buttons=0x%08x lx=%u ly=%u rx=%u ry=%u frame=%u\n",
				samples, pad.buttons, pad.lx, pad.ly, pad.rx, pad.ry, frames);
		fflush(stderr);
	}
	first_sample = false;
	previous_buttons = pad.buttons;
	previous_lx = pad.lx;
	previous_ly = pad.ly;
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
