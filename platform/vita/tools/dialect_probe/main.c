/*
 * VitaGL dialect probe  (Godot 4.x Vita port — Phase 0, make-or-break for Path B)
 * -------------------------------------------------------------------------------
 * The real drivers/gles3/shaders/canvas.glsl is INTEGER-HEAVY: uvec4 vertex
 * attributes, `flat out uvec4` varyings, `uint`, bitwise ops (& << |),
 * unpackHalf2x16, etc. GLSL ES 1.00 has none of that — but VitaGL's shader
 * backend is Cg (SceShaccCg), which DOES support integers/bitwise.
 *
 * So Path B's size hinges on one question this probe isolates:
 *   Does a shader written in the VitaGL-friendly dialect (attribute/varying/
 *   plain uniforms/gl_FragColor/texture2D) but KEEPING int + bitwise ops
 *   compile, link and render on real hardware?
 *
 *   PASS => integers survive through VitaGL->Cg. The down-emit is purely
 *           SYNTACTIC (in/out->attribute/varying, UBO->uniform, texture->
 *           texture2D). Path B is tractable.
 *   FAIL => integer math must be rewritten in float per shader. Path B is huge.
 *
 * Runs two programs, each compile+link+draw+readback, with breadcrumbs:
 *   T1 = control: plain float ES 1.00 dialect (expected PASS, matches
 *        canvas_render_probe).
 *   T2 = same dialect + an `int` uniform and a bitwise test driving the output.
 *
 * Result -> ux0:/data/dialect_probe.txt   (plain fopen + fflush; NO setvbuf —
 * _IONBF zeroes the log on this newlib).
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>

#include <vitaGL.h>

#define SCREEN_W 960
#define SCREEN_H 544

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
static void begin(const char *t) { logln(">>> BEGIN %s", t); }
static void end(const char *t) { logln("<<< END   %s", t); }

/* ---- T1: control, plain float ES 1.00 (no integers) ---- */
static const char *VS_FLOAT =
	"uniform mat4 u_mvp;\n"
	"attribute vec2 a_vertex;\n"
	"attribute vec2 a_uv;\n"
	"varying vec2 v_uv;\n"
	"void main() { v_uv = a_uv; gl_Position = u_mvp * vec4(a_vertex, 0.0, 1.0); }\n";
static const char *FS_FLOAT =
	"precision mediump float;\n"
	"varying vec2 v_uv;\n"
	"uniform sampler2D u_tex;\n"
	"uniform vec4 u_modulate;\n"
	"void main() { gl_FragColor = texture2D(u_tex, v_uv) * u_modulate; }\n";

/* ---- T2: SAME dialect, but with an int uniform + bitwise op driving output.
 * No #version, no in/out, no UBO block — only the integer/bitwise part is new.
 * If VitaGL->Cg accepts this, Godot's integer shader math can pass through a
 * purely syntactic down-emit. ---- */
static const char *VS_INT =
	"uniform mat4 u_mvp;\n"
	"uniform int u_flags;\n" /* plain int uniform (set via glUniform1i) */
	"attribute vec2 a_vertex;\n"
	"attribute vec2 a_uv;\n"
	"varying vec2 v_uv;\n"
	"varying float v_bit;\n"
	"void main() {\n"
	"  v_uv = a_uv;\n"
	"  int masked = u_flags & 0x2;\n" /* bitwise AND */
	"  int shifted = (u_flags >> 1) & 0x1;\n" /* shift + AND */
	"  v_bit = (masked != 0) ? float(shifted) : 0.0;\n"
	"  gl_Position = u_mvp * vec4(a_vertex, 0.0, 1.0);\n"
	"}\n";
static const char *FS_INT =
	"precision mediump float;\n"
	"varying vec2 v_uv;\n"
	"varying float v_bit;\n"
	"uniform sampler2D u_tex;\n"
	"uniform vec4 u_modulate;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(u_tex, v_uv) * u_modulate * v_bit;\n"
	"}\n";

