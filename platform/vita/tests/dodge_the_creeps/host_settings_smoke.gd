extends SceneTree

func _initialize() -> void:
    call_deferred("_run")

func send_button(index: int) -> void:
    var press := InputEventJoypadButton.new()
    press.device = 0
    press.button_index = index
    press.pressed = true
    Input.parse_input_event(press)
    await process_frame
    var release := InputEventJoypadButton.new()
    release.device = 0
    release.button_index = index
    release.pressed = false
    Input.parse_input_event(release)
    await process_frame

func _run() -> void:
    var game := (load("res://main.tscn") as PackedScene).instantiate()
    root.add_child(game)
    await process_frame
    var hud = game.get_node("HUD")
    var baseline_color: Color = game.get_node("ColorRect").color
    await send_button(JOY_BUTTON_Y) # Triangle opens
    await process_frame
    var opened: bool = hud.settings_open and paused and hud.get_node("SettingsPanel").visible
    await send_button(JOY_BUTTON_DPAD_LEFT) # 400->250
    await process_frame
    var speed_ok: bool = game.chosen_speed == 250 and game.get_node("Player").speed == 250
    await send_button(JOY_BUTTON_DPAD_DOWN)
    await process_frame
    await send_button(JOY_BUTTON_DPAD_LEFT) # 100->75
    await process_frame
    var sound_ok: bool = game.chosen_volume == 75
    await send_button(JOY_BUTTON_DPAD_DOWN)
    await process_frame
    await send_button(JOY_BUTTON_DPAD_LEFT) # 60->45
    await process_frame
    var fps_ok: bool = game.chosen_fps == 45 and Engine.max_fps == 45
    await send_button(JOY_BUTTON_B) # Circle closes
    await process_frame
    var close_ok: bool = not hud.settings_open and not paused
    var stored := ConfigFile.new()
    var save_ok: bool = stored.load(game.SETTINGS_FILE) == OK and (
        int(stored.get_value("game", "speed", 0)) == 250 and
        int(stored.get_value("audio", "volume", 0)) == 75 and
        int(stored.get_value("video", "fps", 0)) == 45)
    game.score = 8
    game._update_background_for_score()
    for i in 90:
        await process_frame
    var new_color: Color = game.get_node("ColorRect").color
    var color_ok: bool = absf(baseline_color.r - new_color.r) + absf(baseline_color.g - new_color.g) + absf(baseline_color.b - new_color.b) > 0.01
    printerr("[settings-smoke] opened=", opened, " speed=", speed_ok,
        " sound=", sound_ok, " fps=", fps_ok, " closed=", close_ok,
        " persisted=", save_ok, " color=", color_ok,
        " start_color=", baseline_color, " final_color=", new_color)
    var passed: bool = opened and speed_ok and sound_ok and fps_ok and close_ok and save_ok and color_ok
    printerr("[settings-smoke] RESULT=", "PASS" if passed else "FAIL")
    var cfg := ConfigFile.new()
    cfg.set_value("game", "speed", 400)
    cfg.set_value("audio", "volume", 100)
    cfg.set_value("video", "fps", 60)
    cfg.save(game.SETTINGS_FILE)
    Engine.max_fps = 0
    quit(0 if passed else 4)
