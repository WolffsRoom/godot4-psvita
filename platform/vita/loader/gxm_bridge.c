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
/* Scene guard. vita2d ignores sceGxmBeginScene failures and keeps drawing;
 * the first default-uniform reservation then writes through an invalid
 * pointer inside SceGxm (data abort). A reservation only succeeds inside a
 * scene that really began: on failure end it, drain the GPU and retry once,
 * then skip the scene rather than crash. */
extern SceGxmVertexProgram *_vita2d_colorVertexProgram;
static int scene_began(void) {
    SceGxmContext *ctx = vita2d_get_context();
    sceGxmSetVertexProgram(ctx, _vita2d_colorVertexProgram);
    void *probe = NULL;
    return sceGxmReserveVertexDefaultUniformBuffer(ctx, &probe) >= 0 && probe != NULL;
}
static int start_scene(vita2d_texture *target, unsigned flags, const char *where) {
    static unsigned failures;
    vita2d_start_drawing_advanced(target, flags);
    if (scene_began()) return 1;
    fprintf(stderr, "[gxm2d] scene failed to begin at %s flags=0x%x failures=%u; retrying\n", where, flags, ++failures);
    fflush(stderr);
    vita2d_end_drawing();
    sceGxmFinish(vita2d_get_context());
    vita2d_start_drawing_advanced(target, flags & ~SCE_GXM_SCENE_VERTEX_WAIT_FOR_DEPENDENCY);
    if (scene_began()) return 1;
    fprintf(stderr, "[gxm2d] scene still failed at %s; skipping it\n", where);
    fflush(stderr);
    vita2d_end_drawing();
    return 0;
}
void vita_gxm_begin(uint32_t color) {
    if (!initialized || drawing) return;
    if ((memory_frame++ % 600) == 0) log_free_memory("frame");
    wait_gpu();
    /* Not vita2d_start_drawing(): it resets the vertex pool, which an
     * offscreen scene earlier in this frame may still be reading. The pool
     * is reset by wait_gpu() once the previous frame is done. */
    int began = start_scene(NULL, target_dependency ? SCE_GXM_SCENE_VERTEX_WAIT_FOR_DEPENDENCY : 0, "display");
    target_dependency = 0;
    if (!began) return;
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
    if (!drawing) return; /* vita2d's clip draws stencil rectangles: only inside a scene. */
    /* A full-screen clip is no clip: skip vita2d's two stencil rectangles. */
    if (enabled && x <= 0 && y <= 0 && x + width >= 960 && y + height >= 544) enabled = 0;
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
void vita_gxm_end(int present) {
    if (!drawing) return;
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
    if (!start_scene(t->texture, SCE_GXM_SCENE_FRAGMENT_SET_DEPENDENCY, "subviewport")) return 0;
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
