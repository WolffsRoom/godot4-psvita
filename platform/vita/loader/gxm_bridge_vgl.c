/* Experimental VitaGL transport for the native Godot canvas (A/B backend).
 *
 * Implements the same vita_gxm_* contract as gxm_bridge.c, so libgodot.so and
 * gxm_canvas_vita.inc stay byte-for-byte identical: only eboot.bin changes.
 * The goal is a fair performance comparison between libvita2d/GXM and VitaGL
 * for the 2D canvas, not a replacement of the native path.
 *
 * Godot's GLES3 shaders are NOT used. Two tiny GLSL ES 1.00 programs (colour
 * and textured+tint), compiled once at init by VitaGL's runtime compiler,
 * draw the CPU-transformed canvas vertices. This needs libshacccg.suprx on
 * the console, like the earlier VitaGL probes; the native path does not.
 *
 * Scope: canvas 2D (quads, textures, clip, SubViewport targets, boot splash).
 * The GXM 3D prototype entry points are stubs that report "unavailable"; a
 * package with gxm3d.cfg is refused at init instead of rendering wrongly.
 *
 * Optional app0:vgl2d.cfg lines (key=value), all integers:
 *   ram_threshold_mb, cdram_threshold_mb, phycont_threshold_mb,
 *   circular_pool_mb, target_flip (1 = GL bottom-up FBO convention).
 */
#include "../gxm_bridge.h"
#include "texture_format.h"
#include <vitaGL.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct VGLTexture {
    GLuint id;
    GLuint fbo;
    int width, height;
    int filter; /* -1 unknown, 0 nearest, 1 linear */
    size_t bytes;
} VGLTexture;

typedef struct { float x, y; uint32_t color; } VGLColorVertex;
typedef struct { float x, y, u, v; } VGLTexVertex;

static int initialized;
static int context_ready; /* vglInit* ran: GXM stays owned by VitaGL */
static GLuint current_fbo;
static int drawing;
static int target_drawing;
static int texture_rgba4444;
static size_t texture_bytes;
static unsigned texture_upload_count;
static unsigned memory_frame;
static int blend_add;

/* Configuration (app0:vgl2d.cfg). Defaults keep headroom for thread stacks
 * and kernel allocations made after the renderer starts: VitaGL otherwise
 * claims every free byte of each pool at init. */
static int cfg_ram_threshold_mb = 32;
static int cfg_cdram_threshold_mb = 0;
static int cfg_phycont_threshold_mb = 8;
static int cfg_circular_pool_mb = 8;
static int cfg_target_flip = 1;

/* Programs. Attribute 0 = position, attribute 1 = colour or uv. */
static GLuint color_program, texture_program;
static GLint color_mvp_loc, texture_mvp_loc, texture_tint_loc, texture_sampler_loc;
static float display_mvp[16], target_mvp[16];
static const float *current_mvp;
static GLuint bound_program;
static int mvp_dirty_color, mvp_dirty_texture;

/* Draw batching mirrors gxm_bridge.c: consecutive quads with the same
 * texture/filter/tint (or any flat-colour quads) become one draw. */
#define BATCH_MAX_VERTICES 65532u
enum { BATCH_NONE, BATCH_TEXTURED, BATCH_COLOR };
static int batch_kind;
static VGLTexture *batch_texture;
static int batch_linear;
static uint32_t batch_color;
static unsigned batch_count;
static uint8_t *staging; /* BATCH_MAX_VERTICES * sizeof(VGLTexVertex) */

/* Clip cache, as in the native bridge. */
static int clip_enabled_cache;
static int clip_rect_cache[4];

/* Statistics: same fields as [gxm2d-perf] plus submit_us, the CPU time spent
 * inside the GL draw submission (state validation, vertex copy, draw). */
#define PERF_FRAMES 300u
static unsigned stat_draws, stat_quads, stat_clips, stat_targets;
static unsigned long long stat_submit_us;
static unsigned long long perf_draws, perf_quads, perf_clips, perf_swap, perf_frame_us, perf_submit;
static unsigned perf_frames;
static SceUInt64 perf_last_end;
static unsigned last_swap_us;

