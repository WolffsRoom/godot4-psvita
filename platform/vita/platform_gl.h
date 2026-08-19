/**************************************************************************/
/*  platform_gl.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#ifndef GLES_API_ENABLED
#define GLES_API_ENABLED // Allow using GLES.
#endif

// IWYU pragma: begin_exports.
//
// VitaGL exposes an OpenGL-ES-style API on top of GXM. It ships its own
// <vitaGL.h> header which declares gl* entry points plus the vgl* control
// functions (vglInit, vglSwapBuffers, ...).
//
// IMPORTANT (see README.md and the ES3 audit): VitaGL implements ~GLES2 / GL2
// semantics. Godot's Compatibility renderer (drivers/gles3) targets real
// OpenGL ES 3.0 / GLSL ES 3.00. The include below will surface *link-time*
// missing-symbol errors for every ES3 entry point VitaGL lacks. That error
// list is precisely the porting work for the renderer phase.
#include <vitaGL.h>
// IWYU pragma: end_exports.

// --- ES3 entry-point shims missing from VitaGL --------------------------------
// Godot's Compatibility renderer (and its generated shader binders) call the
// unsigned-int uniform family glUniform*ui / glUniform*uiv, which this VitaGL
// build does not export. GLSL uint uniforms hold small flags/counts, so routing
// them through the signed variants is safe (identical bit layout for *uiv).
static inline void glUniform1ui(GLint l, GLuint x) { glUniform1i(l, (GLint)x); }
static inline void glUniform2ui(GLint l, GLuint x, GLuint y) { glUniform2i(l, (GLint)x, (GLint)y); }
static inline void glUniform3ui(GLint l, GLuint x, GLuint y, GLuint z) { glUniform3i(l, (GLint)x, (GLint)y, (GLint)z); }
static inline void glUniform4ui(GLint l, GLuint x, GLuint y, GLuint z, GLuint w) { glUniform4i(l, (GLint)x, (GLint)y, (GLint)z, (GLint)w); }
static inline void glUniform1uiv(GLint l, GLsizei n, const GLuint *v) { glUniform1iv(l, n, (const GLint *)v); }
static inline void glUniform2uiv(GLint l, GLsizei n, const GLuint *v) { glUniform2iv(l, n, (const GLint *)v); }
static inline void glUniform3uiv(GLint l, GLsizei n, const GLuint *v) { glUniform3iv(l, n, (const GLint *)v); }
static inline void glUniform4uiv(GLint l, GLsizei n, const GLuint *v) { glUniform4iv(l, n, (const GLint *)v); }

// --- ES3 texture-parameter enums missing from VitaGL --------------------------
// Standard GL token values; defining them lets drivers/gles3 storage compile.
// NOTE: VitaGL may not honour all of these at runtime (e.g. texture swizzle,
// base/max level) — that is a Phase-2 renderer concern, not a compile issue.
#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL 0x813C
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
#ifndef GL_TEXTURE_SWIZZLE_R
#define GL_TEXTURE_SWIZZLE_R 0x8E42
#endif
#ifndef GL_TEXTURE_SWIZZLE_G
#define GL_TEXTURE_SWIZZLE_G 0x8E43
#endif
#ifndef GL_TEXTURE_SWIZZLE_B
#define GL_TEXTURE_SWIZZLE_B 0x8E44
#endif
#ifndef GL_TEXTURE_SWIZZLE_A
#define GL_TEXTURE_SWIZZLE_A 0x8E45
#endif

// --- ES3 enum tokens missing from VitaGL (standard GL values) -----------------
#ifndef GL_COLOR
#define GL_COLOR 0x1800
#endif
#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#endif
#ifndef GL_COMPARE_REF_TO_TEXTURE
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#endif
#ifndef GL_CONSTANT_COLOR
#define GL_CONSTANT_COLOR 0x8001
#endif
#ifndef GL_COPY_READ_BUFFER
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_COPY_WRITE_BUFFER 0x8F37
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif
#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#endif
#ifndef GL_UNSIGNED_INT_5_9_9_9_REV
#define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#endif
#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES 0x8D57
#endif
#ifndef GL_MAX_UNIFORM_BLOCK_SIZE
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#endif
#ifndef GL_MAX_FRAGMENT_INPUT_COMPONENTS
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS 0x9125
#endif
#ifndef GL_MAX_VERTEX_OUTPUT_COMPONENTS
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS 0x9122
#endif
#ifndef GL_RASTERIZER_DISCARD
#define GL_RASTERIZER_DISCARD 0x8C89
#endif
#ifndef GL_TRANSFORM_FEEDBACK_BUFFER
#define GL_TRANSFORM_FEEDBACK_BUFFER 0x8C8E
#endif
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D 0x806F
#endif
#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#endif
#ifndef GL_TEXTURE_COMPARE_MODE
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#endif
// Sized/integer internal formats + pixel formats.
#ifndef GL_RG8
#define GL_RG8 0x822B
#endif
#ifndef GL_R16F
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#endif
#ifndef GL_RGB16F
#define GL_RGB16F 0x881B
#define GL_RGB32F 0x8815
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_R16UI
#define GL_R16UI 0x8234
#endif
#ifndef GL_RG16UI
#define GL_RG16UI 0x823A
#endif
#ifndef GL_RGB16UI
#define GL_RGB16UI 0x8D77
#endif
#ifndef GL_RGBA16UI
#define GL_RGBA16UI 0x8D76
#endif
#ifndef GL_RGBA8UI
#define GL_RGBA8UI 0x8D7C
#endif
#ifndef GL_RGB10_A2
#define GL_RGB10_A2 0x8059
#endif
#ifndef GL_RGB565
#define GL_RGB565 0x8D62
#endif
#ifndef GL_RGB9_E5
#define GL_RGB9_E5 0x8C3D
#endif
#ifndef GL_RED_INTEGER
#define GL_RED_INTEGER 0x8D94
#define GL_RG_INTEGER 0x8228
#define GL_RGB_INTEGER 0x8D98
#define GL_RGBA_INTEGER 0x8D99
#endif
#ifndef GL_RG16I
#define GL_RG16I 0x8239
#endif
// Sync object enums.
#ifndef GL_SYNC_STATUS
#define GL_SYNC_STATUS 0x9114
#define GL_SIGNALED 0x9119
#define GL_UNSIGNALED 0x9118
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_ALREADY_SIGNALED 0x911A
#define GL_TIMEOUT_EXPIRED 0x911B
#define GL_CONDITION_SATISFIED 0x911C
#define GL_WAIT_FAILED 0x911D
#endif

// NB: VitaGL already provides GLsync / GLint64 / GLuint64 (GLsync == int32_t
// here), so we must NOT redeclare them — doing so is a conflicting typedef.
#ifndef GL_ES3_COMPAT_GLHALF
#define GL_ES3_COMPAT_GLHALF
typedef unsigned short GLhalf; // 16-bit half float storage (not provided by VitaGL).
#endif

// --- ES3 function shims missing from VitaGL -----------------------------------
// Routable ones map to the ES2 equivalent; the rest are no-op stubs so the
// gles3 driver COMPILES. Features with no GXM path (3D/array textures, MRT,
// transform feedback, sync objects, multiview) are disabled at runtime — this
// is Phase-2 work, tracked in platform/vita/README.md.
static inline void glVertexAttribIPointer(GLuint i, GLint s, GLenum t, GLsizei st, const void *p) { glVertexAttribPointer(i, s, t, GL_FALSE, st, p); }
static inline void glVertexAttribI4ui(GLuint i, GLuint x, GLuint y, GLuint z, GLuint w) { glVertexAttrib4f(i, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w); }
static inline void glGetInteger64v(GLenum pname, GLint64 *data) { GLint t = 0; glGetIntegerv(pname, &t); *data = (GLint64)t; }
static inline void glRenderbufferStorageMultisample(GLenum tg, GLsizei, GLenum ifmt, GLsizei w, GLsizei h) { glRenderbufferStorage(tg, ifmt, w, h); }
static inline void glDrawBuffers(GLsizei, const GLenum *) {} // Vita: single color buffer (no MRT).
static inline void glClearBufferfv(GLenum, GLint, const GLfloat *) {}
static inline void glBlendColor(GLfloat, GLfloat, GLfloat, GLfloat) {}
static inline void glCopyBufferSubData(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr) {}
static inline void glFramebufferTextureLayer(GLenum, GLenum, GLuint, GLint, GLint) {}
static inline void glTexStorage2D(GLenum, GLsizei, GLenum, GLsizei, GLsizei) {} // TODO: emulate via glTexImage2D.
// 3D / array textures — no GXM path; stub to compile.
static inline void glTexImage3D(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *) {}
static inline void glTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void *) {}
static inline void glCompressedTexImage3D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const void *) {}
static inline void glCompressedTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void *) {}
static inline void glTexImage3DMultisample(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean) {}
static inline void glFramebufferTextureMultiviewOVR(GLenum, GLenum, GLuint, GLint, GLint, GLsizei) {}
// Transform feedback — no GXM path (GPU particles disabled on Vita).
#ifndef GL_INTERLEAVED_ATTRIBS
#define GL_INTERLEAVED_ATTRIBS 0x8C8C
#define GL_SEPARATE_ATTRIBS 0x8C8D
#endif
static inline void glBeginTransformFeedback(GLenum) {}
static inline void glEndTransformFeedback(void) {}
static inline void glTransformFeedbackVaryings(GLuint, GLsizei, const GLchar *const *, GLenum) {}
// Sync objects — report "already signaled" so the driver never blocks.
static inline GLsync glFenceSync(GLenum, GLbitfield) { return (GLsync)1; }
static inline GLenum glClientWaitSync(GLsync, GLbitfield, GLuint64) { return GL_ALREADY_SIGNALED; }
static inline void glDeleteSync(GLsync) {}
static inline void glGetSynciv(GLsync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) {
	if (values && bufSize > 0) {
		*values = (pname == GL_SYNC_STATUS) ? (GLint)GL_SIGNALED : 0;
	}
	if (length) {
		*length = (bufSize > 0) ? 1 : 0;
	}
}
