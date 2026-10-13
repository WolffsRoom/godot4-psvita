/**************************************************************************/
/*  audio_pcm_vita.h                                                      */
/*  Native 16-bit output conversion; independently host-testable.         */
/**************************************************************************/
#pragma once
#include <stdint.h>

namespace VitaAudioPCM {
// Godot's audio_server_process() produces Q31-style signed PCM in int32_t,
// with the high 16 bits carrying the hardware sample. This mirrors Godot's
// ALSA driver. Input is already clamped by AudioServer::_driver_process,
// so this converter cannot repair clipping introduced inside the mixer.
static inline int16_t to_s16(int32_t fixed_sample) {
    int32_t value = fixed_sample >> 16;
    if (value > 32767) {
        value = 32767;
    } else if (value < -32768) {
        value = -32768;
    }
    return (int16_t)value;
}

static inline void convert(const int32_t *input, int16_t *output, unsigned frames, unsigned channels) {
    const unsigned count = frames * channels;
    for (unsigned i = 0; i < count; ++i) {
        output[i] = to_s16(input[i]);
    }
}
} // namespace VitaAudioPCM
