/*
 * Godot Vita boot animation -- safe CPU framebuffer path.
 *
 * Do not initialize Vita2D/GXM while so_load is mapping libgodot.so.
 * The 960x544 PNGs decode straight to two displayable CDRAM buffers.
 * Each queued flip waits for VBlank; black out the display before release.
 */
#include "boot_splash.h"
#include <vitasdk.h>
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>

extern int debugPrintf(const char *fmt, ...);

#define BOOT_WIDTH 960u
#define BOOT_HEIGHT 544u
#define BOOT_FPS 30u
#define BOOT_FRAMES 180u
#define BOOT_CYCLE_US 6000000ull
#define BOOT_PATH "app0:boot_anim.dat"
#define BOOT_BYTES (BOOT_WIDTH * BOOT_HEIGHT * 4u)
#define BOOT_MEM_BYTES ((BOOT_BYTES + 262143u) & ~262143u)
#define BOOT_FILE_LIMIT (8u * 1024u * 1024u)

typedef struct {
    const uint8_t *bytes;
    uint32_t size;
} BootFrame;

static SceUID splash_thread = -1;
static volatile int splash_stop_requested;
static volatile int splash_status; /* 0=startup, 1=presenting, -1=failed, 2=stopped */

static uint32_t boot_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
            ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int read_frames(uint8_t **file_data, BootFrame frames[BOOT_FRAMES]) {
    FILE *f = fopen(BOOT_PATH, "rb");
    if (!f) { debugPrintf("[boot-anim] cannot open %s\n", BOOT_PATH); return 0; }
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 0; }
    long length = ftell(f);
    if (length < 24 || length > (long)BOOT_FILE_LIMIT ||
            fseek(f, 0, SEEK_SET)) { fclose(f); return 0; }
    uint8_t *data = malloc((size_t)length);
    if (!data) { fclose(f); return 0; }
    const size_t got = fread(data, 1, (size_t)length, f);
    fclose(f);
    if (got != (size_t)length || memcmp(data, "DVAN", 4) ||
            boot_u32(data + 4) != 1 ||
            boot_u32(data + 8) != BOOT_WIDTH ||
            boot_u32(data + 12) != BOOT_HEIGHT ||
            boot_u32(data + 16) != BOOT_FPS ||
            boot_u32(data + 20) != BOOT_FRAMES) {
        free(data);
        debugPrintf("[boot-anim] bad DVAN header\n");
        return 0;
    }
    size_t offset = 24;
    for (unsigned i = 0; i < BOOT_FRAMES; ++i) {
        if (offset > (size_t)length || (size_t)length - offset < 4) {
            free(data); return 0;
        }
        uint32_t bytes = boot_u32(data + offset);
        offset += 4;
        if (bytes < 32 || bytes > (size_t)length - offset ||
                memcmp(data + offset, "\x89PNG\r\n\x1a\n", 8)) {
            free(data); return 0;
        }
        frames[i].bytes = data + offset;
        frames[i].size = bytes;
        offset += bytes;
    }
    if (offset != (size_t)length) { free(data); return 0; }
    *file_data = data;
    return 1;
}

static int decode_frame(const BootFrame *frame, void *display_pixels) {
    png_image png;
    memset(&png, 0, sizeof(png));
    png.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&png, frame->bytes, frame->size)) {
        return 0;
    }
    if (png.width != BOOT_WIDTH || png.height != BOOT_HEIGHT) {
        png_image_free(&png);
        return 0;
    }
    /* PNG_FORMAT_RGBA bytes correspond to SCE_DISPLAY_PIXELFORMAT_A8B8G8R8
     * in Vita little-endian memory: R,G,B,A; no intermediate staging copy. */
    png.format = PNG_FORMAT_RGBA;
    const int ok = png_image_finish_read(&png, NULL, display_pixels, 0, NULL);
    png_image_free(&png);
    return ok;
}

/* PNG decoding (~50 ms per 960x544 frame) on a single thread, sharing the
 * CPU with so_load, held the animation near 20 FPS. Two decoder threads on
 * their own cores fill a ring of displayable buffers; the presenter shows
 * frames strictly in order at up to BOOT_FPS. */
