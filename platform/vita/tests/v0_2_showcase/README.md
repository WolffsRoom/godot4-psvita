# Godot 4 Vita v0.2 showcase

Small, self-contained project packaged as the v0.2 demonstration VPK. It
exercises the renderer paths introduced or repaired by pull requests #1 and #2:

- per-frame `AnimatedSprite3D` mesh updates;
- Sprite3D vertex tint/modulate updates;
- transparent-surface sorting with depth writes disabled;
- the optional GXM tilt-shift resolve path;
- native 3D followed by a sharp canvas UI pass.

On Vita, press **X** to toggle tilt-shift and **Circle** to pause/resume the
animated sprite. The release VPK uses title ID `GDVT00200`.
