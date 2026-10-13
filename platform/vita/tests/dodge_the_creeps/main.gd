extends Node

@export var mob_scene: PackedScene
var score: int = 0

const SETTINGS_FILE := "user://dodge_vita_settings.cfg"
# Avoid high-frequency synchronous log writes on real hardware/audio mixer.
const VITA_DIAGNOSTICS := false
const SPEED_CHOICES := [250, 400, 550]
const VOLUME_CHOICES := [0, 25, 50, 75, 100]
const FPS_CHOICES := [30, 45, 60]
const BACKGROUND_COLORS := [
	Color("#385f61"), # original
	Color("#275a79"), # 5
	Color("#44476e"), # 10
	Color("#714766"), # 15
	Color("#8e5652"), # 20
	Color("#536b52"), # 25
	Color("#286d6b"), # 30
	Color("#5b567b"), # 35
]
const COLOR_SCORE_STEP := 5

var chosen_speed := 400
var chosen_volume := 100
var chosen_fps := 60
var background_target: Color = BACKGROUND_COLORS[0]
var music_fade: Tween
var death_sequence: int = 0

func _load_settings() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(SETTINGS_FILE) != OK:
		return
	chosen_speed = int(cfg.get_value("game", "speed", chosen_speed))
	chosen_volume = int(cfg.get_value("audio", "volume", chosen_volume))
	chosen_fps = int(cfg.get_value("video", "fps", chosen_fps))

func _save_settings() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("game", "speed", chosen_speed)
	cfg.set_value("audio", "volume", chosen_volume)
	cfg.set_value("video", "fps", chosen_fps)
	var result := cfg.save(SETTINGS_FILE)
	if result != OK:
		printerr("[dodge-vita] could not persist settings: ", result)

func _apply_settings() -> void:
	# Limit choices to known working profiles; saved data might be stale/corrupt.
	if chosen_speed not in SPEED_CHOICES:
		chosen_speed = 400
	if chosen_volume not in VOLUME_CHOICES:
		chosen_volume = 100
	if chosen_fps not in FPS_CHOICES:
		chosen_fps = 60
	$Player.speed = chosen_speed
	Engine.max_fps = chosen_fps
	AudioServer.set_bus_volume_db(0, linear_to_db(float(chosen_volume) / 100.0))
	$HUD.set_settings(chosen_speed, chosen_volume, chosen_fps)
	printerr("[dodge-vita] settings speed=", chosen_speed,
		" volume=", chosen_volume, " fps_cap=", chosen_fps)

func _on_settings_changed(new_speed: int, new_volume: int, new_fps: int) -> void:
	chosen_speed = new_speed
	chosen_volume = new_volume
	chosen_fps = new_fps
	_apply_settings()
	_save_settings()

func _background_for_score(points: int) -> Color:
	var scaled: int = maxi(0, points)
	var segment: int = scaled / COLOR_SCORE_STEP
	var palette_index: int = segment % (BACKGROUND_COLORS.size() - 1)
	var part := float(scaled % COLOR_SCORE_STEP) / float(COLOR_SCORE_STEP)
	return BACKGROUND_COLORS[palette_index].lerp(BACKGROUND_COLORS[palette_index + 1], part)

func _update_background_for_score() -> void:
	background_target = _background_for_score(score)

func game_over():
	$ScoreTimer.stop()
	$MobTimer.stop()
	death_sequence += 1
	var sequence := death_sequence
	if music_fade != null and music_fade.is_running():
		music_fade.kill()
	# Avoid the original abrupt music cut at the exact collision frame.
	music_fade = create_tween()
	music_fade.tween_property($Music, "volume_db", -45.0, 0.30)
	music_fade.tween_callback($Music.stop)
	$DeathSound.play()
	# Let the player's sprite burst and fade finish before showing Game Over.
	await get_tree().create_timer(0.58).timeout
	if sequence == death_sequence:
		$HUD.show_game_over()


func new_game():
	$HUD.hide_start_controls()
	death_sequence += 1
	if music_fade != null and music_fade.is_running():
		music_fade.kill()
	$Music.volume_db = -8.0
	$DeathSound.stop()
	get_tree().call_group(&"mobs", &"queue_free")
	score = 0
	$Player.start($StartPosition.position)
	$StartTimer.start()
	$HUD.update_score(score)
	_update_background_for_score()
	$ColorRect.color = BACKGROUND_COLORS[0]
	$HUD.show_message("Get Ready")
	$Music.play()


func _on_MobTimer_timeout():
	# Create a new instance of the Mob scene.
	var mob = mob_scene.instantiate()

	# Choose a random location on Path2D.
	var mob_spawn_location = get_node(^"MobPath/MobSpawnLocation")
	mob_spawn_location.progress_ratio = randf()

	# Set the mob's position to a random location.
	mob.position = mob_spawn_location.position

	# Set the mob's direction perpendicular to the path direction.
	var direction = mob_spawn_location.rotation + PI / 2

	# Add some randomness to the direction.
	direction += randf_range(-PI / 4, PI / 4)
	mob.rotation = direction

	# Choose the velocity for the mob.
	var velocity = Vector2(randf_range(150.0, 250.0), 0.0)
	mob.linear_velocity = velocity.rotated(direction)

	# Spawn the mob by adding it to the Main scene.
	add_child(mob)


