// Portable test for Godot Q31 -> Vita PCM16 conversion.
#include "../../audio_pcm_vita.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

int main() {
    const int32_t input[] = {
        INT32_MIN, -2147483647, -65536, -32768, 0, 32768, 65536,
        1073741824, INT32_MAX
    };
    const int16_t expected[] = {
        -32768, -32768, -1, -1, 0, 0, 1, 16384, 32767
    };
    int16_t output[9] = {};
    VitaAudioPCM::convert(input, output, 9, 1);
    for (int i = 0; i < 9; ++i) {
        assert(output[i] == expected[i]);
    }
    // Interleaved stereo is copied in channel order, no channel swap.
    const int32_t stereo[] = { 32767 << 16, -32768 * 65536, 0, 16384 << 16 };
    int16_t dest[4] = {};
    VitaAudioPCM::convert(stereo, dest, 2, 2);
    assert(dest[0] == 32767 && dest[1] == -32768);
    assert(dest[2] == 0 && dest[3] == 16384);
    std::puts("PASS Vita audio PCM16 conversion and interleaved stereo");
}
