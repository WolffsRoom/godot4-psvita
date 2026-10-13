extends SceneTree

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var game := (load("res://main.tscn") as PackedScene).instantiate()
    root.add_child(game)
    await process_frame
    var hud = game.get_node("HUD")
    var player = game.get_node("Player")
    var intro_hint: bool = hud.get_node("SettingsHint").visible
    var hint_clean: bool = hud.get_node("SettingsHint").text == "TRIANGLE: SETTINGS"
    var music = game.get_node("Music").stream
    var death_audio = game.get_node("DeathSound").stream
    var audio_ok: bool = music is AudioStreamWAV and death_audio is AudioStreamWAV
    var audio_rate_ok: bool = music.get_mix_rate() == 48000 and death_audio.get_mix_rate() == 48000
    var loop_ok: bool = music.get_loop_mode() == AudioStreamWAV.LOOP_FORWARD and music.get_loop_end() > 1000000
    # While input handling is disabled, the always-processing hardware polling
    # fallback must still open the settings menu with Triangle.
    hud.set_process_input(false)
    var triangle_down := InputEventJoypadButton.new()
    triangle_down.device = 0
    triangle_down.button_index = JOY_BUTTON_Y
    triangle_down.pressed = true
    Input.parse_input_event(triangle_down)
    await process_frame
    await process_frame
    var polling_ok: bool = hud.settings_open and paused
    var triangle_up := InputEventJoypadButton.new()
    triangle_up.device = 0
    triangle_up.button_index = JOY_BUTTON_Y
    triangle_up.pressed = false
    Input.parse_input_event(triangle_up)
    await process_frame
    hud.close_settings()
    hud.set_process_input(true)
    await process_frame
    game.new_game()
    await process_frame
    var hidden_ingame: bool = not hud.get_node("SettingsHint").visible and not hud.get_node("SettingsButton").visible
    player._on_body_entered(null)
    await process_frame
    var fragment_count := 0
    for obj in game.get_children():
        if obj is Sprite2D:
            fragment_count += 1
    var death_begin_ok: bool = player.dying and player.visible and fragment_count >= 6
    await create_timer(0.82).timeout
    var death_finished: bool = not player.visible and not game.get_node("Music").playing
    await create_timer(2.1).timeout
    var home_hint_ok: bool = hud.get_node("SettingsHint").visible and hud.get_node("StartButton").visible
    var passed: bool = intro_hint and hint_clean and audio_ok and audio_rate_ok and loop_ok and polling_ok and hidden_ingame and death_begin_ok and death_finished and home_hint_ok
    printerr("[audio-death-smoke] intro_hint=", intro_hint, " hint_clean=", hint_clean,
        " pcm=", audio_ok, " rate48000=", audio_rate_ok,
        " music_loops=", loop_ok, " triangle_poll=", polling_ok, " hint_hidden=", hidden_ingame,
        " death_started=", death_begin_ok, " fragments=", fragment_count,
        " death_faded=", death_finished, " hint_restored=", home_hint_ok)
    printerr("[audio-death-smoke] RESULT=", "PASS" if passed else "FAIL")
    quit(0 if passed else 7)
