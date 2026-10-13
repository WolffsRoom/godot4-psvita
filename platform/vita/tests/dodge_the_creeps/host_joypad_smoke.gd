extends SceneTree

func _initialize() -> void:
	call_deferred("_test")

func _button(index: JoyButton, pressed: bool) -> void:
	var event := InputEventJoypadButton.new()
	event.device = 0
	event.button_index = index
	event.pressed = pressed
	Input.parse_input_event(event)

func _axis(index: JoyAxis, value: float) -> void:
	var event := InputEventJoypadMotion.new()
	event.device = 0
	event.axis = index
	event.axis_value = value
	Input.parse_input_event(event)

func _test() -> void:
	var scene: PackedScene = load("res://main.tscn")
	var game := scene.instantiate()
	root.add_child(game)
	await process_frame
	if not game.get_node("HUD/StartButton").visible:
		printerr("[joypad-host] FAIL: start not visible")
		quit(2)
		return
	_button(JOY_BUTTON_A, true)
	await process_frame
	_button(JOY_BUTTON_A, false)
	await process_frame
	var started: bool = game.get_node("Player").visible and game.get_node("StartTimer").time_left > 0
	var start_x: float = game.get_node("Player").position.x
	_axis(JOY_AXIS_LEFT_X, 1.0)
	await create_timer(0.22).timeout
	_axis(JOY_AXIS_LEFT_X, 0.0)
	var new_x: float = game.get_node("Player").position.x
	var analog_ok := new_x > start_x + 10
	_button(JOY_BUTTON_DPAD_LEFT, true)
	await create_timer(0.22).timeout
	_button(JOY_BUTTON_DPAD_LEFT, false)
	var dpad_x: float = game.get_node("Player").position.x
	var dpad_ok := dpad_x < new_x - 10
	printerr("[joypad-host] start=", started, " analog=", analog_ok,
		" dpad=", dpad_ok, " pos=", start_x, ",", new_x, ",", dpad_x)
	if started and analog_ok and dpad_ok:
		printerr("[joypad-host] PASS")
		quit(0)
	else:
		printerr("[joypad-host] FAIL")
		quit(3)
