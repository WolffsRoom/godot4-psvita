# Custom GodotPSV native boot splash (960 x 544)

Source: C:\Users\wolff\Downloads\custom_splash_screen

The supplied Godot demo is a Godot scene: a black screen, six-cell
particle flipbook and a Godot/PlayStation Vita logo, animated with an
AnimationPlayer for fade-in, brief hold, rising logo and fade-out.

The PS Vita eboot cannot display Godot scenes until libgodot.so has loaded.
We converted the demo to DVAN v1, with 180 PNG-indexed frames at 960x544,
30 FPS and 6 seconds, already readable by loader/boot_splash.c.

Files:
- boot_anim.dat: 960x544 native DVAN sequence; 180 frames / 30 FPS.
- preview_960x544.gif: visual preview at 15 FPS.
- preview_storyboard_1920x1088.jpg: still-sheet of animation steps.
- generate_vita_splash.py: Python/Pillow/CairoSVG offline animation generator.
- source/: original files from custom_splash_screen and MIT license.

Default integration:
  godot-master/platform/vita/loader/CMakeLists.txt
  VITA_BOOT_ANIMATION defaults to loader/boot_assets/custom_splash_960x544.dat

Sequence on device: LiveArea -> pic0 -> boot native splash during
so_load/so_relocate/so_resolve -> Vita2D teardown -> Godot renderer -> game.

The animation uses ONE 960x544 Vita2D texture and one CPU staging buffer,
without starting Godot before libgodot.so is initialized. Nothing runs
during gameplay, so this splash does not reduce in-game FPS.

For specialized tests, opt out at configuration time:
  -DVITA_BOOT_ANIMATION:FILEPATH=OFF

Credit: the original source splash assets and code use the MIT license from
source/LICENSE.md. Particle positioning is an offline deterministic
approximation of the original CPUParticles2D simulation; it is not a video
capture of the original renderer.

Preview VPK build: build_preview.sh (VitaSDK in WSL required).
Deploying the VPK to a real Vita is still a separate manual test.
