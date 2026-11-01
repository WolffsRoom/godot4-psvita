/* Native GXM transport for the experimental Godot canvas. No OpenGL calls. */
#include "../gxm_bridge.h"
#include "texture_format.h"
#include <vita2d.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/gxm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int drawing;
static int gpu_pending;
static void wait_gpu(void);
static int initialized;
static int texture_rgba4444;
static int scene3d_enabled;
typedef struct GXMTexture {
    vita2d_texture *texture;
    size_t bytes;
    struct GXMTexture *next;
} GXMTexture;
static GXMTexture *retired;
static size_t texture_bytes;
static unsigned memory_frame;
static unsigned texture_upload_count;
static int scene3d_drawing;
static float canvas_matrix[16];
static unsigned last_gpu_wait_us;
/* libvita2d helpers (utils.c): allocate and GPU-map a memory block. */
void *gpu_alloc(SceKernelMemBlockType type, unsigned int size, unsigned int alignment, unsigned int attribs, SceUID *uid);
void gpu_free(SceUID uid);
/* Static mesh arena: bump allocated, reset as a whole by the renderer when
 * its geometry cache is cleared. The GPU is idle between frames because
 * vita_gxm_end waits for rendering, so a reset never races a draw. */
#define MESH_ARENA_BYTES (24u * 1024u * 1024u)
typedef struct GXMMesh {
    vita2d_texture_vertex *vertices;
    unsigned count;
} GXMMesh;
static uint8_t *mesh_arena;
static SceUID mesh_arena_uid = -1;
static unsigned mesh_arena_used;
static int mesh_arena_failed;
static vita2d_texture *white_texture;

/* Draw batching: consecutive quads with the same texture/filter/tint are
 * appended to one contiguous pool range and submitted as a single draw.
 * vita2d_pool_memalign is a bump allocator, so back-to-back allocations of
 * 6-vertex blocks (120/96 bytes, multiples of the 8-byte alignment) are
 * contiguous. Flat-colour quads carry per-vertex colour and always merge. */
#define BATCH_MAX_VERTICES 65532u
enum { BATCH_NONE, BATCH_TEXTURED, BATCH_COLOR };
static int batch_kind;
static GXMTexture *batch_texture;
static int batch_linear;
static uint32_t batch_color;
static void *batch_start;
static unsigned batch_count;
/* Per-frame statistics, logged every PERF_FRAMES frames. */
#define PERF_FRAMES 300u
static unsigned stat_draws, stat_quads, stat_clips, stat_targets;
static unsigned long long perf_draws, perf_quads, perf_clips, perf_gpu_wait, perf_frame_us;
static unsigned perf_frames;
static SceUInt64 perf_last_end;
/* Region-clip cache: vita2d re-draws two full stencil rectangles on every
 * clip change, so redundant per-item calls are skipped. */
static int clip_enabled_cache;
static int clip_rect_cache[4];
/* Offscreen render-target scene (SubViewport). */
static int target_drawing;
static int target_dependency;

static void flush_batch(void) {
    if (batch_count) {
        if (batch_kind == BATCH_TEXTURED) {
            vita2d_texture_set_filters(batch_texture->texture,
                    batch_linear ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT,
                    batch_linear ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT);
            vita2d_draw_array_textured(batch_texture->texture, SCE_GXM_PRIMITIVE_TRIANGLES,
                    (const vita2d_texture_vertex *)batch_start, batch_count, batch_color);
        } else if (batch_kind == BATCH_COLOR) {
            vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLES, (const vita2d_color_vertex *)batch_start, batch_count);
        }
        stat_draws++;
    }
    batch_kind = BATCH_NONE;
    batch_count = 0;
    batch_start = NULL;
}

/* Adds 6 freshly allocated vertices to the open batch or starts a new one. */
static void batch_append(int kind, GXMTexture *t, int linear, uint32_t color, void *vertices, size_t vertex_size) {
    stat_quads++;
    if (batch_count && batch_kind == kind && batch_count + 6 <= BATCH_MAX_VERTICES &&
            (uint8_t *)batch_start + (size_t)batch_count * vertex_size == (uint8_t *)vertices &&
            (kind == BATCH_COLOR || (batch_texture == t && batch_linear == linear && batch_color == color))) {
        batch_count += 6;
        return;
    }
    flush_batch();
    batch_kind = kind;
    batch_texture = t;
    batch_linear = linear;
    batch_color = color;
    batch_start = vertices;
    batch_count = 6;
}

static void log_free_memory(const char *stage) {
    SceKernelFreeMemorySizeInfo info = { 0 };
    info.size = sizeof(info);
    int result = sceKernelGetFreeMemorySize(&info);
    fprintf(stderr, "[vita-memory] stage=%s result=0x%08x user_free=%u cdram_free=%u phycont_free=%u textures=%lu\n",
            stage, (unsigned int)result, info.size_user, info.size_cdram,
            info.size_phycont, (unsigned long)texture_bytes);
    fflush(stderr);
}

