/*
 * Dodge Vita / Godot Vita animated loading logo.
 *
 * Runs on its own Vita thread while the engine is loaded & relocated.
 * Uses PNG memory reads from the user's exact 180-frame sequence with
 * one persistent GPU texture (~2 MiB), one CPU staging buffer (~2 MiB),
 * and one 2.97 MiB compressed file buffer. No game textures preloaded.
 *
 * The Vita2D context is COMPLETELY torn down before the engine's GXM
 * renderer is initialized. This file does not link against libgodot.so.
 */
#include "boot_splash.h"
#include <vitasdk.h>
#include <vita2d.h>
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int debugPrintf(const char *fmt, ...);

#define BOOT_WIDTH 960u
#define BOOT_HEIGHT 544u
#define BOOT_FPS 30u
#define BOOT_FRAMES 180u
#define BOOT_CYCLE_US 6000000ull
#define BOOT_PATH "app0:boot_anim.dat"
#define BOOT_BYTES (BOOT_WIDTH * BOOT_HEIGHT * 4u)

static SceUID splash_thread = -1;
static volatile int splash_stop_requested = 0;

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

typedef struct {
    const uint8_t *bytes;
    uint32_t size;
} BootFrame;

static int read_frames(uint8_t **file_data, BootFrame frames[BOOT_FRAMES]) {
    FILE *f = fopen(BOOT_PATH, "rb");
    if (!f) {
        debugPrintf("[boot-anim] unable to open %s\n", BOOT_PATH);
        return 0;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long bytes = ftell(f);
    if (bytes < 24 || bytes > 8 * 1024 * 1024 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    uint8_t *data = malloc((size_t)bytes);
    if (!data) {
        fclose(f);
        return 0;
    }
    size_t got = fread(data, 1, (size_t)bytes, f);
    fclose(f);
    if (got != (size_t)bytes || memcmp(data, "DVAN", 4) != 0 ||
            read_u32(data + 4) != 1 || read_u32(data + 8) != BOOT_WIDTH ||
            read_u32(data + 12) != BOOT_HEIGHT || read_u32(data + 16) != BOOT_FPS ||
            read_u32(data + 20) != BOOT_FRAMES) {
        free(data);
        debugPrintf("[boot-anim] invalid loading package\n");
        return 0;
    }
    size_t off = 24;
    for (unsigned i = 0; i < BOOT_FRAMES; i++) {
        if (off + 4 > (size_t)bytes) {
            free(data);
            return 0;
        }
        uint32_t size = read_u32(data + off);
        off += 4;
        if (size < 32 || size > (size_t)bytes - off ||
                memcmp(data + off, "\x89PNG\r\n\x1a\n", 8) != 0) {
            free(data);
            return 0;
        }
        frames[i].bytes = data + off;
        frames[i].size = size;
        off += size;
    }
    if (off != (size_t)bytes) {
        free(data);
        return 0;
    }
    *file_data = data;
    return 1;
}

static int decode_to_texture(const BootFrame *frame, vita2d_texture *texture, uint8_t *stage) {
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&image, frame->bytes, frame->size)) {
        return 0;
    }
    if (image.width != BOOT_WIDTH || image.height != BOOT_HEIGHT) {
        png_image_free(&image);
        return 0;
    }
    image.format = PNG_FORMAT_RGBA;
    if (!png_image_finish_read(&image, NULL, stage, 0, NULL)) {
        png_image_free(&image);
        return 0;
    }
    png_image_free(&image);

    uint8_t *gpu = vita2d_texture_get_datap(texture);
    unsigned stride = vita2d_texture_get_stride(texture);
    unsigned pitch = stride >= BOOT_WIDTH * 4u ? stride : stride * 4u;
    if (!gpu || pitch < BOOT_WIDTH * 4u) {
        return 0;
    }
    for (unsigned y = 0; y < BOOT_HEIGHT; ++y) {
        memcpy(gpu + (size_t)y * pitch, stage + (size_t)y * BOOT_WIDTH * 4u, BOOT_WIDTH * 4u);
    }
    return 1;
}

