/*
 * VitaGL UBO generic-binding regression probe for the Godot 4.x Vita port.
 *
 * Godot uploads its canvas StateBuffer using the legal GLES3 sequence:
 *   glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);
 *   glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
 *
 * Per GL semantics, glBindBufferBase/glBindBufferRange also establish the
 * generic GL_UNIFORM_BUFFER binding. VitaGL historically only updated its
 * indexed UBO table, leaving glBufferData with buffer 0 and producing
 * GL_INVALID_OPERATION. This probe reproduces that exact sequence.
 *
 * Result: ux0:/data/ubo_binding_probe.txt
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>

#include <vitaGL.h>

static FILE *g_log;

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

static void clear_errors(void) {
    while (glGetError() != GL_NO_ERROR) {
    }
}

// Select the intended buffer explicitly for verification, so a successful
// write to a stale generic binding cannot produce a false PASS.
static int check_payload(GLuint ubo, const void *expected, size_t size) {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    const void *actual = glMapBufferRange(GL_UNIFORM_BUFFER, 0, size, GL_MAP_READ_BIT);
    GLenum map_err = glGetError();
    int matches = actual && map_err == GL_NO_ERROR && memcmp(actual, expected, size) == 0;
    GLboolean unmapped = actual ? glUnmapBuffer(GL_UNIFORM_BUFFER) : GL_FALSE;
    GLenum unmap_err = glGetError();
    logln("payload: match=%d map_err=0x%04x unmap_err=0x%04x", matches, map_err, unmap_err);
    return matches && unmapped == GL_TRUE && unmap_err == GL_NO_ERROR;
}

static int test_bind_base(void) {
    GLuint ubo = 0;
    const uint32_t payload[4] = { 0x11223344u, 0x55667788u, 0x99aabbccu, 0xddeeff00u };

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    GLenum setup_err = glGetError();
    if (!ubo || setup_err != GL_NO_ERROR) {
        logln("bind-base setup failed: id=%u err=0x%04x", ubo, setup_err);
        glDeleteBuffers(1, &ubo);
        return 0;
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    GLenum bind_err = glGetError();

    clear_errors();
    glBufferData(GL_UNIFORM_BUFFER, sizeof(payload), payload, GL_DYNAMIC_DRAW);
    GLenum data_err = glGetError();

    logln("bind-base: id=%u bind_err=0x%04x data_err=0x%04x", ubo, bind_err, data_err);
    int ok = bind_err == GL_NO_ERROR && data_err == GL_NO_ERROR && check_payload(ubo, payload, sizeof(payload));
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    glDeleteBuffers(1, &ubo);
    return ok;
}

static int test_bind_range(void) {
    GLuint ubo = 0;
    const uint32_t initial[8] = { 0 };
    const uint32_t patch[2] = { 0x12345678u, 0x9abcdef0u };
    const uint32_t expected[8] = { 0x12345678u, 0x9abcdef0u, 0, 0, 0, 0, 0, 0 };

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(initial), initial, GL_DYNAMIC_DRAW);
    if (glGetError() != GL_NO_ERROR) {
        logln("bind-range setup failed");
        glDeleteBuffers(1, &ubo);
        return 0;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    clear_errors();
    glBindBufferRange(GL_UNIFORM_BUFFER, 1, ubo, 0, sizeof(initial));
    GLenum bind_err = glGetError();

    clear_errors();
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(patch), patch);
    GLenum sub_err = glGetError();

    logln("bind-range: id=%u bind_err=0x%04x sub_err=0x%04x", ubo, bind_err, sub_err);
    int ok = ubo != 0 && bind_err == GL_NO_ERROR && sub_err == GL_NO_ERROR && check_payload(ubo, expected, sizeof(expected));
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, 0);
    glDeleteBuffers(1, &ubo);
    return ok;
}

int main(void) {
    scePowerSetArmClockFrequency(444);
    scePowerSetGpuClockFrequency(222);
    g_log = fopen("ux0:/data/ubo_binding_probe.txt", "w");

    logln("=== VitaGL UBO generic-binding probe ===");
    logln("BUILD_ID: %s", VITA_PROBE_BUILD_ID);
    logln("VITAGL_SHA256: %s", VITA_GL_SHA256);
    // In the pinned VitaGL, this is a resolution-fallback flag, NOT a
    // success flag. GL_FALSE is the normal native-resolution path.
    GLboolean resolution_fallback = vglInit(0x800000);
    logln("VitaGL init returned; resolution_fallback=%d", resolution_fallback);
    logln("GL_VERSION: %s", (const char *)glGetString(GL_VERSION));

    clear_errors();
    int base_ok = test_bind_base();
    int range_ok = test_bind_range();
    logln("base=%s range=%s", base_ok ? "PASS" : "FAIL", range_ok ? "PASS" : "FAIL");
    logln("RESULT: %s", (base_ok && range_ok) ? "PASS" : "FAIL");

    if (g_log) {
        fclose(g_log);
    }
    sceKernelExitProcess(0);
    return 0;
}
