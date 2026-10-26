#include "../../loader/texture_format.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const uint8_t red[] = {255, 0, 0, 255};
    const uint8_t green[] = {0, 255, 0, 255};
    const uint8_t blue[] = {0, 0, 255, 255};
    const uint8_t transparent[] = {0, 0, 0, 0};
    assert(vita_pack_rgba4444(red) == 0xf00f);
    assert(vita_pack_rgba4444(green) == 0xf0f0);
    assert(vita_pack_rgba4444(blue) == 0xff00);
    assert(vita_pack_rgba4444(transparent) == 0);
    for (int i = 0; i < 256; i++) {
        uint8_t rgba[4] = {i, i, i, i};
        uint16_t packed = vita_pack_rgba4444(rgba);
        for (int channel = 0; channel < 4; channel++) {
            int reconstructed = ((packed >> (channel * 4)) & 15) * 17;
            assert(reconstructed - i <= 8 && i - reconstructed <= 8);
        }
    }
    assert(vita_texture_allocation_bytes(1024, 1024, 4) == 4194304);
    assert(vita_texture_allocation_bytes(1024, 1024, 2) == 2097152);
    assert(vita_texture_allocation_bytes(32, 32, 4) == 262144);
    assert(vita_texture_allocation_bytes(32, 32, 2) == 262144);
    assert(vita_texture_allocation_bytes(257, 512, 2) == 524288);
    puts("PASS: ABGR channel order, alpha, quantization error <=8, stride and CDRAM accounting");
    return 0;
}