static void destroy_texture(GXMTexture *t) {
    vita2d_free_texture(t->texture);
    texture_bytes -= t->bytes;
    free(t);
}
void *vita_gxm_texture_create(int width, int height, const uint8_t *rgba) {
    if (!initialized || !rgba || width < 1 || height < 1 || width > 4096 || height > 4096) return NULL;
    /* CDRAM allocations are rounded to 256 KiB, including tiny font atlases. */
    size_t bytes = vita_texture_allocation_bytes(width, height, texture_rgba4444 ? 2 : 4);
    if (bytes > 64 * 1024 * 1024 || texture_bytes > 64 * 1024 * 1024 - bytes) return NULL;
    GXMTexture *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->texture = vita2d_create_empty_texture_format(width, height,
            texture_rgba4444 ? SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_ABGR : SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
    if (!t->texture) { free(t); return NULL; }
    uint8_t *data = vita2d_texture_get_datap(t->texture);
    unsigned stride = vita2d_texture_get_stride(t->texture);
    for (int y = 0; y < height; y++) {
        const uint8_t *source = rgba + (size_t)y * width * 4;
        if (texture_rgba4444) {
            uint16_t *destination = (uint16_t *)(data + (size_t)y * stride);
            for (int x = 0; x < width; x++) destination[x] = vita_pack_rgba4444(source + x * 4);
        } else {
            memcpy(data + (size_t)y * stride, source, (size_t)width * 4);
        }
    }
    t->bytes = bytes;
    texture_bytes += bytes;
    texture_upload_count++;
    if (texture_upload_count <= 16 || texture_upload_count % 128 == 0) {
        fprintf(stderr, "[gxm2d] texture uploads=%u last=%dx%d format=%s allocation=%lu budget=%lu\n", texture_upload_count, width, height,
                texture_rgba4444 ? "RGBA4444" : "RGBA8888", (unsigned long)bytes, (unsigned long)texture_bytes);
        fflush(stderr);
    }
    return t;
}
void vita_gxm_texture_free(void *texture) {
    GXMTexture *t = texture;
    if (!t) return;
    if (drawing || target_drawing || gpu_pending) {
        t->next = retired;
        retired = t;
    } else {
        destroy_texture(t);
    }
}
void vita_gxm_texture_quad(void *texture, const float *xy, const float *uv, uint32_t color, int linear) {
    GXMTexture *t = texture;
    if (!(drawing || target_drawing) || !t) return;
    vita2d_texture_vertex *v = vita2d_pool_memalign(6 * sizeof(*v), 8);
    if (!v) return;
    const int corners[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++) {
        int k = corners[i];
        v[i].x = xy[2 * k]; v[i].y = xy[2 * k + 1]; v[i].z = 0.5f;
        v[i].u = uv[2 * k]; v[i].v = uv[2 * k + 1];
    }
    batch_append(BATCH_TEXTURED, t, linear, color, v, sizeof(*v));
}
int vita_gxm_init(void) {
    if (initialized) return 1;
    FILE *scene_file = fopen("app0:gxm3d.cfg", "rb");
    scene3d_enabled = scene_file != NULL;
    if (scene_file) fclose(scene_file);
    /* Old packages without a format marker retain their original quality. */
    texture_rgba4444 = 0;
    FILE *format_file = fopen("app0:gxm_texture_format.cfg", "rb");
    if (format_file) {
        char format[32] = {0};
        if (fgets(format, sizeof(format), format_file)) texture_rgba4444 = strcmp(format, "RGBA4444\n") == 0;
        fclose(format_file);
    }
    /* 2D games also emit thousands of tile and glyph quads per frame; the
     * default 1 MiB vertex pool silently drops draws past ~8k quads. */
    int result = vita2d_init_advanced(4 * 1024 * 1024);
    fprintf(stderr, "[gxm2d] vita2d_init=%d texture_format=%s\n", result,
            texture_rgba4444 ? "RGBA4444" : "RGBA8888");
    fflush(stderr);
    initialized = result > 0;
    log_free_memory("renderer_init");
    return initialized;
}
void vita_gxm_begin(uint32_t color) {
    if (!initialized || drawing) return;
    if ((memory_frame++ % 600) == 0) log_free_memory("frame");
    wait_gpu();
    /* Not vita2d_start_drawing(): it resets the vertex pool, which an
     * offscreen scene earlier in this frame may still be reading. The pool
     * is reset by wait_gpu() once the previous frame is done. */
    vita2d_start_drawing_advanced(NULL, target_dependency ? SCE_GXM_SCENE_VERTEX_WAIT_FOR_DEPENDENCY : 0);
    target_dependency = 0;
    vita2d_disable_clipping();
    clip_enabled_cache = 0;
    vita2d_set_clear_color(color);
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetViewport(ctx, 480.0f, 480.0f, 272.0f, -272.0f, 0.5f, 0.5f);
    if (scene3d_enabled) {
        sceGxmSetFrontDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
        sceGxmSetBackDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
        sceGxmSetFrontDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_ENABLED);
        sceGxmSetBackDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_ENABLED);
    }
    vita2d_clear_screen();
    if (scene3d_enabled) {
        sceGxmSetFrontDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
        sceGxmSetBackDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
    }
    drawing = 1;
}
void vita_gxm_quad(const float *xy, uint32_t color) {
    if (!(drawing || target_drawing)) return;
    vita2d_color_vertex *v = vita2d_pool_memalign(6 * sizeof(*v), 8);
    if (!v) return;
    const int corners[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; i++) {
        v[i].x = xy[corners[i] * 2];
        v[i].y = xy[corners[i] * 2 + 1];
        v[i].z = 0.5f;
        v[i].color = color;
    }
    batch_append(BATCH_COLOR, NULL, 0, 0, v, sizeof(*v));
}
void vita_gxm_clip(int enabled, int x, int y, int width, int height) {
    if (target_drawing) return; /* Offscreen targets draw unclipped. */
    if (enabled == clip_enabled_cache && (!enabled || (clip_rect_cache[0] == x && clip_rect_cache[1] == y &&
            clip_rect_cache[2] == width && clip_rect_cache[3] == height))) {
        return;
    }
    flush_batch();
    stat_clips++;
    clip_enabled_cache = enabled;
    clip_rect_cache[0] = x; clip_rect_cache[1] = y; clip_rect_cache[2] = width; clip_rect_cache[3] = height;
    if (enabled) {
        vita2d_set_clip_rectangle(x, y, x + width, y + height);
        vita2d_enable_clipping();
    } else {
        vita2d_disable_clipping();
    }
}
/* The GPU renders a frame while the CPU runs the next frame's scripts and
 * physics. Pool vertices and retired textures are only released once that
 * frame is done: at the start of the next frame, not right after submitting
 * it (R16 logs: 5-9 ms of CPU idle per frame waiting here). */