func _on_ScoreTimer_timeout():
	score += 1
	$HUD.update_score(score)
	_update_background_for_score()


func _on_StartTimer_timeout():
	$MobTimer.start()
	$ScoreTimer.start()


# PS Vita integration: observability and native joypad start without changing
# the demo's original game-loop, collision handling, spawns or scoring.
var vita_diag_elapsed := 0.0

func _ready() -> void:
	printerr("[dodge-vita] READY viewport=", get_viewport().get_visible_rect().size,
		" audio=", AudioServer.get_bus_count(), " physics=",
		PhysicsServer2D.get_process_info(PhysicsServer2D.INFO_ACTIVE_OBJECTS),
		" joypads=", Input.get_connected_joypads(), " joy_name=",
		Input.get_joy_name(0), " device0_known=", Input.is_joy_known(0))
	# WAV imports may default to non-looping even when importer metadata
	# requests looping; make it explicit in the running Godot scene.
	if $Music.stream is AudioStreamWAV:
		var music_wav: AudioStreamWAV = $Music.stream
		music_wav.loop_mode = AudioStreamWAV.LOOP_FORWARD
		music_wav.loop_begin = 0
		music_wav.loop_end = maxi(1,
			int(round(music_wav.get_length() * float(music_wav.get_mix_rate()))) - 1)
	_load_settings()
	_apply_settings()
	_update_background_for_score()

func _input(event: InputEvent) -> void:
	if event is InputEventJoypadButton and event.pressed:
		if VITA_DIAGNOSTICS:
			printerr("[dodge-vita] input button=", event.button_index, " device=", event.device)
		if event.button_index == JOY_BUTTON_A or event.button_index == JOY_BUTTON_START:
			if $HUD/StartButton.visible and not $HUD.settings_open:
				if VITA_DIAGNOSTICS:
					printerr("[dodge-vita] controller start pressed=", event.button_index)
				$HUD.hide_start_controls()
				new_game()
				get_viewport().set_input_as_handled()

var vita_start_was_pressed := false

func _process(delta: float) -> void:
	# A polling fallback is intentionally kept beside _input to make sure the
	# start screen works even when a focused UI Control consumes a joy event.
	var start_now := Input.is_joy_button_pressed(0, JOY_BUTTON_A) or Input.is_joy_button_pressed(0, JOY_BUTTON_START)
	if start_now and not vita_start_was_pressed and $HUD/StartButton.visible and not $HUD.settings_open:
		if VITA_DIAGNOSTICS:
			printerr("[dodge-vita] controller polling start (X/START)")
		$HUD.hide_start_controls()
		new_game()
	vita_start_was_pressed = start_now
	# Score changes the target color; blending avoids a sudden flash.
	$ColorRect.color = $ColorRect.color.lerp(background_target, minf(delta * 2.0, 1.0))
	if not VITA_DIAGNOSTICS:
		return
	vita_diag_elapsed += delta
	if vita_diag_elapsed >= 5.0:
		vita_diag_elapsed = 0.0
		var mob_animation := "none"
		var mob_nodes := get_tree().get_nodes_in_group("mobs")
		if not mob_nodes.is_empty() and is_instance_valid(mob_nodes[0]):
			var mob_sprite := mob_nodes[0].get_node_or_null("AnimatedSprite2D") as AnimatedSprite2D
			if mob_sprite != null:
				mob_animation = str(mob_sprite.animation) + ":" + str(mob_sprite.frame)
		printerr("[dodge-vita] tick fps=", Engine.get_frames_per_second(),
			" mobs=", get_tree().get_nodes_in_group("mobs").size(),
			" score=", score, " player=", $Player.position,
			" visible=", $Player.visible, " music=", $Music.playing,
			" anim=", $Player/AnimatedSprite2D.animation, " frame=", $Player/AnimatedSprite2D.frame,
			" trails=", $Player.vita_trail_instances.size(),
			" mob_anim_frame=", mob_animation,
			" joypads=", Input.get_connected_joypads(),
			" x=", Input.is_joy_button_pressed(0, JOY_BUTTON_A),
			" start=", Input.is_joy_button_pressed(0, JOY_BUTTON_START),
			" dpadL=", Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_LEFT),
			" dpadR=", Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_RIGHT),
			" lx=", Input.get_joy_axis(0, JOY_AXIS_LEFT_X),
			" ly=", Input.get_joy_axis(0, JOY_AXIS_LEFT_Y),
			" left=", Input.is_action_pressed(&"move_left"),
			" right=", Input.is_action_pressed(&"move_right"),
			" up=", Input.is_action_pressed(&"move_up"),
			" down=", Input.is_action_pressed(&"move_down"))
