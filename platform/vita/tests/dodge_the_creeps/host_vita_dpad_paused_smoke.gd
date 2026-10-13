extends SceneTree

func _initialize() -> void:
    call_deferred("_run")

func press_button(index: int) -> void:
    var down := InputEventJoypadButton.new()
    down.device = 0
    down.button_index = index
    down.pressed = true
    Input.parse_input_event(down)
    await process_frame
    await process_frame

func release_button(index: int) -> void:
    var up := InputEventJoypadButton.new()
    up.device = 0
    up.button_index = index
    up.pressed = false
    Input.parse_input_event(up)
    await process_frame
    await process_frame

func tap_button(index: int) -> void:
    await press_button(index)
    await release_button(index)

func _run() -> void:
    var game := (load("res://main.tscn") as PackedScene).instantiate()
    root.add_child(game)
    await process_frame
    var hud = game.get_node("HUD")
    hud.set_process_input(false)
    await tap_button(JOY_BUTTON_Y)
    var open_ok: bool = hud.settings_open and paused
    await tap_button(JOY_BUTTON_DPAD_DOWN)
    var navigate_ok: bool = hud.selection == 1
    await tap_button(JOY_BUTTON_DPAD_LEFT)
    var value_ok: bool = game.chosen_volume == 75
    await tap_button(JOY_BUTTON_DPAD_UP)
    var up_ok: bool = hud.selection == 0
    await press_button(JOY_BUTTON_DPAD_DOWN)
    await create_timer(0.54, true).timeout
    await release_button(JOY_BUTTON_DPAD_DOWN)
    var repeat_ok: bool = hud.selection >= 2
    await tap_button(JOY_BUTTON_B)
    var close_ok: bool = not hud.settings_open and not paused
    game._on_settings_changed(400, 100, 60)
    hud.set_process_input(true)
    printerr("[dpad-paused-smoke] open=", open_ok, " nav=", navigate_ok,
        " change=", value_ok, " up=", up_ok, " repeat=", repeat_ok,
        " close=", close_ok)
    var result: bool = open_ok and navigate_ok and value_ok and up_ok and repeat_ok and close_ok
    printerr("[dpad-paused-smoke] RESULT=", "PASS" if result else "FAIL")
    quit(0 if result else 7)