static void wait_gpu(void) {
    if (!gpu_pending) return;
    SceUInt64 wait_started = sceKernelGetProcessTimeWide();
    vita2d_wait_rendering_done();
    last_gpu_wait_us = (unsigned)(sceKernelGetProcessTimeWide() - wait_started);
    gpu_pending = 0;
    while (retired) {
        GXMTexture *t = retired;
        retired = t->next;
        destroy_texture(t);
    }
    vita2d_pool_reset();
}
static int tilt_offscreen; /* The 3D pass is in the tilt-shift target */
void vita_gxm_end(int present) {
    if (!drawing) return;
    if (tilt_offscreen) vita_gxm_tiltshift_resolve(0.0f, 1.0f, 0.0f);
    flush_batch();
    vita_gxm_scene3d_end();
    vita2d_disable_clipping();
    vita2d_end_drawing();
    if (present) vita2d_swap_buffers();
    gpu_pending = 1;
    drawing = 0;
    if (present) {
        SceUInt64 now = sceKernelGetProcessTimeWide();
        if (perf_last_end) perf_frame_us += now - perf_last_end;
        perf_last_end = now;
        perf_draws += stat_draws; perf_quads += stat_quads; perf_clips += stat_clips;
        perf_gpu_wait += last_gpu_wait_us;
        if (++perf_frames >= PERF_FRAMES) {
            fprintf(stderr, "[gxm2d-perf] frames=%u avg_frame_us=%llu fps=%.1f draws=%llu quads=%llu clips=%llu gpu_wait_us=%llu targets=%u\n",
                    perf_frames, perf_frame_us / perf_frames, perf_frame_us ? 1000000.0 * perf_frames / (double)perf_frame_us : 0.0,
                    perf_draws / perf_frames, perf_quads / perf_frames, perf_clips / perf_frames, perf_gpu_wait / perf_frames, stat_targets);
            fflush(stderr);
            perf_frames = 0; perf_draws = perf_quads = perf_clips = perf_gpu_wait = perf_frame_us = 0;
            stat_targets = 0;
        }
    }
    stat_draws = stat_quads = stat_clips = 0;
}

/* Offscreen render targets (SubViewport). Drawn before the main scene of
 * the frame; the main scene waits on the fragment dependency. Coordinates
 * use the same 960x544 ortho space, mapped onto the target by the viewport. */
