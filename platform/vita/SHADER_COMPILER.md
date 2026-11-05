# Godot4Vita v0.3 — Native shader compiler

The module is integrated into the Godot4Vita engine, not Split Memories.
It uses the user's own PS Vita libshacccg.suprx module. No firmware
file is ever shipped in a VPK.

## Architecture

- platform/vita/loader/shacc_runtime.h and shacc_runtime.c:
  reusable module discovery, validation, compile-on-demand and shutdown.
- platform/vita/loader/gxm_bridge.c:
  GXM fragment-program registration and draw using the shader patcher.
- platform/vita/gxm_shader_translator_vita.inc:
  experimental Godot canvas_item -> Cg translator subset.
- platform/vita/gxm_canvas_vita.inc:
  compilation queue, processed outside a live GXM scene.
- platform/vita/editor/addons/psvita/platform.gd:
  PS Vita export option Rendering / Shader Compiler.
- platform/vita/tools/make_editor_template.py:
  manifest field runtime_shacc=v1 when the engine has the symbols.

## Export modes

- disabled (default): does not load the Sony module and does not allocate
  extra GXM screen-texture buffers due only to a custom material.
- safe: experimental supported shaders only, <=4096 bytes Cg; prevents
  source with loops, include or discard from reaching the compiler.
- experimental: supports larger Cg sources (up to 32768 bytes), with
  risks of GPU and compiler aborts.

Modes safe and experimental require template manifest runtime_shacc=v1.
Older VPKs without the shacc.cfg setting default to disabled. The export
writes the chosen mode as app0:shacc.cfg. The option is per project.

## On-device module

Primary: ur0:/data/libshacccg.suprx
Secondary: ur0:/data/external/libshacccg.suprx

Detection checks SELF magic SCE-00, size from 256 KiB to 16 MiB and
streams CRC32. Different valid module revisions need not share CRC.
Reference PSM 2.01: 3407920 bytes and CRC32 C9D1FD85.
The module is never copied into an app and no firmware is downloaded.

Install using ShaRKF00D or the documented procedures:
https://github.com/Rinnegatamante/vitaShaRK
https://github.com/OsirizX/ShaRKF00D
https://cimmerian.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx

## Logs and diagnostics

Loader: ux0:data/FOLDER/logs/godot_loader.log
Runtime: ux0:data/FOLDER/logs/godot_stderr.log

Messages include module probe, path, size, CRC, init BEGIN / END,
shader compile BEGIN / END and compiler diagnostics. These entries
are flushed before entering the firmware compiler.

## Limitations

Full Godot 4.8 ShaderLanguage, arbitrary vertex/varying stages, advanced
postprocessing, all sampler types, and PointLight2D / LightOccluder2D are
not complete. The safe setting cannot guarantee the proprietary compiler
will not crash on a valid-looking program.
Physical PS Vita rendering tests remain mandatory.

## Build

This is a loader dependency (vitashark + SceShaccCgExt). The C service
is compiled in all Vita loaders but only activates if needed and enabled.
Runtime .so uses GXM bridge imports, with no app-specific code.


## Hardware crash regression (2026-11-02)

On physical PS Vita the original v0.3 SAFE ShaderTest reached:
    [vita-shacc] init BEGIN mode=safe path=ur0:/data/libshacccg.suprx
and crashed before init END or compile BEGIN. Split Memories R2 similarly
stopped immediately after identifying the module. Its PSP2 dump records
the thread SceShaccCg EntryCall with exception 0x30003 (prefetch abort)
and IFAR=0.

The original module was found on the same console with 3,407,920 bytes
and CRC32 C9D1FD85. Thus file presence alone does not prove the module
is able to execute in that console process; the failure is earlier
than Cg shader compilation. Relevant areas: module dependencies,
sceKernelLoadStartModule startup, PSM runtime, heap/timing and SDK stubs.

Two opt-in diagnostic modes are available by manually including the
file app0:shacc_diag.cfg in a test VPK (not a production export option):

- module_only: call sceKernelLoadStartModule once before loading Godot;
  do not initialize the vitaShaRK compiler; later compile is refused.
  Log stage: [shacc-boot] BEGIN/END in godot_loader.log.
- early: call full vitaShaRK initialization before loading Godot so that
  resources and module are available before the big runtime allocation.
- Missing file: previous lazy initialization on first shader compile.

These are only hardware debugging tools. A crash in module_only points
at firmware module execution or dependencies rather than GXP source.
If module_only works but early fails, focus on vitaShaRK's subsequent
extension/allocator/callback initialization. If early succeeds but
lazy fails, investigate startup timing or available memory.

Rendering fallback fix: if an unsupported shader samples the screen
(e.g. a transition ColorRect), the GXM canvas leaves it transparent
instead of painting opaque white over the entire menu. Screen capture
render targets are allocated lazily only once an actual screen-reading
GXP has successfully compiled. The original Godot shader source is
preserved; this is a safe fallback, not a shader effect implementation.

Do not publish a v0.3 release claiming working shader compilation until
the phases above pass on the physical Vita.


## 2026-11-02 R4 physical logs

The module_only and early runs passed on actual hardware. EARLY_INIT
continued to the main scene, but ALL translated Cg programs failed
with fatal internal error / program=0x0. R4 introduces cg_probe mode:
compiles five small independent Cg sources before the Godot .so
is loaded. Check cg_probe RESULT passes=N total=5.

The same Split Memories comparison revealed that the opaque green
half-screen was an untextured ColorRect using the original fog.gdshader.
The GXM fallback should not paint this shader's backing rectangle as
opaque when the fragment program is unavailable. The generic canvas
renderer now suppresses unsupported untextured ShaderMaterial rectangles,
while keeping ordinary textured sprite fallback and retaining the
original shader assets. This removes the green obstruction but does
NOT implement fog, PointLight2D / occluders or glass broken effects.

Failed native Cg source hashes are cached per process to avoid
repeated expensive compiler attempts for the same source.

Test packages:
GodotPSV/ShaderTest-v0.3/Diagnostic-R4/
Split Memories Vita Port/Dev/GXM-Shader-Implementation/R4/


## Technical handoff after five R5 Cg probes

The full R1–R5 chronology, observed Vita results, code paths, build
commands and next-action decision tree are documented in:
[SHACC_R5_ESTADO_E_CONTINUIDADE.md](SHACC_R5_ESTADO_E_CONTINUIDADE.md).

The five isolated R5 VPKs and the copy-ready continuation prompt are
kept in GodotPSV/ShaderTest-v0.3/Diagnostic-R5. Individual R5 hardware
results were NOT available at documentation time. Do not infer the
successful program from the aggregated R4 rc=3 log.
