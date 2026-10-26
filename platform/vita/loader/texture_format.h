#pragma once
#include <stddef.h>
#include <stdint.h>

/* ABGR4444 packed word: R in the least significant nibble, matching
 * GXM U4U4U4U4_ABGR and the existing little-endian RGBA8 upload layout. */
static inline uint16_t vita_pack_rgba4444(const uint8_t *rgba) {
    return (uint16_t)(((rgba[0] + 8u) / 17u) |
            (((rgba[1] + 8u) / 17u) << 4) |
            (((rgba[2] + 8u) / 17u) << 8) |
            (((rgba[3] + 8u) / 17u) << 12));
}

static inline size_t vita_texture_allocation_bytes(int width, int height, int bytes_per_pixel) {
    return ((((size_t)width + 7u) & ~(size_t)7u) * (size_t)height * (size_t)bytes_per_pixel + 262143u) & ~(size_t)262143u;
}
