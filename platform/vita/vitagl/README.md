# VitaGL patches for the Godot 4 Vita port

Pinned base: VitaGL (nosplash fork), commit
`df63ce8211ce4a42d7092824ffa487bc9671105a`. The previous `ed51b21` note did not
match the local checkout. Commit and build options are recorded in
[`dependency.json`](dependency.json). The build consumes the Git commit, not
uncommitted files in the supplied checkout.

`vitagl-vita-port.patch` contains the changes needed to run Godot 4.x on top of
VitaGL. Apply it to a checkout of that VitaGL source, then build with:

    make -j4 NO_SPLASHSCREEN=1 NO_DEBUG=0 HAVE_GLSL_UBOS=1 HAVE_GLSL_TEXTURE_SIZE=1 HAVE_SHARK_LOG=1 LOG_ERRORS=2

For an isolated dependency + probe build, use
[`../tools/build_ubo_probe.py`](../tools/build_ubo_probe.py); see the
[probe instructions](../tools/ubo_binding_probe/README.md). It checks and
applies the patch to a fresh archive of the pinned commit, preserving the input
checkout. It normalizes patch line endings to LF for Windows/WSL checkouts.
The Makefile accepts `VGL_GIT_HASH` so archive builds retain their base identity.

The probe and loader both require `-DVITA_GL_ROOT=/absolute/path/to/vitagl`.
Their common CMake helper selects `libvitaGL.a` and `source/vitaGL.h` from that
directory and prints the library SHA-256. No implicit SDK VitaGL fallback is
allowed. Optional regenerated stubs use `-DVITA_STUB_DIR=/absolute/path`.
The SCons engine uses the corresponding explicit `vita_gl_root` option; see
[`../BUILDING.md`](../BUILDING.md).

Key fixes: enlarged translator buffers (the `newline[128]` -> `[2048]` stack
overflow was the root of the memory corruption on Godot 4 scene shaders),
cur_vao use-after-free guards, null-shader guards in the program-binary path,
bounded backward scans, SHADER_PAIR default, `GL_INT`/`GL_UNSIGNED_INT` vertex
attributes mapped to `UNTYPED` (GXM has no int32 attribute format), and
`-mfloat-abi=hard`.

`glBindBufferBase`/`glBindBufferRange` also set the generic `GL_UNIFORM_BUFFER`
binding (`uniform_array_unit`), matching real GL semantics. Without this, Godot's
2D canvas uploads its `StateBuffer` (canvas transform, screen size) via
`glBindBufferBase(...)` immediately followed by `glBufferData(GL_UNIFORM_BUFFER,
...)`; the generic target stayed 0, the upload failed with
`GL_INVALID_OPERATION`, the shader saw a zero transform, and the screen rendered
flat gray. This fix addresses the generic buffer upload only; it does not
prove that the engine's flattened uniforms receive UBO data. The available
hardware log for corrected probe `ubo-20261010T223545Z` reports PASS for base,
range and both payload checks, with build identity verified on 2026-10-10.