#define DECODERS 2u
#define RING (DECODERS * 2u)
static uint8_t *ring_pixels[RING];
static volatile int ring_frame[RING];   /* -1 free, else decoded frame number */
static volatile int ring_busy[RING];    /* claimed by a decoder */
static volatile int decoders_stop;
static volatile int decoder_failed;
static BootFrame *decode_frames;

static int decoder_worker(SceSize argc, void *argv) {
    const unsigned id = *(const unsigned *)argv;
    (void)argc;
    for (unsigned n = id; !decoders_stop; n += DECODERS) {
        /* Each decoder owns ring slots id*2 and id*2+1. */
        int slot = -1;
        while (!decoders_stop && slot < 0) {
            for (unsigned k = 0; k < 2; ++k) {
                unsigned candidate = id * 2u + k;
                if (ring_frame[candidate] < 0 && !ring_busy[candidate]) { slot = (int)candidate; break; }
            }
            if (slot < 0) sceKernelDelayThread(1000);
        }
        if (slot < 0) break;
        ring_busy[slot] = 1;
        if (!decode_frame(&decode_frames[n % BOOT_FRAMES], ring_pixels[slot])) {
            decoder_failed = 1;
            ring_busy[slot] = 0;
            break;
        }
        ring_frame[slot] = (int)n;
        ring_busy[slot] = 0;
    }
    return 0;
}

static int splash_worker(SceSize argc, void *argv) {
    (void)argc; (void)argv;
    uint8_t *data = NULL;
    BootFrame frames[BOOT_FRAMES] = {{0}};
    SceUID block[RING];
    SceUID decoder[DECODERS];
    unsigned decoder_id[DECODERS];
    unsigned displayed = 0;
    int video_owned = 0;
    int shown_slot = -1;
    for (unsigned i = 0; i < RING; ++i) { block[i] = -1; ring_pixels[i] = NULL; ring_frame[i] = -1; ring_busy[i] = 0; }
    for (unsigned i = 0; i < DECODERS; ++i) decoder[i] = -1;
    decoders_stop = 0;
    decoder_failed = 0;

    debugPrintf("[boot-anim] cpu-framebuffer worker started, %u decoders, no vita2d_init\n", DECODERS);
    if (!read_frames(&data, frames)) goto cleanup;
    decode_frames = frames;

    for (unsigned i = 0; i < RING; ++i) {
        block[i] = sceKernelAllocMemBlock("boot_display", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, BOOT_MEM_BYTES, NULL);
        void *base = NULL;
        if (block[i] < 0 || sceKernelGetMemBlockBase(block[i], &base) < 0 || !base) {
            debugPrintf("[boot-anim] framebuffer %u allocation failed 0x%08x\n", i, (unsigned)block[i]);
            goto cleanup;
        }
        ring_pixels[i] = base;
        memset(base, 0, BOOT_BYTES);
    }
    debugPrintf("[boot-anim] direct display allocated %ux%u bytes\n", RING, BOOT_MEM_BYTES);

    for (unsigned i = 0; i < DECODERS; ++i) {
        decoder_id[i] = i;
        /* Cores 1 and 2: the loader's main thread keeps core 0 for so_load. */
        decoder[i] = sceKernelCreateThread("boot_decode", decoder_worker, 0x10000100, 0x40000, 0,
                i ? SCE_KERNEL_CPU_MASK_USER_2 : SCE_KERNEL_CPU_MASK_USER_1, NULL);
        if (decoder[i] < 0 || sceKernelStartThread(decoder[i], sizeof(unsigned), &decoder_id[i]) < 0) {
            debugPrintf("[boot-anim] decoder %u start failed\n", i);
            if (decoder[i] >= 0) sceKernelDeleteThread(decoder[i]);
            decoder[i] = -1;
            goto cleanup;
        }
    }

    {
        const uint64_t frame_us = 1000000u / BOOT_FPS;
        uint64_t next_due = sceKernelGetProcessTimeWide();
        unsigned late = 0;
        for (unsigned n = 0;; ++n) {
            if (splash_stop_requested && displayed >= BOOT_FRAMES) break;
            const unsigned owner = n % DECODERS;
            int slot = -1;
            while (slot < 0 && !decoder_failed) {
                for (unsigned k = 0; k < 2; ++k) {
                    unsigned candidate = owner * 2u + k;
                    if (ring_frame[candidate] == (int)n) { slot = (int)candidate; break; }
                }
                if (slot < 0) sceKernelDelayThread(1000);
            }
            if (slot < 0) break;
            uint64_t now = sceKernelGetProcessTimeWide();
            if (now < next_due) sceKernelDelayThread((SceUInt)(next_due - now));
            else if (now > next_due + frame_us) late++;
            next_due += frame_us;
            if (next_due + frame_us < sceKernelGetProcessTimeWide()) next_due = sceKernelGetProcessTimeWide();
            SceDisplayFrameBuf fb;
            memset(&fb, 0, sizeof(fb));
            fb.size = sizeof(fb);
            fb.base = ring_pixels[slot];
            fb.pitch = BOOT_WIDTH;
            fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
            fb.width = BOOT_WIDTH;
            fb.height = BOOT_HEIGHT;
            int rc = sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
            if (rc < 0) {
                debugPrintf("[boot-anim] sceDisplaySetFrameBuf failed 0x%08x\n", (unsigned)rc);
                break;
            }
            video_owned = 1;
            sceDisplayWaitVblankStart();
            splash_status = 1;
            /* The previous buffer is no longer scanned out: give it back. */
            if (shown_slot >= 0) ring_frame[shown_slot] = -1;
            shown_slot = slot;
            ++displayed;
        }
        debugPrintf("[boot-anim] presenter done frames=%u late=%u\n", displayed, late);
    }

cleanup:
    decoders_stop = 1;
    for (unsigned i = 0; i < DECODERS; ++i) {
        if (decoder[i] >= 0) {
            sceKernelWaitThreadEnd(decoder[i], NULL, NULL);
            sceKernelDeleteThread(decoder[i]);
        }
    }
    debugPrintf("[boot-anim] direct display finished frames=%u\n", displayed);
    if (video_owned) {
        /* SDK explicitly documents NULL framebuffer as 'black output'.
         * Wait a retrace before deallocating formerly scanned-out CDRAM. */
        int rc = sceDisplaySetFrameBuf(NULL, SCE_DISPLAY_SETBUF_IMMEDIATE);
        if (rc >= 0) sceDisplayWaitVblankStart();
        else debugPrintf("[boot-anim] framebuffer blackout failed 0x%08x\n", (unsigned)rc);
        if (rc < 0 && shown_slot >= 0) {
            /* Do not release the buffer the display could still scan. */
            block[shown_slot] = -1;
        }
    }
    for (unsigned i = 0; i < RING; ++i) {
        if (block[i] >= 0) sceKernelFreeMemBlock(block[i]);
    }
    free(data);
    if (splash_status == 0) splash_status = -1;
    else splash_status = 2;
    debugPrintf("[boot-anim] direct display cleanup completed\n");
    return 0;
}