static GLuint make_prog(const char *vs_src, const char *fs_src, const char *tag) {
	char log[512];
	begin(tag);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_src, NULL);
	glCompileShader(vs);
	GLint vok = 0;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &vok);
	log[0] = 0;
	if (!vok) { GLsizei n = 0; glGetShaderInfoLog(vs, sizeof(log) - 1, &n, log); }
	logln("[%s] %s.vertex   %s", vok ? "PASS" : "FAIL", tag, vok ? "" : log);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_src, NULL);
	glCompileShader(fs);
	GLint fok = 0;
	glGetShaderiv(fs, GL_COMPILE_STATUS, &fok);
	log[0] = 0;
	if (!fok) { GLsizei n = 0; glGetShaderInfoLog(fs, sizeof(log) - 1, &n, log); }
	logln("[%s] %s.fragment %s", fok ? "PASS" : "FAIL", tag, fok ? "" : log);

	GLuint prog = 0;
	if (vok && fok) {
		prog = glCreateProgram();
		glAttachShader(prog, vs);
		glAttachShader(prog, fs);
		glBindAttribLocation(prog, 0, "a_vertex");
		glBindAttribLocation(prog, 1, "a_uv");
		logln(">>> %s.link (crash here == this construct is the wall)", tag);
		glLinkProgram(prog);
		GLint lok = 0;
		glGetProgramiv(prog, GL_LINK_STATUS, &lok);
		log[0] = 0;
		if (!lok) { GLsizei n = 0; glGetProgramInfoLog(prog, sizeof(log) - 1, &n, log); }
		logln("[%s] %s.link     %s", lok ? "PASS" : "FAIL", tag, lok ? "" : log);
	}
	glDeleteShader(vs);
	glDeleteShader(fs);
	end(tag);
	return prog;
}

/* Draw a fullscreen textured quad with prog, read back center pixel.
 * u_flags is set so the T2 integer math yields multiplier 1.0 (visible). */
static int draw_and_check(GLuint prog, const char *tag, int expR, int expG, int expB) {
	GLuint tex = 0;
	unsigned char texels[16];
	for (int i = 0; i < 4; i++) { texels[i*4+0]=255; texels[i*4+1]=128; texels[i*4+2]=64; texels[i*4+3]=255; }
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);

	const GLfloat quad[] = { -1,-1,0,0,  1,-1,1,0,  -1,1,0,1,  1,1,1,1 };
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

	const GLfloat mvp[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
	const GLfloat mod[4] = {1.0f, 0.5f, 1.0f, 1.0f};

	glViewport(0, 0, SCREEN_W, SCREEN_H);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(prog);
	glUniformMatrix4fv(glGetUniformLocation(prog, "u_mvp"), 1, GL_FALSE, mvp);
	glUniform4fv(glGetUniformLocation(prog, "u_modulate"), 1, mod);
	GLint fl = glGetUniformLocation(prog, "u_flags");
	if (fl >= 0) {
		glUniform1i(fl, 0x3); /* bit1=1 & bit0=1 -> masked!=0, shifted=1 -> v_bit=1.0 */
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(glGetUniformLocation(prog, "u_tex"), 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glFinish();

	unsigned char px[4] = {0,0,0,0};
	glReadPixels(SCREEN_W/2, SCREEN_H/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
	logln("%s center pixel: R=%d G=%d B=%d A=%d", tag, px[0], px[1], px[2], px[3]);

	int tol = 24;
	int ok = (px[0] > expR - tol) && (px[1] > expG - tol && px[1] < expG + tol) && (px[2] > expB - tol && px[2] < expB + tol);
	logln("[%s] %s render (expected ~R%d G%d B%d)", ok ? "PASS" : "FAIL", tag, expR, expG, expB);

	glUseProgram(0);
	glDeleteBuffers(1, &vbo);
	glDeleteTextures(1, &tex);
	return ok;
}

int main(void) {
	scePowerSetArmClockFrequency(444);
	scePowerSetGpuClockFrequency(222);

	g_log = fopen("ux0:/data/dialect_probe.txt", "w");
	vglInit(0x800000);

	logln("=== VitaGL dialect probe ===");
	logln("GL_VERSION  : %s", (const char *)glGetString(GL_VERSION));
	logln("GLSL VERSION: %s", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
	logln("--------------------------------");

	/* T1 control (float only). Expected R255 G64 B64 (texel*modulate). */
	GLuint p1 = make_prog(VS_FLOAT, FS_FLOAT, "T1_float");
	if (p1) {
		draw_and_check(p1, "T1_float", 255, 64, 64);
		glDeleteProgram(p1);
	}

	logln("--------------------------------");

	/* T2 integer/bitwise. u_flags=0x3 -> v_bit=1.0 -> same expected color.
	 * The DECISIVE test: if this compiles/links/renders, integers survive. */
	GLuint p2 = make_prog(VS_INT, FS_INT, "T2_int");
	if (p2) {
		draw_and_check(p2, "T2_int", 255, 64, 64);
		glDeleteProgram(p2);
	}

	logln("--------------------------------");
	logln("READ: if T2_int PASSes -> integer math survives VitaGL->Cg;");
	logln("      Path B down-emit can be purely syntactic. If T2 crashes at");
	logln("      'T2_int.link' or FAILs -> integer math needs float rewrites.");

	for (int i = 0; i < 120; i++) vglSwapBuffers(GL_FALSE);

	if (g_log) { fclose(g_log); g_log = NULL; }
	sceKernelExitProcess(0);
	return 0;
}
