extends Control

var elapsed := 0.0
var phase := -1

func record(message: String) -> void:
	# stderr is captured successfully by the Vita loader, unlike current stdout.
	printerr("[colorrect] ", message)

func _ready() -> void:
	record("READY engine=" + Engine.get_version_info().string)
	print("[colorrect] Expected: black background; red left, green right, purple lower rectangle, changing every 3 seconds.")

func _process(delta: float) -> void:
	elapsed += delta
	var next_phase := int(elapsed / 3.0) % 3
	if next_phase == phase:
		return
	phase = next_phase
	match phase:
		0:
			$Rect.position = Vector2(80, 120)
			$Rect.size = Vector2(160, 80)
			$Rect.color = Color(1, 0.25, 0.25, 1)
		1:
			$Rect.position = Vector2(480, 120)
			$Rect.size = Vector2(160, 80)
			$Rect.color = Color(0.25, 1, 0.25, 1)
		2:
			$Rect.position = Vector2(300, 320)
			$Rect.size = Vector2(80, 80)
			$Rect.color = Color(0.5, 0.5, 1, 1)
	record("phase=%d position=%s size=%s fps=%s" % [phase, $Rect.position, $Rect.size, Engine.get_frames_per_second()])
