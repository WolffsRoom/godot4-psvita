# VitaGL patches for the Godot 4 Vita port

Base: VitaGL (nosplash fork), commit `ed51b21`.

`vitagl-vita-port.patch` contains the changes needed to run Godot 4.x on top of
VitaGL. Apply it to a checkout of that VitaGL source, then build with:

    make -j4 NO_SPLASHSCREEN=1 HAVE_GLSL_UBOS=1 HAVE_SHARK_LOG=1 LOG_ERRORS=2

Key fixes: enlarged translator buffers (the `newline[128]` -> `[2048]` stack
overflow was the root of the memory corruption on Godot 4 scene shaders),
cur_vao use-after-free guards, null-shader guards in the program-binary path,
bounded backward scans, SHADER_PAIR default, and `-mfloat-abi=hard`.