static void log_free_memory(const char *stage) {
    SceKernelFreeMemorySizeInfo info = { 0 };
    info.size = sizeof(info);
    int result = sceKernelGetFreeMemorySize(&info);
    fprintf(stderr, "[vita-memory] stage=%s backend=vitagl result=0x%08x user_free=%u cdram_free=%u phycont_free=%u textures=%lu\n",
            stage, (unsigned int)result, info.size_user, info.size_cdram,
            info.size_phycont, (unsigned long)texture_bytes);
    fflush(stderr);
}

static void read_config(void) {
    FILE *f = fopen("app0:vgl2d.cfg", "rb");
    if (!f) return;
    char line[96];
    while (fgets(line, sizeof(line), f)) {
        char key[48];
        int value;
        if (sscanf(line, "%47[a-z_]=%d", key, &value) != 2 || value < 0 || value > 4096) continue;
        if (!strcmp(key, "ram_threshold_mb")) cfg_ram_threshold_mb = value;
        else if (!strcmp(key, "cdram_threshold_mb")) cfg_cdram_threshold_mb = value;
        else if (!strcmp(key, "phycont_threshold_mb")) cfg_phycont_threshold_mb = value;
        else if (!strcmp(key, "circular_pool_mb")) cfg_circular_pool_mb = value;
        else if (!strcmp(key, "target_flip")) cfg_target_flip = value != 0;
    }
    fclose(f);
}

/* Column-major orthographic matrix for the 960x544 canvas space. The
 * display uses y down. Without HAVE_UNFLIPPED_FBOS, VitaGL keeps GL's
 * bottom-up convention for framebuffer objects, so targets are flipped to
 * keep v=0 on the top row, which is what the canvas samples. */
static void ortho(float *m, int flip_y) {
    memset(m, 0, 16 * sizeof(float));
    m[0] = 2.0f / 960.0f;
    m[5] = flip_y ? 2.0f / 544.0f : -2.0f / 544.0f;
    m[10] = 1.0f;
    m[12] = -1.0f;
    m[13] = flip_y ? -1.0f : 1.0f;
    m[15] = 1.0f;
}

static const char *color_vs =
    "uniform mat4 u_mvp;\n"
    "attribute vec2 a_position;\n"
    "attribute vec4 a_color;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  v_color = a_color;\n"
    "  gl_Position = u_mvp * vec4(a_position, 0.5, 1.0);\n"
    "}\n";
static const char *color_fs =
    "precision mediump float;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_FragColor = v_color;\n"
    "}\n";
static const char *texture_vs =
    "uniform mat4 u_mvp;\n"
    "attribute vec2 a_position;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "  v_uv = a_uv;\n"
    "  gl_Position = u_mvp * vec4(a_position, 0.5, 1.0);\n"
    "}\n";
static const char *texture_fs =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_texture;\n"
    "uniform vec4 u_tint;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(u_texture, v_uv) * u_tint;\n"
    "}\n";

static GLuint compile_stage(GLenum type, const char *source, const char *tag) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char info[256] = { 0 };
        GLsizei n = 0;
        glGetShaderInfoLog(shader, sizeof(info) - 1, &n, info);
        fprintf(stderr, "[vgl2d] %s compile failed: %s\n", tag, n > 0 ? info : "(no log)");
        fflush(stderr);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/* VGL_MODE_SHADER_PAIR expects vertex + fragment compiled back to back and
 * linked together, which is exactly this sequence. */
