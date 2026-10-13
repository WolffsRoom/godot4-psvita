extends CanvasLayer

signal start_game
signal settings_changed(speed: int, volume: int, fps: int)

const SPEED_CHOICES := [250, 400, 550]
const VOLUME_CHOICES := [0, 25, 50, 75, 100]
const FPS_CHOICES := [30, 45, 60]

var settings_open := false
# _input may be swallowed by UI focus on Vita. Polling in _process is a
# second path; the latch prevents a single press from toggling twice.
var triangle_held := false
var message_tween: Tween
var selection := 0
var selected_speed := 400
var selected_volume := 100
var selected_fps := 60

func _ready() -> void:
    # The HUD must receive menu input even when the rest of the game is paused.
    process_mode = Node.PROCESS_MODE_ALWAYS
    $SettingsPanel.hide()
    _sync_start_controls()
    _refresh_settings()

func _process(_delta: float) -> void:
    var now := Input.is_joy_button_pressed(0, JOY_BUTTON_Y)
    if now and not triangle_held:
        _press_triangle()
    elif not now:
        triangle_held = false

func _press_triangle() -> void:
    if triangle_held:
        return
    triangle_held = true
    _menu_action(&"toggle")

func _sync_start_controls() -> void:
    var show_start: bool = $StartButton.visible and not settings_open
    $SettingsButton.visible = show_start
    $SettingsHint.visible = show_start

func set_settings(speed: int, volume: int, fps: int) -> void:
    selected_speed = speed
    selected_volume = volume
    selected_fps = fps
    _refresh_settings()

func _refresh_settings() -> void:
    var rows: Array[Label] = [
        $SettingsPanel/SpeedValue,
        $SettingsPanel/VolumeValue,
        $SettingsPanel/FPSValue,
        $SettingsPanel/ResumeValue
    ]
    rows[0].text = "SPEED: %d px/s" % selected_speed
    rows[1].text = "SOUND: %d%%" % selected_volume
    rows[2].text = "FPS LIMIT: %d" % selected_fps
    rows[3].text = "RETURN TO GAME"
    for i in range(rows.size()):
        rows[i].add_theme_color_override("font_color",
            Color("#ffe6a5") if i == selection else Color.WHITE)
        rows[i].text = (">  " if i == selection else "   ") + rows[i].text

func _toggle_settings() -> void:
    if settings_open:
        close_settings()
    else:
        open_settings()

func open_settings() -> void:
    if settings_open:
        return
    settings_open = true
    selection = 0
    $SettingsPanel.show()
    _sync_start_controls()
    _refresh_settings()
    get_tree().paused = true
    printerr("[dodge-vita] settings open")

func close_settings() -> void:
    if not settings_open:
        return
    settings_open = false
    $SettingsPanel.hide()
    _sync_start_controls()
    get_tree().paused = false
    printerr("[dodge-vita] settings closed")

func _adjust_selected(direction: int) -> void:
    var choices: Array = []
    var current := 0
    match selection:
        0:
            choices = SPEED_CHOICES
            current = selected_speed
        1:
            choices = VOLUME_CHOICES
            current = selected_volume
        2:
            choices = FPS_CHOICES
            current = selected_fps
        _:
            return
    var index := choices.find(current)
    if index < 0:
        index = 0
    index = clampi(index + direction, 0, choices.size() - 1)
    match selection:
        0: selected_speed = int(choices[index])
        1: selected_volume = int(choices[index])
        2: selected_fps = int(choices[index])
    settings_changed.emit(selected_speed, selected_volume, selected_fps)
    _refresh_settings()

func _menu_action(action: StringName) -> void:
    match action:
        &"toggle":
            _toggle_settings()
        &"up":
            if settings_open:
                selection = wrapi(selection - 1, 0, 4)
                _refresh_settings()
        &"down":
            if settings_open:
                selection = wrapi(selection + 1, 0, 4)
                _refresh_settings()
        &"left":
            if settings_open:
                _adjust_selected(-1)
        &"right":
            if settings_open:
                _adjust_selected(1)
        &"accept":
            if settings_open:
                if selection == 3:
                    close_settings()
                else:
                    _adjust_selected(1)
        &"back":
            if settings_open:
                close_settings()

func _input(event: InputEvent) -> void:
    if event is InputEventJoypadButton and event.pressed and not event.is_echo():
        var action := &""
        match event.button_index:
            JOY_BUTTON_Y: 
                _press_triangle()
                get_viewport().set_input_as_handled()
                return
            JOY_BUTTON_B: action = &"back"    # Circle
            JOY_BUTTON_A: action = &"accept"  # Cross
            JOY_BUTTON_DPAD_UP: action = &"up"
            JOY_BUTTON_DPAD_DOWN: action = &"down"
            JOY_BUTTON_DPAD_LEFT: action = &"left"
            JOY_BUTTON_DPAD_RIGHT: action = &"right"
        if settings_open and action != &"":
            _menu_action(action)
            get_viewport().set_input_as_handled()
    elif event is InputEventKey and event.pressed and not event.echo:
        var action := &""
        match event.keycode:
            KEY_F1: action = &"toggle"
            KEY_ESCAPE: action = &"back" if settings_open else &"toggle"
            KEY_UP: action = &"up"
            KEY_DOWN: action = &"down"
            KEY_LEFT: action = &"left"
            KEY_RIGHT: action = &"right"
            KEY_ENTER: action = &"accept"
        if action == &"toggle" or (settings_open and action != &""):
            _menu_action(action)
            get_viewport().set_input_as_handled()

func show_message(text):
    if message_tween != null and message_tween.is_running():
        message_tween.kill()
    $MessageLabel.text = text
    $MessageLabel.modulate.a = 0.0
    $MessageLabel.show()
    message_tween = create_tween()
    message_tween.tween_property($MessageLabel, "modulate:a", 1.0, 0.24)
    $MessageTimer.start()

func show_game_over():
    show_message("Game Over")
    await $MessageTimer.timeout
    $MessageLabel.text = "Dodge the\nCreeps"
    $MessageLabel.show()
    await get_tree().create_timer(1).timeout
    $StartButton.show()
    _sync_start_controls()

func update_score(score):
    $ScoreLabel.text = str(score)

func hide_start_controls() -> void:
    $StartButton.hide()
    _sync_start_controls()

func _on_StartButton_pressed():
    if settings_open:
        return
    hide_start_controls()
    start_game.emit()

func _on_SettingsButton_pressed() -> void:
    open_settings()

func _on_MessageTimer_timeout():
    $MessageLabel.hide()