void *vita_gxm_target_create(int width, int height) {
    if (!initialized || width < 1 || height < 1 || width > 2048 || height > 2048) return NULL;
    GXMTexture *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->texture = vita2d_create_empty_texture_rendertarget(width, height, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
    if (!t->texture) { free(t); return NULL; }
    t->bytes = vita_texture_allocation_bytes(width, height, 4);
    texture_bytes += t->bytes;
    return t;
}
int vita_gxm_target_begin(void *target, uint32_t clear_color) {
    GXMTexture *t = target;
    if (!initialized || !t || drawing || target_drawing) return 0;
    /* Targets are the first scene of a frame: release the previous one. */
    wait_gpu();
    vita2d_start_drawing_advanced(t->texture, SCE_GXM_SCENE_FRAGMENT_SET_DEPENDENCY);
    SceGxmContext *ctx = vita2d_get_context();
    float w = (float)vita2d_texture_get_width(t->texture), h = (float)vita2d_texture_get_height(t->texture);
    sceGxmSetViewport(ctx, w * 0.5f, w * 0.5f, h * 0.5f, -h * 0.5f, 0.5f, 0.5f);
    vita2d_disable_clipping();
    vita2d_set_clear_color(clear_color);
    vita2d_clear_screen();
    target_drawing = 1;
    stat_targets++;
    return 1;
}
void vita_gxm_target_end(void) {
    if (!target_drawing) return;
    flush_batch();
    vita2d_end_drawing();
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetViewport(ctx, 480.0f, 480.0f, 272.0f, -272.0f, 0.5f, 0.5f);
    target_drawing = 0;
    target_dependency = 1;
}
void vita_gxm_finish(void) {
    if (!initialized) return;
    vita_gxm_end(0);
    wait_gpu();
    if (mesh_arena) { gpu_free(mesh_arena_uid); mesh_arena = NULL; mesh_arena_used = 0; }
    if (white_texture) { vita2d_free_texture(white_texture); white_texture = NULL; }
    vita2d_fini();
    initialized = 0;
}
/* Prototype reuses the pinned libvita2d vertex programs with a real MVP matrix.
 * Their shader computes homogeneous clip coordinates; GXM performs clipping,
 * perspective divide, interpolation and depth testing. This private symbol is
 * deliberately isolated here until a standalone GXM shader cache replaces it. */
extern float _vita2d_ortho_matrix[16];
void vita_gxm_scene3d_end(void) {
    if (!scene3d_drawing) return;
    flush_batch();
    memcpy(_vita2d_ortho_matrix, canvas_matrix, sizeof(canvas_matrix));
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetFrontDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
    scene3d_drawing = 0;
}
static void begin_scene3d(void) {
    if (scene3d_drawing) return;
    SceGxmContext *ctx = vita2d_get_context();
    memcpy(canvas_matrix, _vita2d_ortho_matrix, sizeof(canvas_matrix));
    sceGxmSetViewport(ctx, 480.0f, 480.0f, 272.0f, -272.0f, 0.5f, 0.5f);
    sceGxmSetCullMode(ctx, SCE_GXM_CULL_NONE);
    sceGxmSetFrontDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetBackDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
    sceGxmSetFrontDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_ENABLED);
    sceGxmSetBackDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_ENABLED);
    scene3d_drawing = 1;
}
int vita_gxm_triangles3d(void *texture, const float *xyz, const float *uv, unsigned count, const float *mvp, uint32_t color) {
    if (!drawing || !scene3d_enabled || !xyz || !mvp || !count || count > 3072 || count % 3 || (texture && !uv)) return 0;
    void *vertices = vita2d_pool_memalign(count * (texture ? sizeof(vita2d_texture_vertex) : sizeof(vita2d_color_vertex)), 8);
    if (!vertices) return 0;
    flush_batch();
    begin_scene3d();
    memcpy(_vita2d_ortho_matrix, mvp, sizeof(canvas_matrix));
    if (texture) {
        GXMTexture *t = texture;
        vita2d_texture_vertex *v = vertices;
        for (unsigned i = 0; i < count; i++) {
            v[i].x = xyz[i*3]; v[i].y = xyz[i*3+1]; v[i].z = xyz[i*3+2];
            v[i].u = uv[i*2]; v[i].v = uv[i*2+1];
        }
        vita2d_texture_set_filters(t->texture, SCE_GXM_TEXTURE_FILTER_POINT, SCE_GXM_TEXTURE_FILTER_POINT);
        vita2d_draw_array_textured(t->texture, SCE_GXM_PRIMITIVE_TRIANGLES, v, count, color);
    } else {
        vita2d_color_vertex *v = vertices;
        for (unsigned i = 0; i < count; i++) {
            v[i].x = xyz[i*3]; v[i].y = xyz[i*3+1]; v[i].z = xyz[i*3+2]; v[i].color = color;
        }
        vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLES, v, count);
    }
    return 1;
}
void *vita_gxm_mesh_create(const float *xyz, const float *uv, unsigned count) {
    if (!initialized || !scene3d_enabled || !xyz || !count || count % 3 || mesh_arena_failed) return NULL;
    if (!mesh_arena) {
        mesh_arena = gpu_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, MESH_ARENA_BYTES, 16,
                SCE_GXM_MEMORY_ATTRIB_READ, &mesh_arena_uid);
        if (!mesh_arena) {
            /* Keep the CPU path working when USER memory is short. */
            mesh_arena_failed = 1;
            fprintf(stderr, "[gxm3d] mesh arena allocation failed bytes=%u; using CPU vertex path\n", MESH_ARENA_BYTES);
            fflush(stderr);
            return NULL;
        }
        log_free_memory("mesh_arena");
    }
    unsigned bytes = (count * sizeof(vita2d_texture_vertex) + 15u) & ~15u;
    unsigned header = (sizeof(GXMMesh) + 15u) & ~15u;
    if (mesh_arena_used + header + bytes > MESH_ARENA_BYTES) return NULL;
    GXMMesh *mesh = (GXMMesh *)(mesh_arena + mesh_arena_used);
    mesh->vertices = (vita2d_texture_vertex *)(mesh_arena + mesh_arena_used + header);
    mesh->count = count;
    for (unsigned i = 0; i < count; i++) {
        vita2d_texture_vertex *v = &mesh->vertices[i];
        v->x = xyz[i * 3]; v->y = xyz[i * 3 + 1]; v->z = xyz[i * 3 + 2];
        v->u = uv ? uv[i * 2] : 0.0f; v->v = uv ? uv[i * 2 + 1] : 0.0f;
    }
    mesh_arena_used += header + bytes;
    return mesh;
}
int vita_gxm_mesh_draw(void *handle, void *texture, const float *mvp, uint32_t color) {
    GXMMesh *mesh = handle;
    if (!drawing || !scene3d_enabled || !mesh || !mvp) return 0;
    if (!texture && !white_texture) {
        white_texture = vita2d_create_empty_texture_format(8, 8, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
        if (!white_texture) return 0;
        memset(vita2d_texture_get_datap(white_texture), 0xff, vita2d_texture_get_stride(white_texture) * 8);
    }
    vita2d_texture *t = texture ? ((GXMTexture *)texture)->texture : white_texture;
    flush_batch();
    begin_scene3d();
    memcpy(_vita2d_ortho_matrix, mvp, sizeof(canvas_matrix));
    vita2d_texture_set_filters(t, SCE_GXM_TEXTURE_FILTER_POINT, SCE_GXM_TEXTURE_FILTER_POINT);
    /* vita2d's linear index buffer holds 65535 entries. */
    for (unsigned offset = 0; offset < mesh->count; offset += 65532u) {
        unsigned batch = mesh->count - offset < 65532u ? mesh->count - offset : 65532u;
        vita2d_draw_array_textured(t, SCE_GXM_PRIMITIVE_TRIANGLES, mesh->vertices + offset, batch, color);
    }
    return 1;
}
/* Rewrites a mesh in its existing arena slot (same vertex count). */
int vita_gxm_mesh_update(void *handle, const float *xyz, const float *uv, unsigned count) {
    GXMMesh *mesh = handle;
    if (!mesh || !xyz || count != mesh->count) return 0;
    for (unsigned i = 0; i < count; i++) {
        vita2d_texture_vertex *v = &mesh->vertices[i];
        v->x = xyz[i * 3]; v->y = xyz[i * 3 + 1]; v->z = xyz[i * 3 + 2];
        v->u = uv ? uv[i * 2] : 0.0f; v->v = uv ? uv[i * 2 + 1] : 0.0f;
    }
    return 1;
}
void vita_gxm_mesh_reset(void) {
    mesh_arena_used = 0;
}
unsigned vita_gxm_mesh_arena_used(void) {
    return mesh_arena_used;
}
unsigned vita_gxm_last_gpu_wait_us(void) {
    return last_gpu_wait_us;
}
void vita_gxm_set_vsync(int enabled) {
    vita2d_set_vblank_wait(enabled ? 1 : 0);
}
/* Blended 3D overlays (decals) test depth but must not write it. */
void vita_gxm_depth_write(int enabled) {
    if (!drawing || !scene3d_enabled) return;
    begin_scene3d();
    SceGxmContext *ctx = vita2d_get_context();
    SceGxmDepthWriteMode mode = enabled ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED;
    sceGxmSetFrontDepthWriteEnable(ctx, mode);
    sceGxmSetBackDepthWriteEnable(ctx, mode);
}
/* Per-vertex lighting pass: vita2d's own color shaders, recompiled by the
 * shader patcher with a 2x multiply blend (dst = 2 * src * dst). The first
 * pass draws the albedo texture; this pass multiplies it by vertex colors
 * that store light / 2, so 0x80 leaves a pixel unchanged and 0xff doubles it. */
extern const SceGxmProgram color_v_gxp_start;
extern const SceGxmProgram color_f_gxp_start;
extern SceGxmVertexProgram *_vita2d_colorVertexProgram;
extern const SceGxmProgramParameter *_vita2d_colorWvpParam;
static SceGxmShaderPatcherId light_fragment_id;
static SceGxmFragmentProgram *light_fragment_program;
static int light_program_failed;
typedef struct GXMLightMesh {
    vita2d_color_vertex *vertices;
    unsigned count;
} GXMLightMesh;
static int ensure_light_program(void) {
    if (light_fragment_program) return 1;
    if (light_program_failed) return 0;
    SceGxmShaderPatcher *patcher = vita2d_get_shader_patcher();
    int err = patcher ? sceGxmShaderPatcherRegisterProgram(patcher, &color_f_gxp_start, &light_fragment_id) : -1;
    if (err >= 0) {
        static const SceGxmBlendInfo blend = {
            .colorMask = SCE_GXM_COLOR_MASK_ALL,
            .colorFunc = SCE_GXM_BLEND_FUNC_ADD,
            .alphaFunc = SCE_GXM_BLEND_FUNC_ADD,
            .colorSrc = SCE_GXM_BLEND_FACTOR_DST_COLOR,
            .colorDst = SCE_GXM_BLEND_FACTOR_SRC_COLOR,
            .alphaSrc = SCE_GXM_BLEND_FACTOR_ZERO,
            .alphaDst = SCE_GXM_BLEND_FACTOR_ONE,
        };
        err = sceGxmShaderPatcherCreateFragmentProgram(patcher, light_fragment_id,
                SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE,
                &blend, &color_v_gxp_start, &light_fragment_program);
    }
    fprintf(stderr, "[gxm3d] vertex lighting program rc=0x%08x\n", (unsigned)err);
    fflush(stderr);
    if (err < 0 || !light_fragment_program) {
        light_fragment_program = NULL;
        light_program_failed = 1;
        return 0;
    }
    return 1;
}
static void draw_light_vertices(const vita2d_color_vertex *vertices, unsigned count, const float *mvp) {
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetVertexProgram(ctx, _vita2d_colorVertexProgram);
    sceGxmSetFragmentProgram(ctx, light_fragment_program);
    void *uniforms;
    sceGxmReserveVertexDefaultUniformBuffer(ctx, &uniforms);
    sceGxmSetUniformDataF(uniforms, _vita2d_colorWvpParam, 0, 16, mvp);
    for (unsigned offset = 0; offset < count; offset += 65532u) {
        unsigned batch = count - offset < 65532u ? count - offset : 65532u;
        sceGxmSetVertexStream(ctx, 0, vertices + offset);
        sceGxmDraw(ctx, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, vita2d_get_linear_indices(), batch);
    }
}
int vita_gxm_vertex_lighting_available(void) {
    return initialized && scene3d_enabled && ensure_light_program();
}
void *vita_gxm_light_mesh_create(const float *xyz, unsigned count) {
    if (!mesh_arena || !xyz || !count || count % 3 || !ensure_light_program()) return NULL;
    unsigned bytes = (count * sizeof(vita2d_color_vertex) + 15u) & ~15u;
    unsigned header = (sizeof(GXMLightMesh) + 15u) & ~15u;
    if (mesh_arena_used + header + bytes > MESH_ARENA_BYTES) return NULL;
    GXMLightMesh *mesh = (GXMLightMesh *)(mesh_arena + mesh_arena_used);
    mesh->vertices = (vita2d_color_vertex *)(mesh_arena + mesh_arena_used + header);
    mesh->count = count;
    for (unsigned i = 0; i < count; i++) {
        vita2d_color_vertex *v = &mesh->vertices[i];
        v->x = xyz[i * 3]; v->y = xyz[i * 3 + 1]; v->z = xyz[i * 3 + 2];
        v->color = 0xff808080u;
    }
    mesh_arena_used += header + bytes;
    return mesh;
}
/* Called while the CPU builds a frame; vita_gxm_end waited for the GPU, so
 * the previous frame no longer reads these vertices. */
void vita_gxm_light_mesh_set_colors(void *handle, const uint32_t *colors) {
    GXMLightMesh *mesh = handle;
    if (!mesh || !colors) return;
    for (unsigned i = 0; i < mesh->count; i++) mesh->vertices[i].color = colors[i];
}
int vita_gxm_light_mesh_draw(void *handle, const float *mvp) {
    GXMLightMesh *mesh = handle;
    if (!drawing || !mesh || !mvp || !ensure_light_program()) return 0;
    begin_scene3d();
    draw_light_vertices(mesh->vertices, mesh->count, mvp);
    return 1;
}
/* Additive blending for glows and flares (the original shader uses blend_add). */
void vita_gxm_blend_add(int enabled) {
    vita2d_set_blend_mode_add(enabled ? 1 : 0);
}

/* Tilt-shift blur. The 3D pass renders into a full-size target;
 * resolve() halves it three times (each step a bilinear 2x2 box), softens the
 * quarter and eighth levels with a 4-tap tent, copies the sharp frame to the
 * display and blends the soft levels in through destination-alpha ramps that
 * grow toward the top and bottom edges. vita2d's own shaders are reused with
 * blend states that write colour only, so target alpha stays at 1. */
extern const SceGxmProgram texture_v_gxp_start;
extern const SceGxmProgram texture_f_gxp_start;
extern const SceGxmProgram texture_tint_f_gxp_start;
extern SceGxmVertexProgram *_vita2d_textureVertexProgram;
extern const SceGxmProgramParameter *_vita2d_textureWvpParam;
extern const SceGxmProgramParameter *_vita2d_textureTintColorParam;
enum { TILT_FULL, TILT_HALF, TILT_QUARTER, TILT_QUARTER_SOFT, TILT_EIGHTH, TILT_EIGHTH_SOFT, TILT_COUNT };
static const unsigned tilt_sizes[TILT_COUNT][2] = { {960, 544}, {480, 272}, {240, 136}, {240, 136}, {120, 68}, {120, 68} };
static vita2d_texture *tilt_targets[TILT_COUNT];
static SceGxmFragmentProgram *tilt_copy, *tilt_mix, *tilt_dst_mix, *tilt_alpha;
static int tilt_failed;
static float tilt_ortho[16];
#define TILT_RGB (SCE_GXM_COLOR_MASK_R | SCE_GXM_COLOR_MASK_G | SCE_GXM_COLOR_MASK_B)

static int tilt_fragment(SceGxmShaderPatcherId id, SceGxmColorMask mask, SceGxmBlendFactor src, SceGxmBlendFactor dst,
        const SceGxmProgram *vertex, SceGxmFragmentProgram **out) {
    SceGxmBlendInfo blend = {
        .colorMask = mask,
        .colorFunc = SCE_GXM_BLEND_FUNC_ADD,
        .alphaFunc = SCE_GXM_BLEND_FUNC_ADD,
        .colorSrc = src,
        .colorDst = dst,
        .alphaSrc = src,
        .alphaDst = dst,
    };
    return sceGxmShaderPatcherCreateFragmentProgram(vita2d_get_shader_patcher(), id,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, &blend, vertex, out);
}

static int tilt_ready(void) {
    if (tilt_alpha) return 1;
    if (tilt_failed) return 0;
    int err = 0;
    for (int i = 0; i < TILT_COUNT && err >= 0; i++) {
        vita2d_texture *t = vita2d_create_empty_texture_rendertarget(tilt_sizes[i][0], tilt_sizes[i][1], SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
        if (!t) { err = -1; break; }
        memset(vita2d_texture_get_datap(t), 0xff, vita2d_texture_get_stride(t) * tilt_sizes[i][1]);
        vita2d_texture_set_filters(t, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);
        sceGxmTextureSetUAddrMode(&t->gxm_tex, SCE_GXM_TEXTURE_ADDR_CLAMP);
        sceGxmTextureSetVAddrMode(&t->gxm_tex, SCE_GXM_TEXTURE_ADDR_CLAMP);
        tilt_targets[i] = t;
        texture_bytes += vita_texture_allocation_bytes(tilt_sizes[i][0], tilt_sizes[i][1], 4);
    }
    SceGxmShaderPatcher *patcher = vita2d_get_shader_patcher();
    SceGxmShaderPatcherId texture_id, tint_id, color_id;
    if (err >= 0) err = patcher ? sceGxmShaderPatcherRegisterProgram(patcher, &texture_f_gxp_start, &texture_id) : -1;
    if (err >= 0) err = sceGxmShaderPatcherRegisterProgram(patcher, &texture_tint_f_gxp_start, &tint_id);
    if (err >= 0) err = sceGxmShaderPatcherRegisterProgram(patcher, &color_f_gxp_start, &color_id);
    if (err >= 0) err = tilt_fragment(texture_id, TILT_RGB, SCE_GXM_BLEND_FACTOR_ONE, SCE_GXM_BLEND_FACTOR_ZERO, &texture_v_gxp_start, &tilt_copy);
    if (err >= 0) err = tilt_fragment(tint_id, TILT_RGB, SCE_GXM_BLEND_FACTOR_SRC_ALPHA, SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, &texture_v_gxp_start, &tilt_mix);
    if (err >= 0) err = tilt_fragment(texture_id, TILT_RGB, SCE_GXM_BLEND_FACTOR_DST_ALPHA, SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, &texture_v_gxp_start, &tilt_dst_mix);
    if (err >= 0) err = tilt_fragment(color_id, SCE_GXM_COLOR_MASK_A, SCE_GXM_BLEND_FACTOR_ONE, SCE_GXM_BLEND_FACTOR_ZERO, &color_v_gxp_start, &tilt_alpha);
    fprintf(stderr, "[gxm3d] tilt-shift targets and programs rc=0x%08x\n", (unsigned)err);
    fflush(stderr);
    if (err < 0 || !tilt_copy || !tilt_mix || !tilt_dst_mix || !tilt_alpha) {
        for (int i = 0; i < TILT_COUNT; i++) {
            if (!tilt_targets[i]) continue;
            texture_bytes -= vita_texture_allocation_bytes(tilt_sizes[i][0], tilt_sizes[i][1], 4);
            vita2d_free_texture(tilt_targets[i]);
            tilt_targets[i] = NULL;
        }
        tilt_alpha = NULL;
        tilt_failed = 1;
        return 0;
    }
    memcpy(tilt_ortho, _vita2d_ortho_matrix, sizeof(tilt_ortho));
    return 1;
}

static void tilt_states(SceGxmContext *ctx, unsigned width, unsigned height) {
    sceGxmSetViewport(ctx, width * 0.5f, width * 0.5f, height * 0.5f, -(height * 0.5f), 0.5f, 0.5f);
    vita2d_disable_clipping();
    sceGxmSetFrontDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
}

/* Full-width band of a target, in the shared 960x544 ortho space. */
static void tilt_quad(SceGxmFragmentProgram *program, int source, float y0, float y1, float du, float dv, float alpha) {
    vita2d_texture_vertex *v = vita2d_pool_memalign(4 * sizeof(*v), 8);
    if (!v) return;
    const float xs[4] = {0.0f, 960.0f, 0.0f, 960.0f}, ys[4] = {y0, y0, y1, y1};
    for (int i = 0; i < 4; i++) {
        v[i].x = xs[i]; v[i].y = ys[i]; v[i].z = 0.5f;
        v[i].u = xs[i] / 960.0f + du; v[i].v = ys[i] / 544.0f + dv;
    }
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetVertexProgram(ctx, _vita2d_textureVertexProgram);
    sceGxmSetFragmentProgram(ctx, program);
    void *uniforms;
    sceGxmReserveVertexDefaultUniformBuffer(ctx, &uniforms);
    sceGxmSetUniformDataF(uniforms, _vita2d_textureWvpParam, 0, 16, tilt_ortho);
    if (program == tilt_mix) {
        const float tint[4] = {1.0f, 1.0f, 1.0f, alpha};
        sceGxmReserveFragmentDefaultUniformBuffer(ctx, &uniforms);
        sceGxmSetUniformDataF(uniforms, _vita2d_textureTintColorParam, 0, 4, tint);
    }
    sceGxmSetFragmentTexture(ctx, 0, &tilt_targets[source]->gxm_tex);
    sceGxmSetVertexStream(ctx, 0, v);
    sceGxmDraw(ctx, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, vita2d_get_linear_indices(), 4);
    stat_draws++;
}

static void tilt_begin(int target) {
    vita2d_start_drawing_advanced(tilt_targets[target], SCE_GXM_SCENE_FRAGMENT_SET_DEPENDENCY | SCE_GXM_SCENE_VERTEX_WAIT_FOR_DEPENDENCY);
    tilt_states(vita2d_get_context(), tilt_sizes[target][0], tilt_sizes[target][1]);
    stat_targets++;
}

static void tilt_downsample(int target, int source) {
    tilt_begin(target);
    tilt_quad(tilt_copy, source, 0.0f, 544.0f, 0.0f, 0.0f, 1.0f);
    vita2d_end_drawing();
}

/* Four bilinear taps on the source's texel corners: a 3x3 tent. Each tap
 * blends 1/n over the previous ones, so all four weigh the same. */
static void tilt_soften(int target, int source) {
    float du = 0.5f / tilt_sizes[source][0], dv = 0.5f / tilt_sizes[source][1];
    tilt_begin(target);
    tilt_quad(tilt_copy, source, 0.0f, 544.0f, -du, -dv, 1.0f);
    tilt_quad(tilt_mix, source, 0.0f, 544.0f, du, -dv, 1.0f / 2.0f);
    tilt_quad(tilt_mix, source, 0.0f, 544.0f, -du, dv, 1.0f / 3.0f);
    tilt_quad(tilt_mix, source, 0.0f, 544.0f, du, dv, 1.0f / 4.0f);
    vita2d_end_drawing();
}

/* Blur amount: 1 at the top and bottom screen edges, 0 inside the focus band. */
static float tilt_amount(float y, float top, float bottom) {
    float amount = 0.0f;
    if (top > 0.0f && y < top) amount = (top - y) / top;
    if (bottom < 1.0f && y > bottom) {
        float below = (y - bottom) / (1.0f - bottom);
        if (below > amount) amount = below;
    }
    return amount;
}

/* Writes destination alpha over [r0, r1] of the screen: the weight of one
 * blur level, ramping in over [from, to] of the blur amount. */
static void tilt_ramp(float r0, float r1, float top, float bottom, float strength, float from, float to) {
    enum { SEGMENTS = 16 };
    vita2d_color_vertex *v = vita2d_pool_memalign(SEGMENTS * 6 * sizeof(*v), 8);
    if (!v) return;
    for (int s = 0; s < SEGMENTS; s++) {
        float ya = r0 + (r1 - r0) * s / SEGMENTS, yb = r0 + (r1 - r0) * (s + 1) / SEGMENTS;
        float wa = (tilt_amount(ya, top, bottom) * strength - from) / (to - from);
        float wb = (tilt_amount(yb, top, bottom) * strength - from) / (to - from);
        wa = wa < 0.0f ? 0.0f : (wa > 1.0f ? 1.0f : wa);
        wb = wb < 0.0f ? 0.0f : (wb > 1.0f ? 1.0f : wb);
        uint32_t ca = ((uint32_t)(wa * 255.0f + 0.5f) << 24) | 0xffffffu;
        uint32_t cb = ((uint32_t)(wb * 255.0f + 0.5f) << 24) | 0xffffffu;
        const float xs[6] = {0.0f, 960.0f, 960.0f, 0.0f, 960.0f, 0.0f};
        const float ys[6] = {ya, ya, yb, ya, yb, yb};
        for (int i = 0; i < 6; i++) {
            vita2d_color_vertex *p = &v[s * 6 + i];
            p->x = xs[i]; p->y = ys[i] * 544.0f; p->z = 0.5f;
            p->color = ys[i] == ya ? ca : cb;
        }
    }
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetVertexProgram(ctx, _vita2d_colorVertexProgram);
    sceGxmSetFragmentProgram(ctx, tilt_alpha);
    void *uniforms;
    sceGxmReserveVertexDefaultUniformBuffer(ctx, &uniforms);
    sceGxmSetUniformDataF(uniforms, _vita2d_colorWvpParam, 0, 16, tilt_ortho);
    sceGxmSetVertexStream(ctx, 0, v);
    sceGxmDraw(ctx, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, vita2d_get_linear_indices(), SEGMENTS * 6);
    stat_draws++;
}

int vita_gxm_begin_offscreen(uint32_t color) {
    static int skip_logged;
    if (initialized && (drawing || target_drawing) && !skip_logged) {
        skip_logged = 1;
        fprintf(stderr, "[gxm3d] tilt-shift skipped: frame already begun before the 3D pass\n");
        fflush(stderr);
    }
    if (!initialized || drawing || target_drawing || !scene3d_enabled || !tilt_ready()) return 0;
    if ((memory_frame++ % 600) == 0) log_free_memory("frame");
    wait_gpu();
    vita2d_start_drawing_advanced(tilt_targets[TILT_FULL],
            SCE_GXM_SCENE_FRAGMENT_SET_DEPENDENCY | (target_dependency ? SCE_GXM_SCENE_VERTEX_WAIT_FOR_DEPENDENCY : 0));
    target_dependency = 0;
    vita2d_disable_clipping();
    clip_enabled_cache = 0;
    vita2d_set_clear_color(color);
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetViewport(ctx, 480.0f, 480.0f, 272.0f, -272.0f, 0.5f, 0.5f);
    sceGxmSetFrontDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetBackDepthFunc(ctx, SCE_GXM_DEPTH_FUNC_ALWAYS);
    sceGxmSetFrontDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_ENABLED);
    sceGxmSetBackDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_ENABLED);
    vita2d_clear_screen();
    sceGxmSetFrontDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
    sceGxmSetBackDepthWriteEnable(ctx, SCE_GXM_DEPTH_WRITE_DISABLED);
    drawing = 1;
    tilt_offscreen = 1;
    return 1;
}