static GLuint build_program(const char *vs, const char *fs, const char *attr1, const char *tag) {
    GLuint v = compile_stage(GL_VERTEX_SHADER, vs, tag);
    GLuint f = v ? compile_stage(GL_FRAGMENT_SHADER, fs, tag) : 0;
    if (!v || !f) return 0;
    GLuint program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glBindAttribLocation(program, 0, "a_position");
    glBindAttribLocation(program, 1, attr1);
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    fprintf(stderr, "[vgl2d] program %s link=%d\n", tag, linked);
    fflush(stderr);
    if (!linked) {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static void use_program(GLuint program) {
    if (bound_program != program) {
        glUseProgram(program);
        bound_program = program;
    }
    if (program == color_program && mvp_dirty_color) {
        glUniformMatrix4fv(color_mvp_loc, 1, GL_FALSE, current_mvp);
        mvp_dirty_color = 0;
    } else if (program == texture_program && mvp_dirty_texture) {
        glUniformMatrix4fv(texture_mvp_loc, 1, GL_FALSE, current_mvp);
        mvp_dirty_texture = 0;
    }
}

static void set_mvp(const float *m) {
    current_mvp = m;
    mvp_dirty_color = mvp_dirty_texture = 1;
}

static void apply_blend(void) {
    /* libvita2d: SRC_ALPHA / ONE_MINUS_SRC_ALPHA for colour and alpha;
     * additive mode is ONE / ONE. */
    if (blend_add) glBlendFunc(GL_ONE, GL_ONE);
    else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void flush_batch(void) {
    if (batch_count) {
        SceUInt64 started = sceKernelGetProcessTimeWide();
        if (batch_kind == BATCH_TEXTURED) {
            VGLTexture *t = batch_texture;
            use_program(texture_program);
            glBindTexture(GL_TEXTURE_2D, t->id);
            if (t->filter != batch_linear) {
                GLint filter = batch_linear ? GL_LINEAR : GL_NEAREST;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
                t->filter = batch_linear;
            }
            float tint[4] = {
                (float)(batch_color & 0xff) / 255.0f, (float)((batch_color >> 8) & 0xff) / 255.0f,
                (float)((batch_color >> 16) & 0xff) / 255.0f, (float)(batch_color >> 24) / 255.0f,
            };
            glUniform4fv(texture_tint_loc, 1, tint);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VGLTexVertex), staging);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VGLTexVertex), staging + 2 * sizeof(float));
        } else {
            use_program(color_program);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VGLColorVertex), staging);
            glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VGLColorVertex), staging + 2 * sizeof(float));
        }
        /* Client-side arrays: VitaGL copies them into its circular pool. */
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)batch_count);
        stat_submit_us += sceKernelGetProcessTimeWide() - started;
        stat_draws++;
    }
    batch_kind = BATCH_NONE;
    batch_count = 0;
    batch_texture = NULL;
}

/* Returns where the next 6 vertices go, flushing when the state changes. */
static void *batch_reserve(int kind, VGLTexture *t, int linear, uint32_t color) {
    stat_quads++;
    if (batch_count && (batch_kind != kind || batch_count + 6 > BATCH_MAX_VERTICES ||
            (kind == BATCH_TEXTURED && (batch_texture != t || batch_linear != linear || batch_color != color)))) {
        flush_batch();
    }
    if (!batch_count) {
        batch_kind = kind;
        batch_texture = t;
        batch_linear = linear;
        batch_color = color;
    }
    size_t size = kind == BATCH_TEXTURED ? sizeof(VGLTexVertex) : sizeof(VGLColorVertex);
    void *v = staging + (size_t)batch_count * size;
    batch_count += 6;
    return v;
}

