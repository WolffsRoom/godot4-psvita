# Building the Phase-0 probes — verified recipe

Both probes (`es3_probe/`, `canvas_render_probe/`) were built successfully with
VitaSDK (arm-vita-eabi-gcc 15.2.0) + VitaGL in WSL Ubuntu. Building surfaced
several quirks of this particular SDK/VitaGL combo; the working recipe is below.

## Quirks hit (and fixes baked into the CMakeLists / shim)

1. **`vglEnd()` doesn't exist** in this VitaGL header → removed; process exit
   tears down instead.
2. **C++ runtime**: VitaGL's preprocessor is C++ (`std::deque`), so the link
   needs `stdc++ supc++` (added to `target_link_libraries`).
3. **Splashscreen pulls `SceSharedFb`**: the SDK's installed `libvitaGL.a` uses
   `sceSharedFb*` in its core present path. Rebuilt VitaGL from the local
   `vitaGL-nosplash` source with `NO_SPLASHSCREEN=1` (see below).
4. **Stale SDK stubs**: this SDK's `libSceAppMgr_stub.a` lacks
   `_sceAppMgrGetBudgetInfo` and `_sceSharedFbOpen`, but the NID db
   (`share/vita-headers/db/360/SceAppMgr.yml`) has them → regenerated the stub
   with `vita-libs-gen`.
5. **Header/NID name mismatch**: `<psp2/appmgr.h>` declares
   `sceAppMgrGetBudgetInfo` but the db only exports `_sceAppMgrGetBudgetInfo`
   → `sdk_shims.c` bridges the two.

## Recipe

```bash
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:$PATH

# 1) Rebuild VitaGL without the splashscreen (avoids SceSharedFb-in-core issues
#    from a stale SDK). Source: the vitaGL-nosplash tree.
cp -r <path-to>/vitaGL-nosplash-src /tmp/vgl && cd /tmp/vgl
find . -name '*.o' -delete; rm -f libvitaGL.a
make NO_SPLASHSCREEN=1 NO_DEBUG=1 HAVE_GLSL_UBOS=1 HAVE_GLSL_TEXTURE_SIZE=1 -j4

# 2) Regenerate the SceAppMgr stub (adds _sceAppMgrGetBudgetInfo + SceSharedFb)
vita-libs-gen $VITASDK/share/vita-headers/db/360/SceAppMgr.yml /tmp/stubs
make -C /tmp/stubs -j4

# 3) Build a probe against the local VitaGL + regenerated stubs
cd <probe dir>            # e.g. .../tools/es3_probe
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake \
  -DCMAKE_C_FLAGS="-I/tmp/vgl/source" \
  -DCMAKE_EXE_LINKER_FLAGS="-L/tmp/stubs -L/tmp/vgl"
cmake --build build            # -> build/<name>.vpk
```

## What building already proved (hardware-independent)

The link step resolved **every ES3 API entry point `es3_probe` references**
against this VitaGL build — `glMapBufferRange`, `glUnmapBuffer`,
`glVertexAttribDivisor`, `glDrawElementsInstanced`, `glGenVertexArrays`/
`glBindVertexArray`, `glBindBufferBase`, `glGetUniformBlockIndex`,
`glUniformBlockBinding`, `glGetStringi`, `glGetShaderInfoLog`. So this SDK's
VitaGL is **newer/more complete than the `vitaGL-nosplash` source** first
audited: the API surface for those is present.

Still runtime-only (needs the real Vita + `libshacccg.suprx`): whether the Cg
shader compiler accepts `#version 300 es` (es3_probe `glsl300es.*`) and whether
the down-emitted ES 1.00 canvas shader renders (canvas_render_probe). That is
what the on-device `.txt` reports will answer.

## Note on Vita3K

Running the VPKs through Vita3K launched from this environment produced no
console output / no `ux0:/data` file (the emulator needs an attached display and
doesn't run truly headless here). More importantly, Vita3K reimplements GXM and
does not faithfully reproduce VitaGL's on-device Cg shader compilation, so its
result would not be authoritative for the decisive shader question. Test on real
hardware.
```
