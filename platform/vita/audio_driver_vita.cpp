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
#include "audio_pcm_vita.h"

#include "core/os/os.h"

#include <psp2/kernel/threadmgr.h>
#include <stdio.h>

// The native Vita mixer delivers signed 16.16 PCM from Godot. Compare with
// DeltaruneVita's hardware path: it uses the Vita BGM port for soundtrack
// playback, explicitly sets per-channel hardware gain and queues PCM16 buffers.
// Keep Godot's own mixer (required for AudioServer and its bus effects), but
// use a BGM output route with conservative hardware gain and explicit PCM16
// conversion. Hardware gain is separate from project/master-bus volume.

Error AudioDriverVita::init() {
	mix_rate = 48000;
	speaker_mode = SPEAKER_MODE_STEREO;
	channels = 2;

	// 2048 samples = ~42.7 ms at 48 kHz, up from ~21.3 ms.
	// Main port accepts multiples of 64 up to SCE_AUDIO_MAX_LEN.
	buffer_size = 2048;

	samples_in = memnew_arr(int32_t, buffer_size * channels);
	samples_out = memnew_arr(int16_t, buffer_size * channels);

	// BGM is the same playback route used by DeltaruneVita for its game
	// soundtrack. If it is unavailable, retain MAIN for compatibility.
	port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, buffer_size, mix_rate, SCE_AUDIO_OUT_MODE_STEREO);
	bool using_bgm = port >= 0;
	if (port < 0) {
		port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, buffer_size, mix_rate, SCE_AUDIO_OUT_MODE_STEREO);
	}
	ERR_FAIL_COND_V_MSG(port < 0, ERR_CANT_OPEN, "sceAudioOutOpenPort failed.");

	// The BGM port has a hardware automatic-level-control path. Disable it
	// to avoid aggressive gain pumping on continuous music. Leave plenty of
	// headroom at the output device itself, independent of Godot bus gains.
	if (using_bgm) {
		sceAudioOutSetAlcMode(SCE_AUDIO_ALC_OFF);
	}
	constexpr int hardware_gain = (SCE_AUDIO_OUT_MAX_VOL * 72) / 100;
	int channel_volumes[2] = { hardware_gain, hardware_gain };
	int volume_rc = sceAudioOutSetVolume(port,
			(SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH),
			channel_volumes);
	fprintf(stderr, "[vita-audio] route=%s port=%d frames=%u rate=%u hardware_gain=%d/32768 volume_rc=0x%08x\n",
			using_bgm ? "BGM" : "MAIN", port, buffer_size, mix_rate, hardware_gain, (unsigned int)volume_rc);
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
			VitaAudioPCM::convert(ad->samples_in, ad->samples_out, ad->buffer_size, ad->channels);
		} else {
			for (unsigned int i = 0; i < ad->buffer_size * ad->channels; i++) {
				ad->samples_out[i] = 0;
			}
		}
		ad->unlock();

		// Hardware output blocks until the current buffer has been consumed.
		// Keep this call outside the mixing lock to avoid holding up the main
		// thread during a slow/busy speaker/headphone output operation.
		int output_result = sceAudioOutOutput(ad->port, ad->samples_out);
		if (output_result < 0) {
			// Never spam logs from this high-priority thread. A failed block is
			// audible as a crack/pop, so preserve one error code for diagnosis.
			static bool output_error_reported = false;
			if (!output_error_reported) {
				output_error_reported = true;
				fprintf(stderr, "[vita-audio] output_failed result=0x%08x\n", (unsigned int)output_result);
			}
		}
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
