/*
 * VitaGL canvas-render probe  (Godot 4.x Vita port — Phase 0, neutral proof)
 * --------------------------------------------------------------------------
 * Question this answers: can VitaGL actually COMPILE and RENDER a shader shaped
 * like Godot's 2D canvas material? We don't touch Godot or VitaGL source here;
 * we hand-write the ES 1.00 *equivalent* of what drivers/gles3/shaders/canvas.glsl
 * does at its core:
 *
 *     transform a vertex by a matrix  ->  sample a texture at uv  ->  modulate.
 *
 * The real canvas.glsl can't run on VitaGL as-is (it uses `#version 300 es`,
 * `layout(location=N) in`, `uvec4` integer attributes and a std140 UBO — see
 * platform/vita/README.md). This probe deliberately down-emits that essence to
 * the GLSL ES 1.00 dialect VitaGL's translator targets (`attribute`/`varying`/
 * `gl_FragColor`/`texture2D`), then draws a textured quad and READS BACK the
 * center pixel to confirm the draw truly happened — no display capture needed.
 *
 * Result -> ux0:/data/canvas_render_probe.txt (mirrored to sceClibPrintf).
 *
 *   PASS  => VitaGL renders a Godot-shaped 2D shader. Path B (down-emit ES 1.00
 *            from drivers/gles3) is viable; scope it next.
 *   FAIL  => even the down-emitted shader won't compile/render. The renderer
 *            bridge is deeper than a dialect rewrite; reassess.
 *
 * Build: see CMakeLists.txt next to this file (VitaSDK + VitaGL w/ shader
 * compiler support; libshacccg.suprx must be installed on the console).
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

/* --- The down-emitted canvas shader (GLSL ES 1.00 dialect) ------------------
 * Mirrors canvas.glsl's core: mvp * vertex, texture sample, modulate multiply.
 * NOTE the deliberate differences vs. Godot 4's real shader:
 *   #version 300 es      -> (none; ES 1.00 default)
 *   in / out             -> attribute / varying
 *   layout(std140) UBO   -> plain uniforms
 *   texture()/frag out   -> texture2D() / gl_FragColor
 *   uvec4 integer attribs-> dropped (not representable here)               */
static const char *VS =
	"uniform mat4 u_mvp;\n"
	"attribute vec2 a_vertex;\n"
	"attribute vec2 a_uv;\n"
	"varying vec2 v_uv;\n"
	"void main() {\n"
	"  v_uv = a_uv;\n"
	"  gl_Position = u_mvp * vec4(a_vertex, 0.0, 1.0);\n"
	"}\n";

static const char *FS =
	"precision mediump float;\n"
	"varying vec2 v_uv;\n"
	"uniform sampler2D u_tex;\n"
	"uniform vec4 u_modulate;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(u_tex, v_uv) * u_modulate;\n"
	"}\n";

static GLuint compile(GLenum type, const char *src, const char *tag) {
	GLuint sh = glCreateShader(type);
	glShaderSource(sh, 1, &src, NULL);
	glCompileShader(sh);
	GLint ok = 0;
	glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
	char info[512] = { 0 };
	GLsizei n = 0;
	glGetShaderInfoLog(sh, sizeof(info) - 1, &n, info);
	logln("[%s] %s compile: %s", ok ? "PASS" : "FAIL", tag, (n > 0) ? info : "(no log)");
	if (!ok) {
		glDeleteShader(sh);
		return 0;
	}
	return sh;
}

