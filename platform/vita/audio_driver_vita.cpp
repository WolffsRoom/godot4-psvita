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

#include "core/config/project_settings.h"
#include "core/os/os.h"

#include <psp2/kernel/threadmgr.h>
#include <stdlib.h>
#include <stdio.h>

// Two output layouts, chosen by the project setting audio/vita/output_mode:
//
// "godot3" (default): the layout of the Godot 3 Vita driver used by Dome
// Keeper, which plays without distortion on hardware. One BGM port at the
// project mix rate (audio/driver/mix_rate, 44.1 kHz by default), a block of
// audio/driver/output_latency rounded to a power of two (512 frames at the
// defaults), firmware-default hardware volume and ALC, and a normal-priority
// mixer thread on the user cores. Kept from the 4.x driver: the mixer lock is
// released before sceAudioOutOutput and there is no extra sleep per block.
//
// "main48": the previous 4.x layout. MAIN port at 48 kHz, 2048-frame blocks,
// 72% hardware gain, ALC off on the BGM fallback, high-priority thread that
// moves to CPU3 when CapUnlocker is present.

static bool vita_audio_godot3_layout() {
	ProjectSettings *settings = ProjectSettings::get_singleton();
	String mode = settings ? String(settings->get_setting("audio/vita/output_mode", "godot3")) : String("godot3");
	return mode != "main48";
}

Error AudioDriverVita::init() {
	speaker_mode = SPEAKER_MODE_STEREO;
	channels = 2;
	godot3_layout = vita_audio_godot3_layout();
	bool using_bgm = false;

	if (godot3_layout) {
		mix_rate = CLAMP(_get_configured_mix_rate(), 8000, 48000);
		int latency = int(ProjectSettings::get_singleton()->get_setting("audio/driver/output_latency", 15));
		latency = CLAMP(latency, 1, 500);
		// Godot 3 used closest_power_of_2(latency * mix_rate / 1000).
		unsigned int target = MAX(1u, (unsigned int)latency * mix_rate / 1000u);
		unsigned int upper = 64;
		while (upper < target) {
			upper <<= 1;
		}
		unsigned int lower = MAX(64u, upper >> 1);
		buffer_size = (target - lower < upper - target) ? lower : upper;
		buffer_size = MIN(buffer_size, (unsigned int)(SCE_AUDIO_MAX_LEN & ~63));
		port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, buffer_size, mix_rate, SCE_AUDIO_OUT_MODE_STEREO);
		using_bgm = port >= 0;
		if (port < 0) {
			// MAIN only accepts 48 kHz.
			fprintf(stderr, "[vita-audio] BGM port at %u Hz failed rc=0x%08x; using MAIN 48 kHz\n", mix_rate, (unsigned int)port);
			mix_rate = 48000;
			port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, buffer_size, mix_rate, SCE_AUDIO_OUT_MODE_STEREO);
		}
	} else {
		mix_rate = 48000;
		// 2048 samples = ~42.7 ms at 48 kHz.
		// Main port accepts multiples of 64 up to SCE_AUDIO_MAX_LEN.
		buffer_size = 2048;
		// MAIN is the general game-audio route and carries Godot's complete mix.
		// Keep BGM only as a fallback for firmwares where MAIN cannot be acquired.
		port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, buffer_size, mix_rate, SCE_AUDIO_OUT_MODE_STEREO);
		if (port < 0) {
			port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, buffer_size, mix_rate, SCE_AUDIO_OUT_MODE_STEREO);
			using_bgm = port >= 0;
		}
	}
	ERR_FAIL_COND_V_MSG(port < 0, ERR_CANT_OPEN, "sceAudioOutOpenPort failed.");

	samples_in = memnew_arr(int32_t, buffer_size * channels);
	samples_out = memnew_arr(int16_t, buffer_size * channels);

	int hardware_gain = -1;
	int volume_rc = 0;
	if (!godot3_layout) {
		// The BGM port has a hardware automatic-level-control path. Disable it
		// to avoid aggressive gain pumping on continuous music. Leave plenty of
		// headroom at the output device itself, independent of Godot bus gains.
		if (using_bgm) {
			sceAudioOutSetAlcMode(SCE_AUDIO_ALC_OFF);
		}
		hardware_gain = (SCE_AUDIO_OUT_MAX_VOL * 72) / 100;
		int channel_volumes[2] = { hardware_gain, hardware_gain };
		volume_rc = sceAudioOutSetVolume(port,
				(SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH),
				channel_volumes);
	}
	fprintf(stderr, "[vita-audio] layout=%s route=%s port=%d frames=%u rate=%u hardware_gain=%d volume_rc=0x%08x\n",
			godot3_layout ? "godot3" : "main48", using_bgm ? "BGM" : "MAIN", port, buffer_size, mix_rate,
			hardware_gain, (unsigned int)volume_rc);
	return OK;
}

void AudioDriverVita::start() {
	active = true;
	Thread::Settings settings;
	// Godot 3 ran the mixer at the default priority.
	settings.priority = godot3_layout ? Thread::PRIORITY_NORMAL : Thread::PRIORITY_HIGH;
	Thread::ID thread_id = thread.start(AudioDriverVita::thread_func, this, settings);
	fprintf(stderr, "[vita-audio] thread_start id=%llu started=%d\n",
			(unsigned long long)thread_id, thread.is_started() ? 1 : 0);
	fflush(stderr);
}

void AudioDriverVita::thread_func(void *p_udata) {
	AudioDriverVita *ad = static_cast<AudioDriverVita *>(p_udata);
	int affinity_rc = 0;
	const char *capunlocker = getenv("GODOT_VITA_CAPUNLOCKER");
	if (!ad->godot3_layout && capunlocker && capunlocker[0] == '1') {
		affinity_rc = sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_SYSTEM);
	}
	fprintf(stderr, "[vita-audio] mixer_thread running capunlocker=%d cpu3_affinity_rc=0x%08x\n",
			capunlocker && capunlocker[0] == '1' ? 1 : 0, (unsigned int)affinity_rc);
	fflush(stderr);
	unsigned int block_count = 0;

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
		if ((block_count++ % 120) == 0) {
			int peak = 0;
			for (unsigned int i = 0; i < ad->buffer_size * ad->channels; i++) {
				int sample = ad->samples_out[i];
				int magnitude = sample < 0 ? -sample : sample;
				if (magnitude > peak) peak = magnitude;
			}
			fprintf(stderr, "[vita-audio] blocks=%u peak=%d active=%d\n", block_count, peak, ad->active ? 1 : 0);
			fflush(stderr);
		}

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
