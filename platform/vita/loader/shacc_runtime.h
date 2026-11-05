/* Godot4Vita v0.3: PS Vita firmware shader compiler service.
 * Optional, renderer-independent loader component. Sony module stays in ur0:
 * and is NEVER bundled, extracted or copied into the VPK.
 */
#ifndef GODOT_VITA_SHACC_RUNTIME_H
#define GODOT_VITA_SHACC_RUNTIME_H
#include <stdint.h>
#include <psp2/gxm.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum VitaShaccMode {
    VITA_SHACC_DISABLED = 0,
    VITA_SHACC_SAFE = 1,
    VITA_SHACC_EXPERIMENTAL = 2
} VitaShaccMode;

typedef struct VitaShaccInfo {
    VitaShaccMode mode;
    int probed;
    int available;
    int loaded;
    int load_rc;
    unsigned module_bytes;
    uint32_t crc32;
    char path[96];
} VitaShaccInfo;

/* Configure from app0:shacc.cfg. Default disabled for legacy VPKs. */
void vita_shacc_configure(void);
/* Detect without loading or running the Sony compiler. */
const VitaShaccInfo *vita_shacc_probe(void);
const char *vita_shacc_mode_name(void);
int vita_shacc_enabled(void);
/* Hardware diagnostic modes via app0:shacc_diag.cfg (module_only / early).
 * Never enabled in production unless the file is included explicitly. */
int vita_shacc_diagnostic_boot(void);

/* Compile only from a safe, idle render-thread boundary; caller enforces
 * no active GXM scene. Returned program is owned by vitaShaRK until
 * vita_shacc_release_output(). */
const SceGxmProgram *vita_shacc_compile_fragment(const char *source, unsigned *size_out);
void vita_shacc_release_output(void);
void vita_shacc_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif
