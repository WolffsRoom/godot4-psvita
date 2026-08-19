extends Control

# Minimal 4.x smoke test: animate the background color and log from _process,
# proving the 2D compat renderer draws and the main loop / GDScript run on Vita.

var t := 0.0

func _ready() -> void:
	print("[game] Vita test scene _ready() — Godot ", Engine.get_version_info().string)

func _process(delta: float) -> void:
	t += delta
	$BG.color = Color.from_hsv(fmod(t * 0.2, 1.0), 0.6, 0.35)
	if int(t) != int(t - delta):
		print("[game] tick ", int(t), " fps=", Engine.get_frames_per_second())
