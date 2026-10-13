/**************************************************************************/
/*  audio_driver_vita.cpp                                                 */
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

#include "audio_driver_vita.h"

#include "core/os/os.h"

#include <psp2/kernel/threadmgr.h>

// Hardware audio is block-based. Low block sizes are sensitive to the
// occasional slow frame and IO stall on the Vita, causing audible crackling.
// Keep the PCM mixer stereo at 48 kHz (required by MAIN), with some buffering
// headroom and a higher-priority audio worker for stable output cadence.

Error AudioDriverVita::init() {
	mix_rate = 48000;
	speaker_mode = SPEAKER_MODE_STEREO;
	channels = 2;

	// 2048 samples = ~42.7 ms at 48 kHz, up from ~21.3 ms.
	// Main port accepts multiples of 64 up to SCE_AUDIO_MAX_LEN.
	buffer_size = 2048;

	samples_in = memnew_arr(int32_t, buffer_size * channels);
	samples_out = memnew_arr(int16_t, buffer_size * channels);

	port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, buffer_size, mix_rate, SCE_AUDIO_OUT_MODE_STEREO);
	ERR_FAIL_COND_V_MSG(port < 0, ERR_CANT_OPEN, "sceAudioOutOpenPort failed.");

	return OK;
}

void AudioDriverVita::start() {
	active = true;
	Thread::Settings settings;
	settings.priority = Thread::PRIORITY_HIGH;
	thread.start(AudioDriverVita::thread_func, this, settings);
}

void AudioDriverVita::thread_func(void *p_udata) {
	AudioDriverVita *ad = static_cast<AudioDriverVita *>(p_udata);

	while (!ad->exit_thread) {
		ad->lock();
		if (ad->active) {
			ad->audio_server_process(ad->buffer_size, ad->samples_in);
			for (unsigned int i = 0; i < ad->buffer_size * ad->channels; i++) {
				ad->samples_out[i] = ad->samples_in[i] >> 16;
			}
		} else {
			for (unsigned int i = 0; i < ad->buffer_size * ad->channels; i++) {
				ad->samples_out[i] = 0;
			}
		}
		ad->unlock();

		// Hardware output blocks until the current buffer has been consumed.
		// Keep this call outside the mixing lock to avoid holding up the main
		// thread during a slow/busy speaker/headphone output operation.
		sceAudioOutOutput(ad->port, ad->samples_out);
	}
	ad->thread_exited = true;
}

void AudioDriverVita::lock() {
	mutex.lock();
}

void AudioDriverVita::unlock() {
	mutex.unlock();
}

void AudioDriverVita::finish() {
	exit_thread = true;
	if (thread.is_started()) {
		thread.wait_to_finish();
	}
	if (port >= 0) {
		sceAudioOutReleasePort(port);
		port = -1;
	}
	if (samples_in) {
		memdelete_arr(samples_in);
		samples_in = nullptr;
	}
	if (samples_out) {
		memdelete_arr(samples_out);
		samples_out = nullptr;
	}
}
