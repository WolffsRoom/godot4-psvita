# Godot 4 Vita v0.2 showcase

Small, self-contained project packaged as the v0.2 demonstration VPK. It
exercises the renderer paths introduced or repaired by pull requests #1 and #2:

- per-frame `AnimatedSprite3D` mesh updates;
- Sprite3D vertex tint/modulate updates;
- transparent-surface sorting with depth writes disabled;
- the optional GXM tilt-shift resolve path;
- native 3D followed by a sharp canvas UI pass.
- a rigged Pixel Plush character with idle, run, jump, fall and roll animation;
- a layered 2D scene with shadowed text.

On Vita, **Select** opens the 2D preview and **Start** returns to the playable 3D
scene. In 3D, use the left stick to walk, the right stick to orbit the camera,
L/R to zoom, **X** to jump and **Circle** to roll. **Square** toggles tilt-shift
and **Triangle** pauses/resumes the original AnimatedSprite3D demonstration.
The release VPK uses title ID `GDVT00200` and the standard Godot4Vita LiveArea
(`icon0`, `pic0`, `bg0`, `startup` and `template.xml`).
