extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var pack: PackedScene = load("res://main.tscn")
	var game := pack.instantiate()
	root.add_child(game)
	await process_frame
	var no_stuck_backdrop := game.get_node_or_null("HUD/StartBackdrop") == null
	var event := InputEventJoypadButton.new()
	event.device = 0
	event.button_index = JOY_BUTTON_A
	event.pressed = true
	Input.parse_input_event(event)
	await process_frame
	event.pressed = false
	Input.parse_input_event(event)
	Input.action_press("move_right")
	await create_timer(0.55).timeout
	var trail_count: int = game.get_node("Player").vita_trail_instances.size()
	var anim: AnimatedSprite2D = game.get_node("Player/AnimatedSprite2D")
	var sprite_animated: bool = anim.sprite_frames.get_frame_count(anim.animation) >= 2
	var ghosts_renderable := true
	for item in game.get_node("Player").vita_trail_instances:
		var ghost: Sprite2D = item["node"]
		ghosts_renderable = ghosts_renderable and ghost.texture != null and ghost.modulate.a > 0
	Input.action_release("move_right")
	await create_timer(0.75).timeout
	var faded: bool = game.get_node("Player").vita_trail_instances.is_empty()
	var button_hidden: bool = not game.get_node("HUD/StartButton").visible
	printerr("[visual-smoke] backdrop_removed=", no_stuck_backdrop,
		" trail_generated=", trail_count, " trail_textured=", ghosts_renderable,
		" trail_faded=", faded, " animation_frames_ok=", sprite_animated,
		" button_hidden=", button_hidden)
	var passed := no_stuck_backdrop and trail_count > 0 and ghosts_renderable and faded and sprite_animated and button_hidden
	printerr("[visual-smoke] RESULT=", "PASS" if passed else "FAIL")
	quit(0 if passed else 2)
