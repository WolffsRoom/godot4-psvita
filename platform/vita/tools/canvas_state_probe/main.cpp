// Tests the same explicit CanvasData upload helper used by the Vita renderer.
// No UBO/instancing/integer-attribute dependency: isolate CPU -> plain uniforms.
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <vitaGL.h>
#include "../../canvas_uniforms_vita.h"

static FILE *log_file;
static void logln(const char *format, ...) {
	char line[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	sceClibPrintf("%s\n", line);
	if (log_file) {
		fprintf(log_file, "%s\n", line);
		fflush(log_file);
	}
}

struct CanvasState {
	float canvas_transform[16], screen_transform[16], canvas_normal_transform[16];
	float canvas_modulate[4], screen_pixel_size[2], time;
	unsigned int use_pixel_snap;
	float sdf_to_tex[4], sdf_to_screen[2], screen_to_sdf[2];
	unsigned int directional_light_count;
	float tex_to_sdf;
};

static const char *vertex_source =
	"attribute vec2 vertex;\n"
	"uniform mat4 canvas_transform;\n"
	"uniform mat4 screen_transform;\n"
	"void main() {\n"
	"gl_Position = screen_transform * (canvas_transform * vec4(vertex, 0.0, 1.0));\n"
	"}\n";
static const char *fragment_source =
	"precision mediump float;\n"
	"uniform vec4 canvas_modulation;\n"
	"uniform vec2 screen_pixel_size;\n"
	"uniform float time;\n"
	"uniform bool use_pixel_snap;\n"
	"void main() {\n"
	"vec3 factor = vec3(screen_pixel_size.x * 960.0, screen_pixel_size.y * 544.0, time);\n"
	"gl_FragColor = vec4(canvas_modulation.rgb * factor, canvas_modulation.a);\n"
	"if (use_pixel_snap) gl_FragColor.r *= 0.5;\n"
	"}\n";

static GLuint compile(GLenum stage, const char *source) {
	GLuint shader = glCreateShader(stage);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	char info[1024] = {};
	glGetShaderInfoLog(shader, sizeof(info), nullptr, info);
	logln("compile stage=%u ok=%d %s", stage, ok, info);
	if (!ok) {
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint create_program() {
	GLuint vs = compile(GL_VERTEX_SHADER, vertex_source);
	GLuint fs = compile(GL_FRAGMENT_SHADER, fragment_source);
	if (!vs || !fs) return 0;
	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glBindAttribLocation(program, 0, "vertex");
	logln("BEGIN program link");
	glLinkProgram(program);
	GLint ok = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	char info[1024] = {};
	glGetProgramInfoLog(program, sizeof(info), nullptr, info);
	logln("link program=%u ok=%d %s", program, ok, info);
	if (!ok) return 0;
	const char *required[] = { "canvas_transform", "screen_transform", "canvas_modulation", "screen_pixel_size", "time", "use_pixel_snap" };
	for (const char *name : required) {
		GLint location = glGetUniformLocation(program, name);
		logln("uniform %s location=%d", name, location);
		if (location == -1) return 0;
	}
	return program;
}

static void identity(float *matrix) {
	for (int i = 0; i < 16; i++) matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

static bool pixel(const char *tag, int x, int y, int r, int g, int b) {
	unsigned char value[4] = {};
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, value);
	GLenum error = glGetError();
	bool ok = error == GL_NO_ERROR && abs(int(value[0]) - r) <= 8 &&
			abs(int(value[1]) - g) <= 8 && abs(int(value[2]) - b) <= 8;
	logln("%s (%d,%d): got=%u,%u,%u expected=%d,%d,%d err=0x%04x %s",
			tag, x, y, value[0], value[1], value[2], r, g, b, error, ok ? "PASS" : "FAIL");
	return ok;
}

static bool draw_case(GLuint program, CanvasState &state, int x, int y, int old_x, int old_y, int r, int g, int b, const char *tag) {
	logln("BEGIN %s", tag);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	vita_upload_canvas_uniforms(program, state);
	GLenum upload_error = glGetError();
	logln("upload err=0x%04x", upload_error);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glFinish();
	bool inside = pixel("inside", x, y, r, g, b);
	bool outside = pixel("outside/old position", old_x, old_y, 0, 0, 0);
	logln("END %s: %s", tag, upload_error == GL_NO_ERROR && inside && outside ? "PASS" : "FAIL");
	return upload_error == GL_NO_ERROR && inside && outside;
}

int main() {
	log_file = fopen("ux0:/data/canvas_state_probe.txt", "w");
	logln("=== CanvasData plain-uniform transport probe ===");
	logln("BUILD_ID: %s", VITA_PROBE_BUILD_ID);
	logln("VITAGL_SHA256: %s", VITA_GL_SHA256);
	logln("resolution_fallback=%d", vglInit(0x800000));
	logln("GL_VERSION: %s", (const char *)glGetString(GL_VERSION));
	GLuint programs[2] = { create_program(), create_program() };
	bool ok = programs[0] && programs[1];
	if (ok) {
		// Pixel-space quad; independent screen and canvas matrices expose
		// missing uploads, transposition and stale state between passes.
		const float vertices[] = {0, 0, 160, 0, 0, 80, 160, 80};
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glViewport(0, 0, 960, 544);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		CanvasState state = {};
		identity(state.canvas_transform);
		identity(state.screen_transform);
		identity(state.canvas_normal_transform);
		state.screen_transform[0] = 2.0f / 960.0f;
		state.screen_transform[5] = 2.0f / 544.0f;
		state.screen_transform[12] = state.screen_transform[13] = -1.0f;
		state.screen_pixel_size[0] = 1.0f / 960.0f;
		state.screen_pixel_size[1] = 1.0f / 544.0f;
		state.time = 1.0f;
		state.canvas_transform[12] = 80;
		state.canvas_transform[13] = 120;
		state.canvas_modulate[0] = 1;
		state.canvas_modulate[1] = state.canvas_modulate[2] = 0.25f;
		state.canvas_modulate[3] = 1;
		bool first = draw_case(programs[0], state, 160, 160, 560, 160, 255, 64, 64, "A translated");
		state.canvas_transform[12] = 480;
		state.canvas_modulate[0] = 0.25f;
		state.canvas_modulate[1] = 1;
		state.canvas_modulate[2] = 0.5f;
		state.time = 0.5f;
		bool second = draw_case(programs[1], state, 560, 160, 160, 160, 64, 255, 64, "B switched program");
		state.canvas_transform[0] = 0.5f;
		state.canvas_transform[12] = 300;
		state.canvas_transform[13] = 320;
		state.canvas_modulate[0] = 1;
		state.canvas_modulate[1] = 0.5f;
		state.canvas_modulate[2] = 1;
		state.time = 1;
		state.use_pixel_snap = 1;
		bool third = draw_case(programs[0], state, 340, 360, 420, 360, 128, 128, 255, "A rebound scaled and updated");
		ok = first && second && third;
		vglSwapBuffers(GL_FALSE);
	}
	logln("RESULT: %s", ok ? "PASS" : "FAIL");
	if (log_file) fclose(log_file);
	sceKernelExitProcess(ok ? 0 : 1);
	return ok ? 0 : 1;
}
