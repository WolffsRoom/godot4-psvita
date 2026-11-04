#!/usr/bin/env python3
"""Check the VitaGL canvas bridge against a recording host fake of VitaGL.

Covers the A/B contract with gxm_bridge.c: batching, vertex layout, colour
and tint conversion, scissor origin, render-target orientation, RGBA4444
packing, deferred texture release ordering, single VitaGL init and the 3D
path (VBO meshes, depth state, CPU triangles, the 2x multiply light pass). It does
not prove anything about the console: VitaGL, the shader compiler and the
GPU are simulated.
"""
from pathlib import Path
import subprocess
import tempfile

platform = Path(__file__).resolve().parents[2]

sysmem = r'''
#pragma once
typedef struct { unsigned size, size_user, size_cdram, size_phycont; } SceKernelFreeMemorySizeInfo;
int sceKernelGetFreeMemorySize(SceKernelFreeMemorySizeInfo *info);
'''
processmgr = r'''
#pragma once
typedef unsigned long long SceUInt64;
SceUInt64 sceKernelGetProcessTimeWide(void);
'''
vitagl = r'''
#pragma once
#include <stdint.h>
typedef unsigned int GLuint, GLenum, GLbitfield;
typedef int GLint, GLsizei;
typedef float GLfloat;
typedef unsigned char GLboolean;
typedef char GLchar;
typedef void GLvoid;
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_FLOAT 0x1406
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_RGBA 0x1908
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_TRIANGLES 0x0004
#define GL_ARRAY_BUFFER 0x8892
#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE 0x0B44
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 1
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_DEPTH_BUFFER_BIT 0x0100
#define GL_LEQUAL 0x0203
#define GL_DST_COLOR 0x0306
#define GL_SRC_COLOR 0x0300
#define GL_ZERO 0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
typedef long GLsizeiptr;
#define SCE_GXM_MULTISAMPLE_NONE 0
#define SCE_GXM_MULTISAMPLE_4X 2
typedef int SceGxmMultisampleMode;
#define VGL_MODE_SHADER_PAIR 0
void vglSetSemanticBindingMode(GLenum);
void vglSetParamBufferSize(uint32_t);
void vglSetCircularPoolSize(uint32_t);
void vglUseTripleBuffering(GLboolean);
GLboolean vglInitWithCustomThreshold(int, int, int, int, int, int, int, SceGxmMultisampleMode);
void vglSwapBuffers(GLboolean);
void vglWaitVblankStart(GLboolean);
GLuint glCreateShader(GLenum);
void glShaderSource(GLuint, GLsizei, const GLchar *const *, const GLint *);
void glCompileShader(GLuint);
void glGetShaderiv(GLuint, GLenum, GLint *);
void glGetShaderInfoLog(GLuint, GLsizei, GLsizei *, GLchar *);
void glDeleteShader(GLuint);
GLuint glCreateProgram(void);
void glAttachShader(GLuint, GLuint);
void glBindAttribLocation(GLuint, GLuint, const GLchar *);
void glLinkProgram(GLuint);
void glGetProgramiv(GLuint, GLenum, GLint *);
void glDeleteProgram(GLuint);
void glUseProgram(GLuint);
GLint glGetUniformLocation(GLuint, const GLchar *);
void glUniform1i(GLint, GLint);
void glUniform4fv(GLint, GLsizei, const GLfloat *);
void glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat *);
void glActiveTexture(GLenum);
void glBindBuffer(GLenum, GLuint);
void glEnableVertexAttribArray(GLuint);
void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
void glDrawArrays(GLenum, GLint, GLsizei);
void glEnable(GLenum);
void glDisable(GLenum);
void glBlendFunc(GLenum, GLenum);
void glGenTextures(GLsizei, GLuint *);
void glDeleteTextures(GLsizei, const GLuint *);
void glBindTexture(GLenum, GLuint);
void glPixelStorei(GLenum, GLint);
void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
void glTexParameteri(GLenum, GLenum, GLint);
void glGenFramebuffers(GLsizei, GLuint *);
void glDeleteFramebuffers(GLsizei, const GLuint *);
void glBindFramebuffer(GLenum, GLuint);
void glFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint);
GLenum glCheckFramebufferStatus(GLenum);
void glViewport(GLint, GLint, GLsizei, GLsizei);
void glScissor(GLint, GLint, GLsizei, GLsizei);
void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat);
void glClear(GLbitfield);
void glFinish(void);
void glDepthFunc(GLenum);
void glDepthMask(GLboolean);
void glClearDepthf(GLfloat);
void glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum);
void glGenBuffers(GLsizei, GLuint *);
void glDeleteBuffers(GLsizei, const GLuint *);
void glBufferData(GLenum, GLsizeiptr, const void *, GLenum);
'''