int vita_gxm_init(void) {
    if (initialized) return 1;
    FILE *scene_file = fopen("app0:gxm3d.cfg", "rb");
    if (scene_file) {
        fclose(scene_file);
        fprintf(stderr, "[vgl2d] gxm3d.cfg present: the VitaGL A/B backend is 2D only; refusing to start\n");
        fflush(stderr);
        return 0;
    }
    texture_rgba4444 = 0;
    FILE *format_file = fopen("app0:gxm_texture_format.cfg", "rb");
    if (format_file) {
        char format[32] = { 0 };
        if (fgets(format, sizeof(format), format_file)) texture_rgba4444 = strcmp(format, "RGBA4444\n") == 0;
        fclose(format_file);
    }
    read_config();
    staging = malloc(BATCH_MAX_VERTICES * sizeof(VGLTexVertex));
    if (!staging) return 0;
    if (context_ready) goto programs;
    log_free_memory("before_vglinit");
    /* Same binding mode and parameter buffer as gl_manager_vita.cpp, but no
     * MSAA: libvita2d renders without it, and 4x would skew the comparison. */
    vglSetSemanticBindingMode(VGL_MODE_SHADER_PAIR);
    vglSetParamBufferSize(4 * 1024 * 1024);
    vglSetCircularPoolSize((uint32_t)cfg_circular_pool_mb * 1024u * 1024u);
    vglUseTripleBuffering(GL_FALSE);
    vglInitWithCustomThreshold(0, 960, 544,
            cfg_ram_threshold_mb * 1024 * 1024, cfg_cdram_threshold_mb * 1024 * 1024,
            cfg_phycont_threshold_mb * 1024 * 1024, 0, SCE_GXM_MULTISAMPLE_NONE);
    context_ready = 1;
programs:
    color_program = build_program(color_vs, color_fs, "a_color", "color");
    texture_program = build_program(texture_vs, texture_fs, "a_uv", "texture");
    if (!color_program || !texture_program) {
        /* GXM is already owned by VitaGL: libvita2d cannot take over now. */
        fprintf(stderr, "[vgl2d] shader setup failed (is libshacccg.suprx installed?)\n");
        fflush(stderr);
        free(staging);
        staging = NULL;
        return 0;
    }
    color_mvp_loc = glGetUniformLocation(color_program, "u_mvp");
    texture_mvp_loc = glGetUniformLocation(texture_program, "u_mvp");
    texture_tint_loc = glGetUniformLocation(texture_program, "u_tint");
    texture_sampler_loc = glGetUniformLocation(texture_program, "u_texture");
    glUseProgram(texture_program);
    glUniform1i(texture_sampler_loc, 0);
    bound_program = texture_program;
    ortho(display_mvp, 0);
    ortho(target_mvp, cfg_target_flip);
    set_mvp(display_mvp);
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    apply_blend();
    initialized = 1;
    fprintf(stderr, "[vgl2d] VitaGL canvas backend ready texture_format=%s ram_threshold_mb=%d cdram_threshold_mb=%d phycont_threshold_mb=%d circular_pool_mb=%d target_flip=%d\n",
            texture_rgba4444 ? "RGBA4444" : "RGBA8888", cfg_ram_threshold_mb, cfg_cdram_threshold_mb,
            cfg_phycont_threshold_mb, cfg_circular_pool_mb, cfg_target_flip);
    fflush(stderr);
    log_free_memory("renderer_init");
    return 1;
}

