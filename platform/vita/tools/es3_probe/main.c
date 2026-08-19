/*
 * VitaGL ES3 capability probe  (Godot 4.x Vita port — Phase 0)  — v2 instrumented
 * -------------------------------------------------------------------------------
 * Measures, on real hardware, how much of the OpenGL ES 3.0 / GLSL ES 3.00
 * surface Godot 4's Compatibility renderer needs is usable through VitaGL.
 * See platform/vita/README.md.
 *
 * v2 changes vs v1 (v1 crashed mid-run and buffered log lines were lost):
 *   - UNBUFFERED log (setvbuf _IONBF) + a "BEGIN <t>"/"END <t>" breadcrumb around
 *     every sub-test, so a crash leaves the culprit as the last BEGIN without END.
 *   - the instanced-draw test now binds a VALID program first (v1 drew with no
 *     program bound — a likely GXM fault, not a real API result).
 *
 * Result -> ux0:/data/es3_probe.txt (mirrored to sceClibPrintf).
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>

#include <vitaGL.h>

static FILE *g_log = NULL;
static void logln(const char *fmt, ...) {
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	sceClibPrintf("%s\n", buf);
	if (g_log) {
		fprintf(g_log, "%s\n", buf);
		fflush(g_log);
	}
}

/* Breadcrumbs: last BEGIN without a matching END == the call that crashed. */
static void begin(const char *t) { logln(">>> BEGIN %s", t); }
static void end(const char *t) { logln("<<< END   %s", t); }

static int checks_total = 0, checks_pass = 0;
static void report(const char *name, int ok, const char *note) {
	checks_total++;
	if (ok) {
		checks_pass++;
	}
	logln("[%s] %-28s %s", ok ? "PASS" : "FAIL", name, note ? note : "");
}

static const char *gl_err_str(GLenum e) {
	switch (e) {
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		default: return "GL_ERR_?";
	}
}
static void drain_gl_errors(const char *where) {
	GLenum e;
	while ((e = glGetError()) != GL_NO_ERROR) {
		logln("    (gl error @ %s: %s)", where, gl_err_str(e));
	}
}

/* --- decisive: a GLSL ES 3.00 shader like Godot emits ----------------------- */
static const char *VS_300ES =
	"#version 300 es\n"
	"layout(std140) uniform Data { mat4 mvp; vec4 tint; };\n"
	"in vec3 a_pos;\n"
	"in vec2 a_uv;\n"
	"flat out uint v_flags;\n"
	"out vec2 v_uv;\n"
	"void main() {\n"
	"  v_uv = a_uv;\n"
	"  v_flags = uint(gl_VertexID);\n"
	"  gl_Position = mvp * vec4(a_pos, 1.0);\n"
	"}\n";

static const char *FS_300ES =
	"#version 300 es\n"
	"precision mediump float;\n"
	"layout(std140) uniform Data { mat4 mvp; vec4 tint; };\n"
	"flat in uint v_flags;\n"
	"in vec2 v_uv;\n"
	"out vec4 frag;\n"
	"void main() {\n"
	"  frag = tint * vec4(v_uv, float(v_flags & 1u), 1.0);\n"
	"}\n";

/* Minimal valid ES 1.00 program, used to bind SOMETHING before the instanced draw. */
static const char *VS_ES1 =
	"attribute vec3 a_pos;\n"
	"void main() { gl_Position = vec4(a_pos, 1.0); }\n";
static const char *FS_ES1 =
	"precision mediump float;\n"
	"void main() { gl_FragColor = vec4(1.0); }\n";

static GLuint build_program(const char *vsrc, const char *fsrc, int *r_vok, int *r_fok, int *r_lok,
		char *vlog, char *flog, char *plog, int logsz) {
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vsrc, NULL);
	glCompileShader(vs);
	GLint vok = 0;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &vok);
	if (!vok && vlog) {
		GLsizei n = 0;
		glGetShaderInfoLog(vs, logsz - 1, &n, vlog);
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fsrc, NULL);
	glCompileShader(fs);
	GLint fok = 0;
	glGetShaderiv(fs, GL_COMPILE_STATUS, &fok);
	if (!fok && flog) {
		GLsizei n = 0;
		glGetShaderInfoLog(fs, logsz - 1, &n, flog);
	}

	GLuint prog = 0;
	int lok = 0;
	if (vok && fok) {
		prog = glCreateProgram();
		glAttachShader(prog, vs);
		glAttachShader(prog, fs);
		glBindAttribLocation(prog, 0, "a_pos");
		glBindAttribLocation(prog, 1, "a_uv");
		glLinkProgram(prog);
		GLint l = 0;
		glGetProgramiv(prog, GL_LINK_STATUS, &l);
		lok = l;
		if (!lok && plog) {
			GLsizei n = 0;
			glGetProgramInfoLog(prog, logsz - 1, &n, plog);
		}
	}
	if (r_vok) *r_vok = vok;
	if (r_fok) *r_fok = fok;
	if (r_lok) *r_lok = lok;
	glDeleteShader(vs);
	glDeleteShader(fs);
	return prog;
}

