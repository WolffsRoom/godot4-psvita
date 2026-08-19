/**************************************************************************/
/*  audio_driver_vita.h                                                   */
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

#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "servers/audio/audio_driver.h"

#include <psp2/audioout.h>

// Ports 1:1 from the 3.x AudioDriverVita. The 4.x AudioDriver interface is
// almost identical (get_speaker_mode() -> get_output_channels() naming aside).
class AudioDriverVita : public AudioDriver {
	Thread thread;
	Mutex mutex;

	int32_t *samples_in = nullptr;
	int16_t *samples_out = nullptr;
	int port = -1;

	unsigned int buffer_size = 0;
	unsigned int mix_rate = 48000;
	SpeakerMode speaker_mode = SPEAKER_MODE_STEREO;
	int channels = 2;

	bool active = false;
	bool thread_exited = false;
	mutable bool exit_thread = false;

	static void thread_func(void *p_udata);

public:
	virtual const char *get_name() const override { return "Vita"; }

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override { return mix_rate; }
	virtual SpeakerMode get_speaker_mode() const override { return speaker_mode; }
	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	AudioDriverVita() {}
	~AudioDriverVita() {}
};
