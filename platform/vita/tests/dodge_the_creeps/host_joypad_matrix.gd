extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _btn(index: JoyButton, value: bool) -> void:
	var ev := InputEventJoypadButton.new()
	ev.device = 0
	ev.button_index = index
	ev.pressed = value
	Input.parse_input_event(ev)

func _run() -> void:
	var game := (load("res://main.tscn") as PackedScene).instantiate()
	root.add_child(game)
	await process_frame
	_btn(JOY_BUTTON_START, true)
	await process_frame
	_btn(JOY_BUTTON_START, false)
	await process_frame
	var started: bool = game.get_node("Player").visible
	var cases: Array = [
		[JOY_BUTTON_DPAD_RIGHT, "move_right", 1.0, 0.0],
		[JOY_BUTTON_DPAD_LEFT, "move_left", -1.0, 0.0],
		[JOY_BUTTON_DPAD_UP, "move_up", 0.0, -1.0],
		[JOY_BUTTON_DPAD_DOWN, "move_down", 0.0, 1.0]
	]
	var all_ok: bool = started
	for c in cases:
		var before: Vector2 = game.get_node("Player").position
		_btn(c[0], true)
		await process_frame
		var action_state: bool = Input.is_action_pressed(c[1])
		await create_timer(0.12).timeout
		_btn(c[0], false)
		await process_frame
		var movement: Vector2 = game.get_node("Player").position - before
		var moved: bool = movement.dot(Vector2(c[2], c[3])) > 5
		printerr("[joypad-matrix] ", c[1], " action=", action_state, " moved=", moved, " delta=", movement)
		all_ok = all_ok and action_state and moved
	printerr("[joypad-matrix] START=", started, " RESULT=", "PASS" if all_ok else "FAIL")
	quit(0 if all_ok else 3)