static int splash_worker(SceSize argc, void *argv) {
    (void)argc; (void)argv;
    uint8_t *data = NULL;
    uint8_t *stage = NULL;
    vita2d_texture *texture = NULL;
    BootFrame frames[BOOT_FRAMES] = {0};
    unsigned displayed = 0;
    int init_ok = 0;

    if (!read_frames(&data, frames)) {
        debugPrintf("[boot-anim] frame package missing/invalid; continue launch\n");
        goto done;
    }
    if (vita2d_init() <= 0) {
        debugPrintf("[boot-anim] vita2d_init failed; continue launch\n");
        goto done;
    }
    init_ok = 1;
    vita2d_set_vblank_wait(0);
    vita2d_set_clear_color(0xff000000);
    texture = vita2d_create_empty_texture(BOOT_WIDTH, BOOT_HEIGHT);
    stage = malloc(BOOT_BYTES);
    if (!texture || !stage) {
        debugPrintf("[boot-anim] texture/CPU buffer allocation failed\n");
        goto done;
    }
    vita2d_texture_set_filters(texture, SCE_GXM_TEXTURE_FILTER_POINT, SCE_GXM_TEXTURE_FILTER_POINT);

    /* Entire loop lasts at least 6 seconds; repeats only while loader is busy.
       Skip overdue frames rather than stretching playback or allocating more textures. */
    const uint64_t start_us = sceKernelGetProcessTimeWide();
    int last_index = -1;
    debugPrintf("[boot-anim] begin 960x544 180 frames 30fps (during runtime loading)\n");
    while (1) {
        uint64_t elapsed = sceKernelGetProcessTimeWide() - start_us;
        if (splash_stop_requested && elapsed >= BOOT_CYCLE_US) {
            break;
        }
        int index = (int)((elapsed / (1000000u / BOOT_FPS)) % BOOT_FRAMES);
        if (index != last_index) {
            /* GPU must have finished sampling the previous contents before write. */
            vita2d_wait_rendering_done();
            if (!decode_to_texture(&frames[index], texture, stage)) {
                debugPrintf("[boot-anim] decoding frame %d failed\n", index);
                break;
            }
            vita2d_start_drawing();
            vita2d_clear_screen();
            vita2d_draw_texture(texture, 0.0f, 0.0f);
            vita2d_end_drawing();
            vita2d_wait_rendering_done();
            vita2d_swap_buffers();
            vita2d_pool_reset();
            last_index = index;
            displayed++;
        } else {
            sceKernelDelayThread(2000);
        }
    }
    debugPrintf("[boot-anim] finished frames_displayed=%u; releasing GPU before Godot\n", displayed);
done:
    if (texture) {
        vita2d_wait_rendering_done();
        vita2d_free_texture(texture);
    }
    free(stage);
    if (init_ok) {
        vita2d_fini();
    }
    free(data);
    return 0;
}

int dodge_boot_splash_start(void) {
    splash_stop_requested = 0;
    if (splash_thread >= 0) {
        return 0;
    }
    splash_thread = sceKernelCreateThread("dodge_splash", splash_worker,
            0x10000100, 0x40000, 0, SCE_KERNEL_CPU_MASK_USER_ALL, NULL);
    if (splash_thread < 0) {
        debugPrintf("[boot-anim] sceKernelCreateThread failed: 0x%08x\n", splash_thread);
        return 0;
    }
    int code = sceKernelStartThread(splash_thread, 0, NULL);
    if (code < 0) {
        debugPrintf("[boot-anim] sceKernelStartThread failed: 0x%08x\n", code);
        sceKernelDeleteThread(splash_thread);
        splash_thread = -1;
        return 0;
    }
    return 1;
}

void dodge_boot_splash_stop(void) {
    if (splash_thread < 0) {
        return;
    }
    splash_stop_requested = 1;
    sceKernelWaitThreadEnd(splash_thread, NULL, NULL);
    sceKernelDeleteThread(splash_thread);
    splash_thread = -1;
}