harness = r'''
#include <vitaGL.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include "gxm_bridge.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int vgl_inits, swaps, draws, last_count, deleted_textures, deleted_fbos, compile_ok = 1;
static int init_msaa = -1, init_ram = -1, init_phycont = -1;
static unsigned circular = 0;
static GLuint next_id = 1, bound_texture, bound_fbo, program_in_use;
static const unsigned char *attrib_ptr[2];
static GLint attrib_size[2], attrib_stride[2];
static GLenum attrib_type[2];
static GLboolean attrib_norm[2];
static float last_mvp[16], last_tint[4], clear_rgba[4];
static int scissor[4], scissor_on, viewport[4];
static GLenum blend_src, blend_dst;
static GLint tex_filter[64];
static GLenum upload_type;
static uint16_t upload_4444[4];
static int vsync = -1;
static unsigned char drawn[4096];
static size_t drawn_bytes;
static GLuint drawn_texture, drawn_program;
static GLuint bound_buffer, drawn_buffer;
static int depth_test, depth_mask = 1, depth_func, clears_with_depth, buffers_alive, drawn_first;
static unsigned char vbo_data[8][4096];
static size_t vbo_size[8];
static GLenum blend_src_alpha, blend_dst_alpha;

int sceKernelGetFreeMemorySize(SceKernelFreeMemorySizeInfo *i) { memset(i, 0, sizeof(*i)); return 0; }
SceUInt64 sceKernelGetProcessTimeWide(void) { static SceUInt64 t; return t += 10; }
void vglSetSemanticBindingMode(GLenum m) { assert(m == VGL_MODE_SHADER_PAIR); }
void vglSetParamBufferSize(uint32_t s) { assert(s == 4u * 1024 * 1024); }
void vglSetCircularPoolSize(uint32_t s) { circular = s; }
void vglUseTripleBuffering(GLboolean b) { assert(!b); }
GLboolean vglInitWithCustomThreshold(int pool, int w, int h, int ram, int cdram, int phycont, int cdlg, SceGxmMultisampleMode msaa) {
    assert(pool == 0 && w == 960 && h == 544 && cdram == 0 && cdlg == 0);
    vgl_inits++; init_ram = ram; init_phycont = phycont; init_msaa = msaa; return GL_FALSE;
}
void vglSwapBuffers(GLboolean d) { assert(!d); swaps++; }
void vglWaitVblankStart(GLboolean e) { vsync = e; }
GLuint glCreateShader(GLenum t) { (void)t; return next_id++; }
void glShaderSource(GLuint s, GLsizei n, const GLchar *const *src, const GLint *l) { (void)s; assert(n == 1 && src[0] && !l); }
void glCompileShader(GLuint s) { (void)s; }
void glGetShaderiv(GLuint s, GLenum p, GLint *v) { (void)s; assert(p == GL_COMPILE_STATUS); *v = compile_ok; }
void glGetShaderInfoLog(GLuint s, GLsizei n, GLsizei *l, GLchar *b) { (void)s; (void)n; *l = 0; b[0] = 0; }
void glDeleteShader(GLuint s) { (void)s; }
GLuint glCreateProgram(void) { return next_id++; }
void glAttachShader(GLuint p, GLuint s) { (void)p; (void)s; }
void glBindAttribLocation(GLuint p, GLuint i, const GLchar *n) {
    (void)p; assert((i == 0 && !strcmp(n, "a_position")) || (i == 1 && (!strcmp(n, "a_color") || !strcmp(n, "a_uv"))));
}
void glLinkProgram(GLuint p) { (void)p; }
void glGetProgramiv(GLuint p, GLenum e, GLint *v) { (void)p; assert(e == GL_LINK_STATUS); *v = 1; }
void glDeleteProgram(GLuint p) { (void)p; }
void glUseProgram(GLuint p) { program_in_use = p; }
GLint glGetUniformLocation(GLuint p, const GLchar *n) {
    (void)p;
    if (!strcmp(n, "u_mvp")) return 1;
    if (!strcmp(n, "u_tint")) return 2;
    if (!strcmp(n, "u_texture")) return 3;
    assert(0); return -1;
}
void glUniform1i(GLint l, GLint v) { assert(l == 3 && v == 0); }
void glUniform4fv(GLint l, GLsizei n, const GLfloat *v) { assert(l == 2 && n == 1); memcpy(last_tint, v, sizeof(last_tint)); }
void glUniformMatrix4fv(GLint l, GLsizei n, GLboolean t, const GLfloat *v) { assert(l == 1 && n == 1 && !t); memcpy(last_mvp, v, sizeof(last_mvp)); }
void glActiveTexture(GLenum t) { assert(t == GL_TEXTURE0); }
void glBindBuffer(GLenum t, GLuint b) { assert(t == GL_ARRAY_BUFFER); bound_buffer = b; }
void glGenBuffers(GLsizei n, GLuint *b) { assert(n == 1); static GLuint next = 1; *b = next++; assert(*b < 8); buffers_alive++; }
void glDeleteBuffers(GLsizei n, const GLuint *b) { assert(n == 1 && *b); buffers_alive--; }
void glBufferData(GLenum t, GLsizeiptr n, const void *d, GLenum u) {
    assert(t == GL_ARRAY_BUFFER && bound_buffer && n > 0 && (size_t)n <= sizeof(vbo_data[0]) && d && (u == GL_STATIC_DRAW || u == GL_DYNAMIC_DRAW));
    memcpy(vbo_data[bound_buffer], d, (size_t)n); vbo_size[bound_buffer] = (size_t)n;
}
void glDepthFunc(GLenum f) { depth_func = f; }
void glDepthMask(GLboolean m) { depth_mask = m; }
void glClearDepthf(GLfloat d) { assert(d == 1.0f); }
void glBlendFuncSeparate(GLenum a, GLenum b, GLenum c, GLenum d) { blend_src = a; blend_dst = b; blend_src_alpha = c; blend_dst_alpha = d; }
void glEnableVertexAttribArray(GLuint i) { assert(i < 2); }
void glVertexAttribPointer(GLuint i, GLint s, GLenum t, GLboolean n, GLsizei st, const void *p) {
    assert(i < 2); attrib_size[i] = s; attrib_type[i] = t; attrib_norm[i] = n; attrib_stride[i] = st; attrib_ptr[i] = p;
}
void glDrawArrays(GLenum m, GLint f, GLsizei c) {
    assert(m == GL_TRIANGLES && f >= 0 && c > 0 && c % 3 == 0);
    draws++; last_count = c; drawn_first = f; drawn_buffer = bound_buffer;
    drawn_bytes = (size_t)c * attrib_stride[0];
    assert(drawn_bytes <= sizeof(drawn));
    if (bound_buffer) memcpy(drawn, vbo_data[bound_buffer] + (size_t)attrib_ptr[0], drawn_bytes);
    else memcpy(drawn, attrib_ptr[0], drawn_bytes); /* VitaGL copies client arrays here too. */
    drawn_texture = bound_texture; drawn_program = program_in_use;
}
void glEnable(GLenum c) { if (c == GL_SCISSOR_TEST) scissor_on = 1; if (c == GL_DEPTH_TEST) depth_test = 1; }
void glDisable(GLenum c) { if (c == GL_SCISSOR_TEST) scissor_on = 0; if (c == GL_DEPTH_TEST) depth_test = 0; }
void glBlendFunc(GLenum s, GLenum d) { blend_src = s; blend_dst = d; }
void glGenTextures(GLsizei n, GLuint *t) { assert(n == 1); *t = next_id++; }
void glDeleteTextures(GLsizei n, const GLuint *t) { assert(n == 1 && *t); deleted_textures++; }
void glBindTexture(GLenum t, GLuint id) { assert(t == GL_TEXTURE_2D); bound_texture = id; }
void glPixelStorei(GLenum p, GLint v) { assert(p == GL_UNPACK_ALIGNMENT && v == 1); }
void glTexImage2D(GLenum t, GLint l, GLint i, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void *d) {
    (void)d;
    assert(t == GL_TEXTURE_2D && l == 0 && i == GL_RGBA && w > 0 && h > 0 && b == 0 && f == GL_RGBA);
    upload_type = ty;
    if (ty == GL_UNSIGNED_SHORT_4_4_4_4 && d) memcpy(upload_4444, d, sizeof(upload_4444));
}
void glTexParameteri(GLenum t, GLenum p, GLint v) {
    assert(t == GL_TEXTURE_2D && bound_texture < 64);
    if (p == GL_TEXTURE_MIN_FILTER || p == GL_TEXTURE_MAG_FILTER) tex_filter[bound_texture] = v;
    else assert(v == GL_CLAMP_TO_EDGE);
}
void glGenFramebuffers(GLsizei n, GLuint *f) { assert(n == 1); *f = next_id++; }
void glDeleteFramebuffers(GLsizei n, const GLuint *f) { assert(n == 1 && *f); deleted_fbos++; }
void glBindFramebuffer(GLenum t, GLuint f) { assert(t == GL_FRAMEBUFFER); bound_fbo = f; }
void glFramebufferTexture2D(GLenum t, GLenum a, GLenum tt, GLuint tex, GLint l) {
    assert(t == GL_FRAMEBUFFER && a == GL_COLOR_ATTACHMENT0 && tt == GL_TEXTURE_2D && tex && l == 0);
}
GLenum glCheckFramebufferStatus(GLenum t) { (void)t; return GL_FRAMEBUFFER_COMPLETE; }
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { viewport[0] = x; viewport[1] = y; viewport[2] = w; viewport[3] = h; }
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h) { scissor[0] = x; scissor[1] = y; scissor[2] = w; scissor[3] = h; }
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { clear_rgba[0] = r; clear_rgba[1] = g; clear_rgba[2] = b; clear_rgba[3] = a; }
void glClear(GLbitfield m) {
    assert(m == GL_COLOR_BUFFER_BIT || m == (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    if (m & GL_DEPTH_BUFFER_BIT) { assert(depth_mask); clears_with_depth++; }
}
void glFinish(void) {}

static int near(float a, float b) { return fabsf(a - b) < 1e-5f; }
static float project_y(float y) { return last_mvp[5] * y + last_mvp[13]; }

int main(void) {
    FILE *f = fopen("app0:vitagl.cfg", "w"); fputs("vitagl-bridge-ab-v2\nram_threshold_mb=40\ncircular_pool_mb=6\n", f); fclose(f);
    f = fopen("app0:gxm_texture_format.cfg", "w"); fputs("RGBA4444\n", f); fclose(f);
    assert(vita_gxm_init());
    assert(vgl_inits == 1 && init_msaa == SCE_GXM_MULTISAMPLE_NONE);
    assert(init_ram == 40 * 1024 * 1024 && init_phycont == 8 * 1024 * 1024 && circular == 6u * 1024 * 1024);
    assert(blend_src == GL_SRC_ALPHA && blend_dst == GL_ONE_MINUS_SRC_ALPHA);

    /* RGBA4444 in GL order: R in the high nibble. */
    unsigned char rgba[16] = { 255, 0, 0, 255,  0, 255, 0, 0,  0, 0, 255, 136,  17, 34, 51, 68 };
    void *tex = vita_gxm_texture_create(2, 2, rgba);
    assert(tex && upload_type == GL_UNSIGNED_SHORT_4_4_4_4);
    assert(upload_4444[0] == 0xF00F && upload_4444[1] == 0x0F00 && upload_4444[2] == 0x00F8 && upload_4444[3] == 0x1234);
    GLuint tex_id = bound_texture;

    vita_gxm_quad((const float[]){0, 0, 1, 0, 1, 1, 0, 1}, 0xffffffff); /* Outside a frame: dropped. */
    vita_gxm_begin(0xff336699);
    assert(bound_fbo == 0 && viewport[2] == 960 && viewport[3] == 544);
    assert(near(clear_rgba[0], 0x99 / 255.0f) && near(clear_rgba[1], 0x66 / 255.0f) && near(clear_rgba[2], 0x33 / 255.0f) && near(clear_rgba[3], 1.0f));

    float xy[8] = { 80, 120, 240, 120, 240, 200, 80, 200 };
    float uv[8] = { 0.25f, 0, 0.5f, 0, 0.5f, 1, 0.25f, 1 };
    vita_gxm_texture_quad(tex, xy, uv, 0x804020ff, 1);
    vita_gxm_texture_quad(tex, xy, uv, 0x804020ff, 1);
    assert(draws == 0); /* Same texture/filter/tint: still batching. */
    vita_gxm_quad(xy, 0xff4040ff); /* Kind change flushes the textured batch. */
    assert(draws == 1 && last_count == 12 && drawn_texture == tex_id && tex_filter[tex_id] == GL_LINEAR);
    assert(attrib_size[0] == 3 && attrib_type[0] == GL_FLOAT && attrib_stride[0] == 20);
    assert(attrib_size[1] == 2 && attrib_type[1] == GL_FLOAT && attrib_ptr[1] == attrib_ptr[0] + 12);
    const float *tv = (const float *)drawn;
    assert(tv[0] == 80 && tv[1] == 120 && tv[2] == 0.5f && tv[3] == 0.25f && tv[4] == 0);     /* corner 0 */
    assert(tv[10] == 240 && tv[11] == 200 && tv[13] == 0.5f && tv[14] == 1);                 /* corner 2 */
    assert(tv[25] == 80 && tv[26] == 200 && tv[28] == 0.25f && tv[29] == 1);                 /* corner 3 */
    assert(near(last_tint[0], 1.0f) && near(last_tint[1], 0x20 / 255.0f) && near(last_tint[2], 0x40 / 255.0f) && near(last_tint[3], 0x80 / 255.0f));
    /* Display matrix: y down, top edge maps to NDC +1. */
    assert(near(project_y(0), 1.0f) && near(project_y(544), -1.0f) && near(last_mvp[0] * 960 + last_mvp[12], 1.0f));

    /* Clip: GL scissor origin is bottom-left; redundant clips are skipped. */
    vita_gxm_clip(1, 10, 20, 30, 40);
    assert(draws == 2 && attrib_type[1] == GL_UNSIGNED_BYTE && attrib_norm[1] && attrib_size[1] == 4 && attrib_stride[0] == 16);
    const unsigned char *cv = drawn;
    uint32_t color; memcpy(&color, cv + 12, 4); assert(color == 0xff4040ff);
    assert(scissor_on && scissor[0] == 10 && scissor[1] == 544 - 60 && scissor[2] == 30 && scissor[3] == 40);
    vita_gxm_clip(1, 10, 20, 30, 40);
    vita_gxm_clip(1, 0, 0, 960, 544); /* Full screen == no clip. */
    assert(!scissor_on);

    /* Freeing the texture of the open batch submits it first. */
    vita_gxm_texture_quad(tex, xy, uv, 0xffffffff, 0);
    int before = draws;
    vita_gxm_texture_free(tex);
    assert(draws == before + 1 && deleted_textures == 1 && tex_filter[tex_id] == GL_NEAREST);

    vita_gxm_set_vsync(0); assert(vsync == 0);
    vita_gxm_end(1);
    assert(swaps == 1 && !scissor_on);
    vita_gxm_end(1);
    assert(swaps == 1);

    /* SubViewport target: own FBO, flipped matrix (GL bottom-up FBOs). */
    void *target = vita_gxm_target_create(128, 64);
    assert(target && bound_fbo == 0);
    assert(vita_gxm_target_begin(target, 0xff000000));
    GLuint target_fbo = bound_fbo;
    assert(target_fbo && viewport[2] == 128 && viewport[3] == 64);
    vita_gxm_quad(xy, 0xffffffff);
    void *texture2 = vita_gxm_texture_create(1, 1, rgba); /* Mid-target upload keeps the FBO bound. */
    assert(texture2 && bound_fbo == target_fbo);
    vita_gxm_target_end();
    assert(bound_fbo == 0 && near(project_y(0), -1.0f) && near(project_y(544), 1.0f));
    vita_gxm_begin(0xff000000);
    vita_gxm_quad(xy, 0xffffffff);
    vita_gxm_end(1);
    assert(near(project_y(0), 1.0f)); /* Display matrix restored for the screen. */
    vita_gxm_texture_free(target);
    vita_gxm_texture_free(texture2);
    assert(deleted_fbos == 1);

    vita_gxm_blend_add(1); assert(blend_src == GL_ONE && blend_dst == GL_ONE);
    vita_gxm_blend_add(0); assert(blend_src == GL_SRC_ALPHA);
    /* 2D-only package: no 3D, and no depth clear. */
    assert(!vita_gxm_mesh_create(xy, uv, 3) && !vita_gxm_vertex_lighting_available());
    assert(!vita_gxm_begin_offscreen(0) && clears_with_depth == 0);

    /* Teardown and re-init never call vglInit twice; gxm3d.cfg enables 3D. */
    vita_gxm_finish(); vita_gxm_finish();
    f = fopen("app0:gxm3d.cfg", "w"); fputs("static-mesh-prototype-v1\n", f); fclose(f);
    assert(vita_gxm_init() && vgl_inits == 1 && vita_gxm_vertex_lighting_available());

    float xyz[9] = { 0, 0, 0,  1, 0, 0,  0, 1, 0 }, tri_uv[6] = { 0, 0, 1, 0, 0, 1 };
    float mvp[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0.25f, 1 };
    void *mesh = vita_gxm_mesh_create(xyz, tri_uv, 3);
    assert(mesh && vita_gxm_mesh_arena_used() == 64 && bound_buffer == 0);
    assert(!vita_gxm_mesh_draw(mesh, NULL, mvp, 0xffffffff)); /* Outside a frame. */
    vita_gxm_begin(0xff000000);
    assert(clears_with_depth == 1 && !depth_mask);
    vita_gxm_quad(xy, 0xffffffff); /* Pending canvas batch must go out before 3D. */
    before = draws;
    assert(vita_gxm_mesh_draw(mesh, NULL, mvp, 0xff00ff00));
    assert(draws == before + 2 && drawn_buffer && depth_test && depth_mask && depth_func == GL_LEQUAL);
    assert(attrib_ptr[0] == NULL && attrib_ptr[1] == (const unsigned char *)12 && attrib_stride[0] == 20);
    assert(last_mvp[14] == 0.25f && near(last_tint[1], 1.0f) && near(last_tint[0], 0.0f));
    const float *mv = (const float *)drawn;
    assert(mv[0] == 0 && mv[5] == 1 && mv[8] == 1 && mv[11] == 1 && mv[14] == 1);
    GLuint white = drawn_texture; assert(white && white != tex_id);

    /* In-place update and CPU triangles. */
    xyz[0] = 0.5f;
    assert(vita_gxm_mesh_update(mesh, xyz, tri_uv, 3) && !vita_gxm_mesh_update(mesh, xyz, tri_uv, 6));
    assert(vita_gxm_triangles3d(NULL, xyz, NULL, 3, mvp, 0xff0000ff));
    assert(drawn_buffer == 0 && last_count == 3 && attrib_stride[0] == 16);
    memcpy(&color, drawn + 12, 4); assert(color == 0xff0000ff);
    assert(!vita_gxm_triangles3d(NULL, xyz, NULL, 4, mvp, 0)); /* Not whole triangles. */

    /* Light pass: 2x multiply blend, then normal blend restored. */
    void *lit = vita_gxm_light_mesh_create(xyz, 3);
    assert(lit);
    uint32_t colors[3] = { 0xff102030, 0xff405060, 0xff708090 };
    vita_gxm_light_mesh_set_colors(lit, colors);
    assert(vita_gxm_light_mesh_draw(lit, mvp));
    assert(drawn_program != white && attrib_stride[0] == 16);
    memcpy(&color, drawn + 16 + 12, 4); assert(color == 0xff405060);
    assert(blend_src == GL_SRC_ALPHA && blend_dst == GL_ONE_MINUS_SRC_ALPHA);
    assert(blend_src_alpha == GL_ZERO && blend_dst_alpha == GL_ONE);

    /* Transparent pass toggles depth writes; scene end returns to 2D state. */
    vita_gxm_depth_write(0); assert(!depth_mask);
    vita_gxm_depth_write(1); assert(depth_mask);
    vita_gxm_scene3d_end();
    assert(!depth_test && !depth_mask);
    vita_gxm_quad(xy, 0xffffffff); /* UI after 3D uses the canvas matrix again. */
    vita_gxm_end(1);
    assert(near(project_y(0), 1.0f) && last_count == 6);
    assert(buffers_alive == 2);
    vita_gxm_mesh_reset();
    assert(buffers_alive == 0 && vita_gxm_mesh_arena_used() == 0);
    vita_gxm_finish();
    remove("app0:gxm3d.cfg");
    remove("app0:vitagl.cfg");
    remove("app0:gxm_texture_format.cfg");
    return 0;
}
'''

with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    (root / 'psp2/kernel').mkdir(parents=True)
    (root / 'psp2/kernel/sysmem.h').write_text(sysmem)
    (root / 'psp2/kernel/processmgr.h').write_text(processmgr)
    (root / 'vitaGL.h').write_text(vitagl)
    (root / 'test.c').write_text(harness)
    exe = root / 'test'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I' + temp, '-I' + str(platform),
                    str(root / 'test.c'), str(platform / 'loader/gxm_bridge_vgl.c'), '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True, cwd=temp)
print('PASS: VitaGL bridge init, batching, vertex layout, tint/colour, scissor origin, '
      'target orientation, RGBA4444 GL packing, batch flush before free, single vglInit, '
      '3D meshes/depth/CPU triangles/light pass')