int dodge_boot_splash_start(void) {
    if (splash_thread >= 0) return 0;
    splash_stop_requested = 0;
    splash_status = 0;
    splash_thread = sceKernelCreateThread("dodge_splash", splash_worker,
            0x10000100, 0x40000, 0, SCE_KERNEL_CPU_MASK_USER_ALL, NULL);
    if (splash_thread < 0) {
        debugPrintf("[boot-anim] create worker failed 0x%08x\n", (unsigned)splash_thread);
        return 0;
    }
    int rc = sceKernelStartThread(splash_thread, 0, NULL);
    if (rc < 0) {
        debugPrintf("[boot-anim] start worker failed 0x%08x\n", (unsigned)rc);
        sceKernelDeleteThread(splash_thread);
        splash_thread = -1;
        return 0;
    }
    /* Wait for successful display setup before so_load uses more memory.
     * If the splash can't start, its worker cleans up and the game loads. */
    for (int i = 0; i < 4000 && splash_status == 0; ++i) {
        sceKernelDelayThread(1000);
    }
    debugPrintf("[boot-anim] direct display startup status=%d\n", splash_status);
    return splash_status == 1;
}

void dodge_boot_splash_stop(void) {
    if (splash_thread < 0) return;
    splash_stop_requested = 1;
    sceKernelWaitThreadEnd(splash_thread, NULL, NULL);
    sceKernelDeleteThread(splash_thread);
    splash_thread = -1;
}
