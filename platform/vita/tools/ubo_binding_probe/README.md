# VitaGL UBO Binding Probe

Regression test for the generic `GL_UNIFORM_BUFFER` binding semantics required by Godot 4.x.

It reproduces the sequence used by the GLES3 compatibility renderer:

1. `glBindBufferBase(GL_UNIFORM_BUFFER, ...)` then `glBufferData(GL_UNIFORM_BUFFER, ...)`.
2. `glBindBufferRange(GL_UNIFORM_BUFFER, ...)` then `glBufferSubData(GL_UNIFORM_BUFFER, ...)`.

Both indexed bind calls must also update the generic uniform-buffer binding. The probe writes `ux0:/data/ubo_binding_probe.txt` and reports `RESULT: PASS` only when both paths work.

The probe also maps the intended buffer and checks the uploaded bytes, including
the untouched portion after `glBufferSubData`. Its log contains a build ID and
the SHA-256 of the VitaGL archive linked into the executable. GL error handling
must remain enabled (`NO_DEBUG=0`). This tests CPU buffer upload semantics; it
does not validate the canvas shader's consumption of those buffers.

## Build (Linux / WSL)

Requires Python 3.12+, Git, Make, CMake, pkg-config and an installed VitaSDK with
VitaGL dependencies (`vitashark`, `SceShaccCgExt`, etc.). From the workspace root:

```sh
python3 godot-master/platform/vita/tools/build_ubo_probe.py \
  --vitagl-repo _build/vitaGL-clean \
  --output _build/ubo-new-build \
  --vitasdk /usr/local/vitasdk
```

The repository argument must contain the commit pinned in
`../../vitagl/dependency.json`. The output directory must not already exist.
No SDK installation or input checkout is modified. The output contains:

- `ubo_binding_probe.vpk`: installable test, title ID `GDUB40001`.
- `manifest.json`: versions, input hashes, VitaGL/VPK hashes and link command.
- `build.log`: dependency build, patch check and probe build diagnostics.
- `vitagl/`: patched library and matching headers, reusable by the loader.
- `probe/`: executable and intermediate files retained for diagnosis.

For a manual probe build with an already built dependency:

```sh
export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"
cmake -S godot-master/platform/vita/tools/ubo_binding_probe -B _build/ubo-manual \
  -DVITA_GL_ROOT=/absolute/path/to/patched/vitagl \
  -DVITA_PROBE_BUILD_ID=my-build
cmake --build _build/ubo-manual
```

## Device acceptance

1. Install this VPK and run **UBO Binding Probe** on the Vita.
2. Retrieve `ux0:/data/ubo_binding_probe.txt` (each run overwrites this file).
3. Match `BUILD_ID` and `VITAGL_SHA256` to the build manifest.
4. Require both payload comparisons to match and `base=PASS range=PASS` followed
   by `RESULT: PASS`. Preserve the log with the manifest and VPK.

The app exits after writing the result; returning to LiveArea is expected.
If it crashes, preserve the dump and VitaGL log alongside the build's ELF.
A successful compilation or the source guard alone is not a device PASS.

## Superseded probe: ubo-20261010T222928Z

The device log from this build reported `FAIL (VitaGL initialization)` before
running either buffer test. That was a probe bug: in the pinned VitaGL,
`vglInit()` returns the resolution-fallback flag, so a normal initialization
at the requested resolution returns `GL_FALSE`. It is not a success boolean.
The probe now logs this flag and continues to the buffer tests. The old result
does not establish a VitaGL initialization failure or a UBO failure.
