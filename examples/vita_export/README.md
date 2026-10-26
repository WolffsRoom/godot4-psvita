# Example PS Vita export

A minimal project exported with the v0.1 editor (`Godot4Vita.exe`), kept as a reference for what a correct export looks like.

| Path | Content |
|---|---|
| `project/` | The source project (Compatibility renderer, 960 × 544 viewport, two worker threads, `PS Vita` preset). |
| `Exemplo.vpk` | The VPK produced by **Export Project**: loader `eboot.bin`, `libgodot.so`, boot animation, `PARAM.SFO`, LiveArea `a5` and the default 8-bit LiveArea images. |
| `exemplovita/` | The separate data folder. Copy it to `ux0:data/exemplovita/` on the Vita. |

```
ux0:data/exemplovita/
  game_data/project.binary
  game_data/.godot/imported/...   (textures imported on the PC)
  game_data/.godot/exported/...   (binary scenes)
  logs/                           (created by the runtime)
```

Install `Exemplo.vpk` with VitaShell, copy `exemplovita` to `ux0:data/` and start **Godot Vita Game** (Title ID `GDOT40001`). The scene draws the GXM canvas test: rectangles, an imported texture and bitmap text.

To recreate it, open `project/` in `Godot4Vita.exe` and use **Project > Export > Export Project**, or from the command line:

```
Godot4Vita.exe --headless --path project --export-release "PS Vita" Exemplo.vpk
```