int main(void) {
	scePowerSetArmClockFrequency(444);
	scePowerSetGpuClockFrequency(222);

	g_log = fopen("ux0:/data/canvas_render_probe.txt", "w");

	vglInit(0x800000);

	logln("=== VitaGL canvas-render probe ===");
	logln("GL_VERSION  : %s", (const char *)glGetString(GL_VERSION));
	logln("GLSL VERSION: %s", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
	logln("----------------------------------");

	int failed = 0;

	/* 1) Shaders */
	GLuint vs = compile(GL_VERTEX_SHADER, VS, "vertex");
	GLuint fs = compile(GL_FRAGMENT_SHADER, FS, "fragment");
	if (!vs || !fs) {
		logln("RESULT: FAIL (shader compile) — even ES 1.00 down-emit rejected.");
		goto done;
	}

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	/* Bind attribute locations before link (VitaGL maps these to Cg semantics). */
	glBindAttribLocation(prog, 0, "a_vertex");
	glBindAttribLocation(prog, 1, "a_uv");
	glLinkProgram(prog);
	GLint linked = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &linked);
	{
		char plog[512] = { 0 };
		GLsizei n = 0;
		glGetProgramInfoLog(prog, sizeof(plog) - 1, &n, plog);
		logln("[%s] program link: %s", linked ? "PASS" : "FAIL", (n > 0) ? plog : "(no log)");
	}
	if (!linked) {
		failed = 1;
		logln("RESULT: FAIL (link).");
		goto done;
	}

	/* 2) A known 2x2 texture: solid (255,128,64,255). */
	unsigned char texels[2 * 2 * 4];
	for (int i = 0; i < 4; i++) {
		texels[i * 4 + 0] = 255;
		texels[i * 4 + 1] = 128;
		texels[i * 4 + 2] = 64;
		texels[i * 4 + 3] = 255;
	}
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);

	/* 3) Fullscreen quad in NDC with uv 0..1 (two triangles). */
	const GLfloat quad[] = {
		//  x     y     u    v
		-1.f, -1.f, 0.f, 0.f,
		1.f, -1.f, 1.f, 0.f,
		-1.f, 1.f, 0.f, 1.f,
		1.f, 1.f, 1.f, 1.f
	};
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

	/* 4) Identity-ish mvp and a modulate that Godot-style tints the sample. */
	const GLfloat mvp[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	const GLfloat modulate[4] = { 1.0f, 0.5f, 1.0f, 1.0f }; /* halves green */

	glViewport(0, 0, SCREEN_W, SCREEN_H);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f); /* bg distinct from expected pixel */
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(prog);
	glUniformMatrix4fv(glGetUniformLocation(prog, "u_mvp"), 1, GL_FALSE, mvp);
	glUniform4fv(glGetUniformLocation(prog, "u_modulate"), 1, modulate);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(glGetUniformLocation(prog, "u_tex"), 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glFinish();

	/* 5) Read back the center pixel BEFORE swap (back buffer). */
	unsigned char px[4] = { 0, 0, 0, 0 };
	glReadPixels(SCREEN_W / 2, SCREEN_H / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
	GLenum e = glGetError();
	logln("center pixel: R=%d G=%d B=%d A=%d (glReadPixels err=0x%04x)", px[0], px[1], px[2], px[3], e);

	/* Expected ~ texel(255,128,64) * modulate(1,0.5,1) = (255,64,64). */
	int expR = 255, expG = 64, expB = 64;
	int tol = 24;
	int rendered_ok =
			(px[0] > expR - tol) &&
			(px[1] > expG - tol && px[1] < expG + tol) &&
			(px[2] > expB - tol && px[2] < expB + tol);
	/* Also make sure it's NOT the clear color (~26,26,26). */
	int not_bg = !(px[0] < 40 && px[1] < 40 && px[2] < 40);

	if (rendered_ok && not_bg) {
		logln("[PASS] textured+modulated quad rendered correctly.");
	} else {
		failed = 1;
		logln("[FAIL] center pixel does not match expected (255,64,64).");
		if (!not_bg) {
			logln("       (pixel is background — draw produced nothing.)");
		}
	}

	logln("----------------------------------");
	logln("RESULT: %s", failed ? "FAIL" : "PASS");
	if (!failed) {
		logln("VitaGL renders a Godot-shaped 2D shader. Path B (down-emit ES 1.00");
		logln("from drivers/gles3) is viable — scope that fork next.");
	} else {
		logln("The down-emitted shader did not render. The renderer bridge is");
		logln("deeper than a dialect rewrite; reassess before investing.");
	}

	/* Show it on screen for a few seconds too. */
	for (int i = 0; i < 180; i++) {
		vglSwapBuffers(GL_FALSE);
	}

done:
	if (g_log) {
		fclose(g_log);
		g_log = NULL;
	}
	// This VitaGL build exposes no public teardown; process exit tears down.
	sceKernelExitProcess(0);
	return 0;
}
