/* Native GXM transport for the experimental Godot canvas. No OpenGL calls. */
#include "../gxm_bridge.h"
#include <vita2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int drawing;
static int initialized;
typedef struct GXMTexture {
    vita2d_texture *texture;
    size_t bytes;
    struct GXMTexture *next;
} GXMTexture;
static GXMTexture *retired;
static size_t texture_bytes;

static void destroy_texture(GXMTexture *t) {
    vita2d_free_texture(t->texture);
    texture_bytes -= t->bytes;
    free(t);
}
void *vita_gxm_texture_create(int width, int height, const uint8_t *rgba) {
    if (!initialized || !rgba || width < 1 || height < 1 || width > 4096 || height > 4096) return NULL;
    /* CDRAM allocations are rounded to 256 KiB, including tiny font atlases. */
    size_t bytes = ((((size_t)width + 7) & ~(size_t)7) * height * 4 + 262143) & ~(size_t)262143;
    if (bytes > 48 * 1024 * 1024 || texture_bytes > 48 * 1024 * 1024 - bytes) return NULL;
    GXMTexture *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->texture = vita2d_create_empty_texture(width, height);
    if (!t->texture) { free(t); return NULL; }
    uint8_t *data = vita2d_texture_get_datap(t->texture);
    unsigned stride = vita2d_texture_get_stride(t->texture);
    for (int y = 0; y < height; y++) {
        memcpy(data + (size_t)y * stride, rgba + (size_t)y * width * 4, (size_t)width * 4);
    }
    t->bytes = bytes;
    texture_bytes += bytes;
    fprintf(stderr, "[gxm2d] texture uploaded %dx%d budget=%lu\n", width, height, (unsigned long)texture_bytes);
    fflush(stderr);
    return t;
}
void vita_gxm_texture_free(void *texture) {
    GXMTexture *t = texture;
    if (!t) return;
    if (drawing) {
        t->next = retired;
        retired = t;
    } else {
        destroy_texture(t);
    }
}
void vita_gxm_texture_quad(void *texture, const float *xy, const float *uv, uint32_t color, int linear) {
    GXMTexture *t = texture;
    if (!drawing || !t) return;
    vita2d_texture_vertex *v = vita2d_pool_memalign(6 * sizeof(*v), 8);
    if (!v) return;
    const int corners[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++) {
        int k = corners[i];
        v[i].x = xy[2 * k]; v[i].y = xy[2 * k + 1]; v[i].z = 0.5f;
        v[i].u = uv[2 * k]; v[i].v = uv[2 * k + 1];
    }
    vita2d_texture_set_filters(t->texture, linear ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT,
            linear ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT);
    vita2d_draw_array_textured(t->texture, SCE_GXM_PRIMITIVE_TRIANGLES, v, 6, color);
}
int vita_gxm_init(void) {
    if (initialized) return 1;
    int result = vita2d_init();
    fprintf(stderr, "[gxm2d] vita2d_init=%d\n", result);
    fflush(stderr);
    initialized = result > 0;
    return initialized;
}
void vita_gxm_begin(uint32_t color) {
    if (!initialized || drawing) return;
    vita2d_start_drawing();
    vita2d_disable_clipping();
    vita2d_set_clear_color(color);
    vita2d_clear_screen();
    drawing = 1;
}
void vita_gxm_quad(const float *xy, uint32_t color) {
    if (!drawing) return;
    vita2d_color_vertex *v = vita2d_pool_memalign(6 * sizeof(*v), 8);
    if (!v) return;
    const int corners[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++) {
        v[i].x = xy[corners[i] * 2];
        v[i].y = xy[corners[i] * 2 + 1];
        v[i].z = 0.5f;
        v[i].color = color;
    }
    vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLES, v, 6);
}
void vita_gxm_clip(int enabled, int x, int y, int width, int height) {
    if (enabled) {
        vita2d_set_clip_rectangle(x, y, x + width, y + height);
        vita2d_enable_clipping();
    } else {
        vita2d_disable_clipping();
    }
}
void vita_gxm_end(int present) {
    if (!drawing) return;
    vita2d_disable_clipping();
    vita2d_end_drawing();
    /* Keep pool vertices alive until the GPU has consumed them. */
    vita2d_wait_rendering_done();
    while (retired) {
        GXMTexture *t = retired;
        retired = t->next;
        destroy_texture(t);
    }
    if (present) vita2d_swap_buffers();
    vita2d_pool_reset();
    drawing = 0;
}
void vita_gxm_finish(void) {
    if (!initialized) return;
    vita_gxm_end(0);
    vita2d_fini();
    initialized = 0;
}
