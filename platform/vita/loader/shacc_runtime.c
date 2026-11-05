/* Godot4Vita v0.3 — reusable vitaShaRK/SceShaccCg service.
 * Compile-time dependency: VitaSDK vitaShaRK. Runtime dependency:
 * the user's own ur0:/data/libshacccg.suprx (PSM compiler module).
 *
 * Safe mode intentionally rejects complex source instead of calling the
 * firmware compiler with a potentially unsafe translator output. The module
 * cannot catch an abort in SceShaccCg itself; logs flush BEFORE that call.
 */
#include "shacc_runtime.h"
#include "diag.h"
extern int debugPrintf(const char *fmt, ...);
#include <psp2/io/fcntl.h>
#include <psp2/kernel/modulemgr.h>
#include <stdio.h>
#include <string.h>
#include <vitashark.h>

#define SHACC_MIN_BYTES (256u * 1024u)
#define SHACC_MAX_BYTES (16u * 1024u * 1024u)
#define SHACC_SAFE_SOURCE_MAX 4096u
#define SHACC_EXPERIMENTAL_SOURCE_MAX 32768u
static VitaShaccInfo info;
static int configured;
static int probe_once;
static int load_attempted;
static char diagnostic_mode[24];
static SceUID diagnostic_module_id = -1;
static int output_pending;