void *vita_gxm_texture_create(int width, int height, const uint8_t *rgba) {
    if (!initialized || !rgba || width < 1 || height < 1 || width > 4096 || height > 4096) return NULL;
    size_t bytes = vita_texture_allocation_bytes(width, height, texture_rgba4444 ? 2 : 4);
    if (bytes > 64 * 1024 * 1024 || texture_bytes > 64 * 1024 * 1024 - bytes) return NULL;
    VGLTexture *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    /* A pending batch is unaffected: flush_batch() binds its own texture. */
    glGenTextures(1, &t->id);
    if (!t->id) { free(t); return NULL; }
    glBindTexture(GL_TEXTURE_2D, t->id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (texture_rgba4444) {
        uint16_t *packed = malloc((size_t)width * height * sizeof(uint16_t));
        if (!packed) { glDeleteTextures(1, &t->id); free(t); return NULL; }
        for (size_t i = 0; i < (size_t)width * height; i++) packed[i] = vita_pack_rgba4444_gl(rgba + i * 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, packed);
        free(packed);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    t->filter = 0;
    t->width = width;
    t->height = height;
    t->bytes = bytes;
    texture_bytes += bytes;
    texture_upload_count++;
    if (texture_upload_count <= 16 || texture_upload_count % 128 == 0) {
        fprintf(stderr, "[vgl2d] texture uploads=%u last=%dx%d format=%s allocation=%lu budget=%lu\n", texture_upload_count, width, height,
                texture_rgba4444 ? "RGBA4444" : "RGBA8888", (unsigned long)bytes, (unsigned long)texture_bytes);
        fflush(stderr);
    }
    return t;
}

/* VitaGL defers the release of texture memory until the GPU is done with
 * it, so no retired list is needed here. A pending batch is flushed first
 * because it still references the texture by id. */
void vita_gxm_texture_free(void *texture) {
    VGLTexture *t = texture;
    if (!t) return;
    if (batch_texture == t) flush_batch();
    if (t->fbo && t->fbo == current_fbo) {
        flush_batch();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        current_fbo = 0;
        target_drawing = 0;
    }
    if (t->fbo) glDeleteFramebuffers(1, &t->fbo);
    glDeleteTextures(1, &t->id);
    texture_bytes -= t->bytes;
    free(t);
}

static const int corners[6] = { 0, 1, 2, 0, 2, 3 };

void vita_gxm_texture_quad(void *texture, const float *xy, const float *uv, uint32_t color, int linear) {
    VGLTexture *t = texture;
    if (!(drawing || target_drawing) || !t) return;
    VGLTexVertex *v = batch_reserve(BATCH_TEXTURED, t, linear ? 1 : 0, color);
    for (int i = 0; i < 6; i++) {
        int k = corners[i];
        v[i].x = xy[2 * k]; v[i].y = xy[2 * k + 1];
        v[i].u = uv[2 * k]; v[i].v = uv[2 * k + 1];
    }
}

void vita_gxm_quad(const float *xy, uint32_t color) {
    if (!(drawing || target_drawing)) return;
    VGLColorVertex *v = batch_reserve(BATCH_COLOR, NULL, 0, 0);
    for (int i = 0; i < 6; i++) {
        v[i].x = xy[corners[i] * 2];
        v[i].y = xy[corners[i] * 2 + 1];
        v[i].color = color;
    }
}

static void clear_to(uint32_t color) {
    glClearColor((float)(color & 0xff) / 255.0f, (float)((color >> 8) & 0xff) / 255.0f,
            (float)((color >> 16) & 0xff) / 255.0f, (float)(color >> 24) / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void vita_gxm_begin(uint32_t color) {
    if (!initialized || drawing) return;
    if ((memory_frame++ % 600) == 0) log_free_memory("frame");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    current_fbo = 0;
    glViewport(0, 0, 960, 544);
    glDisable(GL_SCISSOR_TEST);
    clip_enabled_cache = 0;
    set_mvp(display_mvp);
    clear_to(color);
    drawing = 1;
}

void vita_gxm_clip(int enabled, int x, int y, int width, int height) {
    if (target_drawing) return; /* Offscreen targets draw unclipped (native parity). */
    if (!drawing) return;
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
        /* GL scissor origin is bottom-left on the display. */
        glScissor(x, 544 - (y + height), width, height);
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void vita_gxm_end(int present) {
    if (!drawing) return;
    flush_batch();
    glDisable(GL_SCISSOR_TEST);
    clip_enabled_cache = 0;
    drawing = 0;
    if (!present) return;
    SceUInt64 swap_started = sceKernelGetProcessTimeWide();
    vglSwapBuffers(GL_FALSE);
    SceUInt64 now = sceKernelGetProcessTimeWide();
    last_swap_us = (unsigned)(now - swap_started);
    if (perf_last_end) perf_frame_us += now - perf_last_end;
    perf_last_end = now;
    perf_draws += stat_draws; perf_quads += stat_quads; perf_clips += stat_clips;
    perf_swap += last_swap_us; perf_submit += stat_submit_us;
    if (++perf_frames >= PERF_FRAMES) {
        fprintf(stderr, "[vgl2d-perf] frames=%u avg_frame_us=%llu fps=%.1f draws=%llu quads=%llu clips=%llu submit_us=%llu swap_us=%llu targets=%u\n",
                perf_frames, perf_frame_us / perf_frames, perf_frame_us ? 1000000.0 * perf_frames / (double)perf_frame_us : 0.0,
                perf_draws / perf_frames, perf_quads / perf_frames, perf_clips / perf_frames,
                perf_submit / perf_frames, perf_swap / perf_frames, stat_targets);
        fflush(stderr);
        perf_frames = 0; perf_draws = perf_quads = perf_clips = perf_swap = perf_frame_us = perf_submit = 0;
        stat_targets = 0;
    }
    stat_draws = stat_quads = stat_clips = 0;
    stat_submit_us = 0;
}

void *vita_gxm_target_create(int width, int height) {
    if (!initialized || width < 1 || height < 1 || width > 2048 || height > 2048) return NULL;
    VGLTexture *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    glGenTextures(1, &t->id);
    if (!t->id) { free(t); return NULL; }
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    t->filter = 0;
    glGenFramebuffers(1, &t->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->id, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    /* Creation may happen while another scene is open: restore it. */
    glBindFramebuffer(GL_FRAMEBUFFER, current_fbo);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "[vgl2d] render target %dx%d incomplete status=0x%04x\n", width, height, (unsigned)status);
        fflush(stderr);
        glDeleteFramebuffers(1, &t->fbo);
        glDeleteTextures(1, &t->id);
        free(t);
        return NULL;
    }
    t->width = width;
    t->height = height;
    t->bytes = vita_texture_allocation_bytes(width, height, 4);
    texture_bytes += t->bytes;
    return t;
}

int vita_gxm_target_begin(void *target, uint32_t clear_color) {
    VGLTexture *t = target;
    if (!initialized || !t || !t->fbo || drawing || target_drawing) return 0;
    glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    current_fbo = t->fbo;
    glViewport(0, 0, t->width, t->height);
    glDisable(GL_SCISSOR_TEST);
    set_mvp(target_mvp);
    clear_to(clear_color);
    target_drawing = 1;
    stat_targets++;
    return 1;
}

void vita_gxm_target_end(void) {
    if (!target_drawing) return;
    flush_batch();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    current_fbo = 0;
    glViewport(0, 0, 960, 544);
    set_mvp(display_mvp);
    target_drawing = 0;
}

unsigned vita_gxm_last_gpu_wait_us(void) {
    return last_swap_us;
}

void vita_gxm_set_vsync(int enabled) {
    vglWaitVblankStart(enabled ? GL_TRUE : GL_FALSE);
}

void vita_gxm_blend_add(int enabled) {
    enabled = enabled ? 1 : 0;
    if (enabled == blend_add) return;
    flush_batch();
    blend_add = enabled;
    if (initialized) apply_blend();
}

/* Tilt-shift needs the GXM 3D pass; fall back to a plain display frame. */
int vita_gxm_begin_offscreen(uint32_t color) {
    (void)color;
    return 0;
}
void vita_gxm_tiltshift_resolve(float top, float bottom, float strength) {
    (void)top; (void)bottom; (void)strength;
}

/* GXM 3D prototype entry points: unavailable in this backend. */
int vita_gxm_triangles3d(void *texture, const float *xyz, const float *uv, unsigned count, const float *mvp, uint32_t color) {
    (void)texture; (void)xyz; (void)uv; (void)count; (void)mvp; (void)color;
    return 0;
}
void vita_gxm_scene3d_end(void) {}
void *vita_gxm_mesh_create(const float *xyz, const float *uv, unsigned count) {
    (void)xyz; (void)uv; (void)count;
    return NULL;
}
int vita_gxm_mesh_draw(void *mesh, void *texture, const float *mvp, uint32_t color) {
    (void)mesh; (void)texture; (void)mvp; (void)color;
    return 0;
}
int vita_gxm_mesh_update(void *mesh, const float *xyz, const float *uv, unsigned count) {
    (void)mesh; (void)xyz; (void)uv; (void)count;
    return 0;
}
void vita_gxm_mesh_reset(void) {}
unsigned vita_gxm_mesh_arena_used(void) { return 0; }
void vita_gxm_depth_write(int enabled) { (void)enabled; }
int vita_gxm_vertex_lighting_available(void) { return 0; }
void *vita_gxm_light_mesh_create(const float *xyz, unsigned count) {
    (void)xyz; (void)count;
    return NULL;
}
void vita_gxm_light_mesh_set_colors(void *mesh, const uint32_t *colors) { (void)mesh; (void)colors; }
int vita_gxm_light_mesh_draw(void *mesh, const float *mvp) {
    (void)mesh; (void)mvp;
    return 0;
}

void vita_gxm_finish(void) {
    if (!initialized) return;
    vita_gxm_end(0);
    vita_gxm_target_end();
    glFinish();
    if (color_program) glDeleteProgram(color_program);
    if (texture_program) glDeleteProgram(texture_program);
    color_program = texture_program = 0;
    free(staging);
    staging = NULL;
    /* This VitaGL build has no public teardown; the context lives on. */
    initialized = 0;
}
