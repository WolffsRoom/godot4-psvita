# Default Godot 4 Vita native pre-engine splash

custom_splash_960x544.dat is included in the generic loader VPK as app0:boot_anim.dat.

Technical format: DVAN v1; 960x544; 30 FPS; 180 PNG frames; six-second timeline. Its source is the user-provided Godot custom_splash_screen sample (MIT). The faithful static SVG and WebP asset adaptation lives under GodotPSV/Load/custom_splash_screen_vita alongside the GIF and the source license.

The GDScript AnimationPlayer and CPUParticles2D are rendered offline: before libgodot.so is loaded, the native loader cannot execute them. The loader keeps one Vita2D texture and one staging buffer, and frees the Vita2D context before starting the GXM runtime.

Use -DVITA_BOOT_ANIMATION:FILEPATH=OFF to opt out. Actual Vita visual confirmation is pending.