void vita_gxm_tiltshift_resolve(float top, float bottom, float strength) {
    if (!tilt_offscreen) return;
    tilt_offscreen = 0;
    flush_batch();
    vita_gxm_scene3d_end();
    vita2d_disable_clipping();
    clip_enabled_cache = 0;
    vita2d_end_drawing();
    top = top < 0.0f ? 0.0f : (top > 1.0f ? 1.0f : top);
    bottom = bottom < top ? top : (bottom > 1.0f ? 1.0f : bottom);
    int blur = strength > 0.0f && (top > 0.0f || bottom < 1.0f);
    if (blur) {
        tilt_downsample(TILT_HALF, TILT_FULL);
        tilt_downsample(TILT_QUARTER, TILT_HALF);
        tilt_soften(TILT_QUARTER_SOFT, TILT_QUARTER);
        tilt_downsample(TILT_EIGHTH, TILT_QUARTER_SOFT);
        tilt_soften(TILT_EIGHTH_SOFT, TILT_EIGHTH);
    }
    vita2d_start_drawing_advanced(NULL, SCE_GXM_SCENE_VERTEX_WAIT_FOR_DEPENDENCY);
    tilt_states(vita2d_get_context(), 960, 544);
    tilt_quad(tilt_copy, TILT_FULL, 0.0f, 544.0f, 0.0f, 0.0f, 1.0f);
    if (blur) {
        const int levels[2] = {TILT_QUARTER_SOFT, TILT_EIGHTH_SOFT};
        const float from[2] = {0.0f, 0.4f}, to[2] = {0.5f, 1.0f};
        for (int level = 0; level < 2; level++) {
            if (top > 0.0f) tilt_ramp(0.0f, top, top, bottom, strength, from[level], to[level]);
            if (bottom < 1.0f) tilt_ramp(bottom, 1.0f, top, bottom, strength, from[level], to[level]);
            if (top > 0.0f) tilt_quad(tilt_dst_mix, levels[level], 0.0f, top * 544.0f, 0.0f, 0.0f, 1.0f);
            if (bottom < 1.0f) tilt_quad(tilt_dst_mix, levels[level], bottom * 544.0f, 544.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}