static uint32_t crc32_file(SceUID file) {
    uint8_t bytes[4096];
    uint32_t crc = 0xffffffffu;
    if (sceIoLseek(file, 0, SCE_SEEK_SET) < 0) return 0;
    while (1) {
        int n = sceIoRead(file, bytes, sizeof(bytes));
        if (n < 0) return 0;
        if (!n) break;
        for (int i = 0; i < n; i++) {
            crc ^= bytes[i];
            for (int b = 0; b < 8; b++)
                crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
        }
    }
    return crc ^ 0xffffffffu;
}
const char *vita_shacc_mode_name(void) {
    if (info.mode == VITA_SHACC_SAFE) return "safe";
    if (info.mode == VITA_SHACC_EXPERIMENTAL) return "experimental";
    return "disabled";
}
void vita_shacc_configure(void) {
    if (configured) return;
    configured = 1;
    memset(&info, 0, sizeof(info));
    info.mode = VITA_SHACC_DISABLED;
    FILE *f = fopen("app0:shacc.cfg", "rb");
    if (!f) return;
    char mode[24] = {};
    if (fgets(mode, sizeof(mode), f)) {
        if (strncmp(mode, "experimental", 12) == 0) info.mode = VITA_SHACC_EXPERIMENTAL;
        else if (strncmp(mode, "safe", 4) == 0) info.mode = VITA_SHACC_SAFE;
    }
    fclose(f);
    FILE *diag = fopen("app0:shacc_diag.cfg", "rb");
    if (diag) {
        if (!fgets(diagnostic_mode, sizeof(diagnostic_mode), diag))
            diagnostic_mode[0] = '\0';
        fclose(diag);
    }
}
int vita_shacc_enabled(void) {
    vita_shacc_configure();
    return info.mode != VITA_SHACC_DISABLED;
}
const VitaShaccInfo *vita_shacc_probe(void) {
    vita_shacc_configure();
    if (probe_once || info.mode == VITA_SHACC_DISABLED) return &info;
    probe_once = 1;
    info.probed = 1;
    static const char *paths[] = {
        "ur0:/data/libshacccg.suprx",
        "ur0:/data/external/libshacccg.suprx",
    };
    for (unsigned i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
        SceUID file = sceIoOpen(paths[i], SCE_O_RDONLY, 0);
        if (file < 0) {
            fprintf(stderr, "[vita-shacc] probe path=%s open_rc=0x%08x\n", paths[i], (unsigned)file);
            fflush(stderr);
            continue;
        }
        uint8_t magic[4] = {};
        int n = sceIoRead(file, magic, 4);
        int64_t size = sceIoLseek(file, 0, SCE_SEEK_END);
        if (n == 4 && !memcmp(magic, "SCE\0", 4) &&
                size >= SHACC_MIN_BYTES && size <= SHACC_MAX_BYTES) {
            uint32_t crc = crc32_file(file);
            if (crc) {
                info.available = 1;
                info.module_bytes = (unsigned)size;
                info.crc32 = crc;
                snprintf(info.path, sizeof(info.path), "%s", paths[i]);
                fprintf(stderr, "[vita-shacc] module=present path=%s bytes=%u crc32=%08x reference_psm201=%s\n",
                        info.path, info.module_bytes, info.crc32,
                        info.module_bytes == 3407920u && crc == 0xc9d1fd85u ? "match" : "other");
                fflush(stderr);
                sceIoClose(file);
                break;
            }
        }
        fprintf(stderr, "[vita-shacc] invalid path=%s size=%lld magic=%02x%02x%02x%02x\n",
                paths[i], (long long)size, magic[0], magic[1], magic[2], magic[3]);
        fflush(stderr);
        sceIoClose(file);
    }
    if (!info.available) {
        fprintf(stderr, "[vita-shacc] missing/invalid module. Install to ur0:/data/libshacccg.suprx.\n");
        fflush(stderr);
    }
    return &info;
}
static void shacc_log(const char *message, shark_log_level level, int line) {
    static unsigned n;
    if (++n <= 80) {
        fprintf(stderr, "[vita-shacc] compiler level=%u line=%d %s\n",
                (unsigned)level, line, message ? message : "");
        fflush(stderr);
        // Before Godot initializes, stderr is not redirected to a file.
        // Persist errors in the loader's already-configured logs instead.
        debugPrintf("[vita-shacc] compiler level=%u line=%d %s\n",
                (unsigned)level, line, message ? message : "");
    }
}
static int load_compiler(void) {
    const VitaShaccInfo *status = vita_shacc_probe();
    if (!status->available || status->mode == VITA_SHACC_DISABLED) return 0;
    if (info.loaded) return 1;
    if (load_attempted) return 0;
    load_attempted = 1;
    shark_install_log_cb(shacc_log);
    fprintf(stderr, "[vita-shacc] init BEGIN mode=%s path=%s\n", vita_shacc_mode_name(), info.path);
    fflush(stderr);
    const int rc = shark_init(info.path);
    info.load_rc = rc;
    info.loaded = (rc >= 0);
    fprintf(stderr, "[vita-shacc] init END rc=0x%08x loaded=%d\n", (unsigned)rc, info.loaded);
    fflush(stderr);
    return info.loaded;
}
int vita_shacc_diagnostic_boot(void) {
    vita_shacc_configure();
    if (!vita_shacc_enabled() || !diagnostic_mode[0]) return 0;
    if (!strncmp(diagnostic_mode, "module_only", 11)) {
        const VitaShaccInfo *status = vita_shacc_probe();
        if (!status->available) return -1;
        fprintf(stderr, "[vita-shacc] diagnostic module_only BEGIN path=%s\n", status->path);
        fflush(stderr);
        diagnostic_module_id = sceKernelLoadStartModule(status->path, 0, NULL, 0, NULL, NULL);
        fprintf(stderr, "[vita-shacc] diagnostic module_only END module_id=0x%08x\n",
                (unsigned)diagnostic_module_id);
        fflush(stderr);
        return diagnostic_module_id >= 0 ? 1 : -2;
    }
    if (!strncmp(diagnostic_mode, "cg_probe", 8)) {
        // A probe can be selected independently as cg_probe_0 through
        // cg_probe_4; this avoids poisoning the next attempt after a
        // proprietary-compiler failure. Bare cg_probe tests all five.
        static const char *const probes[] = {
            "float4 main(uniform float4 uClearColor) : COLOR { return uClearColor; }",
            "float4 main() : COLOR { return float4(1.0, 0.0, 0.0, 1.0); }",
            "float4 main(float2 uv : TEXCOORD0) : COLOR { return float4(uv.x, uv.y, 0.2, 1.0); }",
            "uniform float4 gxm_tint; float4 main(float2 uv : TEXCOORD0) : COLOR { float4 COLOR = float4(uv.x, uv.y, 0.2, 1.0); return COLOR * gxm_tint; }",
            "uniform sampler2D TEXTURE : TEXUNIT0; uniform float4 gxm_tint; float4 main(float2 uv : TEXCOORD0) : COLOR { return tex2D(TEXTURE, uv) * gxm_tint; }",
        };
        const unsigned total = sizeof(probes) / sizeof(probes[0]);
        int chosen = -1;
        if (diagnostic_mode[8] == '_' && diagnostic_mode[9] >= '0' &&
                diagnostic_mode[9] <= '4' &&
                (diagnostic_mode[10] == '\0' || diagnostic_mode[10] == '\n' ||
                        diagnostic_mode[10] == '\r'))
            chosen = diagnostic_mode[9] - '0';
        debugPrintf("[vita-shacc] cg_probe version=R5 selected=%d total=%u mode=%s\n",
                chosen, chosen < 0 ? total : 1, vita_shacc_mode_name());
        if (!load_compiler()) {
            debugPrintf("[vita-shacc] cg_probe FAILED init rc=0x%08x\n", (unsigned)info.load_rc);
            return -4;
        }
        int successes = 0;
        for (unsigned i = 0; i < total; i++) {
            if (chosen >= 0 && i != (unsigned)chosen) continue;
            unsigned bytes = 0;
            debugPrintf("[vita-shacc] cg_probe #%u BEGIN bytes=%u source=%s\n",
                    i, (unsigned)strlen(probes[i]), probes[i]);
            const SceGxmProgram *gxp = vita_shacc_compile_fragment(probes[i], &bytes);
            if (gxp && bytes) successes++;
            debugPrintf("[vita-shacc] cg_probe #%u END gxp=%d gxp_bytes=%u\n",
                    i, gxp != NULL, bytes);
            if (gxp && bytes) {
                // Save independently compiled fragment binary to diagnose GXP
                // metadata and vertex/fragment linkage during later tests.
                char outfile[180];
                snprintf(outfile, sizeof(outfile), "%s/cg_probe_%u.gxp", vita_log_dir(), i);
                FILE *f = fopen(outfile, "wb");
                if (f) {
                    size_t written = fwrite(gxp, 1, bytes, f);
                    fclose(f);
                    debugPrintf("[vita-shacc] cg_probe #%u binary=%s saved_bytes=%u\n",
                            i, outfile, (unsigned)written);
                } else {
                    debugPrintf("[vita-shacc] cg_probe #%u GXP save failed path=%s\n",
                            i, outfile);
                }
            }
            vita_shacc_release_output();
        }
        debugPrintf("[vita-shacc] cg_probe RESULT passes=%d total=%u selected=%d\n",
                successes, chosen < 0 ? total : 1, chosen);
        return successes ? 3 : -5;
    }
    if (!strncmp(diagnostic_mode, "early", 5)) {
        fprintf(stderr, "[vita-shacc] diagnostic early vitaShaRK BEGIN\n");
        fflush(stderr);
        int loaded = load_compiler();
        fprintf(stderr, "[vita-shacc] diagnostic early vitaShaRK END loaded=%d rc=0x%08x\n",
                loaded, (unsigned)info.load_rc);
        fflush(stderr);
        return loaded ? 2 : -3;
    }
    return 0;
}
static int source_allowed(const char *src, size_t bytes) {
    if (bytes == 0 || bytes > SHACC_EXPERIMENTAL_SOURCE_MAX) return 0;
    if (info.mode == VITA_SHACC_EXPERIMENTAL) return 1;
    if (bytes > SHACC_SAFE_SOURCE_MAX) return 0;
    /* Cg syntax produced from a supported Godot canvas_item subset.
       Runtime guard remains even if the editor's translator missed a case. */
    if (strstr(src, "for (") || strstr(src, "for(") ||
        strstr(src, "while (") || strstr(src, "while(") ||
        strstr(src, "do {") || strstr(src, "#include") ||
        strstr(src, "textureLod(") || strstr(src, "discard")) return 0;
    return 1;
}
const SceGxmProgram *vita_shacc_compile_fragment(const char *source, unsigned *size_out) {
    if (size_out) *size_out = 0;
    if (!source || !size_out) return NULL;
    vita_shacc_configure();
    if (info.mode == VITA_SHACC_DISABLED) return NULL;
    const size_t bytes = strnlen(source, SHACC_EXPERIMENTAL_SOURCE_MAX + 1);
    if (!source_allowed(source, bytes)) {
        fprintf(stderr, "[vita-shacc] refused source_bytes=%u mode=%s reason=unsupported_or_complex\n",
                (unsigned)bytes, vita_shacc_mode_name());
        fflush(stderr);
        return NULL;
    }
    if (!strncmp(diagnostic_mode, "module_only", 11)) {
        fprintf(stderr, "[vita-shacc] diagnostic module_only: native compile intentionally blocked\n");
        fflush(stderr);
        return NULL;
    }
    if (!load_compiler()) return NULL;
    if (output_pending) {
        fprintf(stderr, "[vita-shacc] refused compile with output still pending\n");
        fflush(stderr);
        return NULL;
    }
    *size_out = (unsigned)bytes;
    fprintf(stderr, "[vita-shacc] compile BEGIN stage=fragment bytes=%u\n", *size_out);
    fflush(stderr);
    const SceGxmProgram *program = shark_compile_shader_extended(source, size_out,
            SHARK_FRAGMENT_SHADER, SHARK_OPT_SAFE, SHARK_DISABLE, SHARK_DISABLE, SHARK_DISABLE);
    output_pending = program != NULL;
    fprintf(stderr, "[vita-shacc] compile END program=%p gxp_bytes=%u\n", (void *)program, *size_out);
    fflush(stderr);
    if (!program || !*size_out) {
        shark_clear_output();
        output_pending = 0;
        return NULL;
    }
    return program;
}
void vita_shacc_release_output(void) {
    if (output_pending && info.loaded) shark_clear_output();
    output_pending = 0;
}
void vita_shacc_shutdown(void) {
    vita_shacc_release_output();
    if (info.loaded) shark_end();
    info.loaded = 0;
    load_attempted = 0;
}