static void test_glsl_300es(void) {
	/* Granular breadcrumbs: v1+v2 showed vertex+fragment compile OK but the
	 * process dies during LINK. This isolates each step so the .txt proves it.
	 * Kept LAST in main() so the API tests below run before this crashes. */
	begin("glsl300es");
	char log[256] = { 0 };

	begin("glsl300es.compile.vs");
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &VS_300ES, NULL);
	glCompileShader(vs);
	GLint vok = 0;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &vok);
	if (!vok) {
		GLsizei n = 0;
		glGetShaderInfoLog(vs, sizeof(log) - 1, &n, log);
	}
	report("glsl300es.vertex", vok, vok ? "" : log);
	end("glsl300es.compile.vs");

	begin("glsl300es.compile.fs");
	log[0] = 0;
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &FS_300ES, NULL);
	glCompileShader(fs);
	GLint fok = 0;
	glGetShaderiv(fs, GL_COMPILE_STATUS, &fok);
	if (!fok) {
		GLsizei n = 0;
		glGetShaderInfoLog(fs, sizeof(log) - 1, &n, log);
	}
	report("glsl300es.fragment", fok, fok ? "" : log);
	end("glsl300es.compile.fs");

	if (vok && fok) {
		GLuint prog = glCreateProgram();
		glAttachShader(prog, vs);
		glAttachShader(prog, fs);
		glBindAttribLocation(prog, 0, "a_pos");
		glBindAttribLocation(prog, 1, "a_uv");
		begin("glsl300es.link"); /* <-- expected crash point */
		glLinkProgram(prog);
		GLint lok = 0;
		glGetProgramiv(prog, GL_LINK_STATUS, &lok);
		if (!lok) {
			GLsizei n = 0;
			log[0] = 0;
			glGetProgramInfoLog(prog, sizeof(log) - 1, &n, log);
		}
		report("glsl300es.link", lok, lok ? "" : log);
		end("glsl300es.link");
		glDeleteProgram(prog);
	}
	glDeleteShader(vs);
	glDeleteShader(fs);
	drain_gl_errors("glsl300es");
	end("glsl300es");
}

static void test_vao(void) {
	begin("vao");
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	GLenum e = glGetError();
	report("vao.genBind", (vao != 0 && e == GL_NO_ERROR), (e != GL_NO_ERROR) ? gl_err_str(e) : "");
	glBindVertexArray(0);
	if (vao) {
		glDeleteVertexArrays(1, &vao);
	}
	end("vao");
}

static void test_ubo_binding(void) {
	begin("ubo.bind");
	GLuint ubo = 0;
	float data[20];
	memset(data, 0, sizeof(data));
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
	GLenum e = glGetError();
	report("ubo.bindBufferBase", e == GL_NO_ERROR, (e != GL_NO_ERROR) ? gl_err_str(e) : "");
	glDeleteBuffers(1, &ubo);
	end("ubo.bind");
}

static void test_map_buffer_range(void) {
	begin("mapRange");
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 1024, NULL, GL_DYNAMIC_DRAW);
	void *p = glMapBufferRange(GL_ARRAY_BUFFER, 0, 1024, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	int ok = (p != NULL);
	if (ok) {
		memset(p, 0, 16);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	report("buffer.mapRange", ok, ok ? "" : "returned NULL");
	drain_gl_errors("mapBufferRange");
	glDeleteBuffers(1, &vbo);
	end("mapRange");
}

static void test_instancing(void) {
	begin("instancing");
	/* v2: bind a valid ES 1.00 program + viewport first, so we exercise the
	 * instanced-draw path rather than crashing on a null pipeline. */
	int vok, fok, lok;
	GLuint prog = build_program(VS_ES1, FS_ES1, &vok, &fok, &lok, NULL, NULL, NULL, 0);
	if (!lok) {
		report("draw.instanced", 0, "helper ES1 program failed to build");
		if (prog) glDeleteProgram(prog);
		end("instancing");
		return;
	}
	glViewport(0, 0, 960, 544);
	glUseProgram(prog);

	GLuint vbo = 0, ibo = 0, vao = 0;
	float verts[] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };
	unsigned short idx[] = { 0, 1, 2 };
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 0);
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

	begin("instancing.draw");
	glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0, 4);
	end("instancing.draw");

	GLenum e = glGetError();
	report("draw.instanced", e == GL_NO_ERROR, (e != GL_NO_ERROR) ? gl_err_str(e) : "");
	glBindVertexArray(0);
	glUseProgram(0);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(prog);
	end("instancing");
}

int main(void) {
	scePowerSetArmClockFrequency(444);
	scePowerSetGpuClockFrequency(222);

	/* NB: do NOT setvbuf(_IONBF) here — on this Vita newlib it silently makes
	 * every fprintf a no-op (v3 produced a 0-byte file). Plain buffered stream
	 * + fflush() per line (in logln) is what actually persists across a crash. */
	g_log = fopen("ux0:/data/es3_probe.txt", "w");

	vglInit(0x800000);

	logln("=== VitaGL ES3 probe (v4) ===");
	logln("GL_VENDOR   : %s", (const char *)glGetString(GL_VENDOR));
	logln("GL_RENDERER : %s", (const char *)glGetString(GL_RENDERER));
	logln("GL_VERSION  : %s", (const char *)glGetString(GL_VERSION));
	logln("GLSL VERSION: %s", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
	logln("------------------------------------------");

	/* API tests FIRST (they're the ones we need answers for). The 300es link
	 * test is LAST because it is known to crash the process on this VitaGL. */
	test_vao();
	test_ubo_binding();
	test_map_buffer_range();
	test_instancing();

	logln("--- API tests done; now the crashy 300es link test ---");
	test_glsl_300es(); /* decisive — expected to die at glsl300es.link */

	logln("------------------------------------------");
	logln("RESULT: %d/%d checks passed", checks_pass, checks_total);
	logln("(A crash shows as the last '>>> BEGIN x' with no '<<< END x'.)");

	if (g_log) {
		fclose(g_log);
		g_log = NULL;
	}
	sceKernelExitProcess(0);
	return 0;
}
