# Dodge the Creeps Vita demo assets

These are optional demonstration assets for the native Vita loader.

- LiveArea/: five user-supplied Vita LiveArea assets, including template.xml style a5.
- boot_anim.dat: 180 frames of the Godot Vita logo chewing animation, RGB PNG 960x544, 30 FPS, 6 seconds.
- frame_manifest.json: SHA-256 hashes of the original PNG frames.

The editable frame sequence remains in the local workspace at
GodotPSV/Load/animation/alternativa_mastigando/frames/, and the standalone
game build lives in the sibling DodgeCreepsVita project. The package itself
contains original PNG frame data, so frame source is not needed for VPK
packaging. The demo is an example of a custom pre-engine loading screen:
the shell shows pic0, then the native loader displays the animation.

Using these assets is opt-in through --boot-animation and --livearea-dir
when calling platform/vita/tools/package_runtime.py.
