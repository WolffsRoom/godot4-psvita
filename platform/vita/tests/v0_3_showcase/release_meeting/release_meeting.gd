extends Node3D

const FPS := 15
const FRAME_COUNT := 90
const GODOT_START := Vector3(-1.78, -0.02, 0.0)
const GODOT_MEET := Vector3(-0.58, -0.02, 0.0)
const LINUX_START := Vector3(1.78, -0.02, 0.0)
const LINUX_MEET := Vector3(0.58, -0.02, 0.0)

var godot_actor: Node3D
var linux_actor: Node3D
var godot_animation: AnimationPlayer
var godot_skeleton: Skeleton3D
var arm_bone := -1
var arm_base := Quaternion.IDENTITY
var was_waving := false
var capture_dir := ""
var capture_frame := 0

func _ready() -> void:
	get_viewport().transparent_bg = true
	_build_camera_and_light()
	_build_characters()
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--frames="):
			capture_dir = arg.trim_prefix("--frames=")
	if not capture_dir.is_empty():
		DirAccess.make_dir_recursive_absolute(capture_dir)
		Engine.max_fps = FPS
		capture_frame = 0
		_capture_next.call_deferred()

func _build_camera_and_light() -> void:
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color(0, 0, 0, 0)
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("dcecff")
	environment.ambient_light_energy = 1.05
	var world_environment := WorldEnvironment.new()
	world_environment.environment = environment
	add_child(world_environment)
	var light := DirectionalLight3D.new()
	light.rotation_degrees = Vector3(-35.0, -25.0, 0.0)
	light.light_color = Color("fff2d2")
	light.light_energy = 1.15
	add_child(light)
	var camera := Camera3D.new()
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.size = 3.25
	camera.position = Vector3(0.0, 1.1, 5.0)
	camera.look_at_from_position(camera.position, Vector3(0.0, 1.0, 0.0))
	add_child(camera)

func _build_characters() -> void:
	var godot_scene := load("res://models/pixel_plush/godot_pixel_plush.gltf") as PackedScene
	var linux_scene := load("res://models/linux_pixel_plush/linux_pixel_plush.tscn") as PackedScene
	if not godot_scene or not linux_scene:
		push_error("Release meeting characters could not be loaded")
		return
	godot_actor = Node3D.new()
	godot_actor.name = "GodotPixelPlush"
	godot_actor.position = GODOT_START
	add_child(godot_actor)
	var godot_model := godot_scene.instantiate()
	godot_model.scale = Vector3.ONE * 0.9
	godot_model.rotation_degrees.y = 0.0
	godot_actor.add_child(godot_model)
	godot_animation = _find_animation_player(godot_model)
	godot_skeleton = _find_skeleton(godot_model)
	if godot_skeleton:
		arm_bone = godot_skeleton.find_bone("Arm_R")
	if godot_animation and godot_animation.has_animation("running"):
		godot_animation.play("running")
	linux_actor = linux_scene.instantiate()
	linux_actor.name = "LinuxPixelPlush"
	linux_actor.position = LINUX_START
	linux_actor.scale = Vector3.ONE * 0.82
	linux_actor.set("wave", false)
	add_child(linux_actor)

func _find_animation_player(node: Node) -> AnimationPlayer:
	if node is AnimationPlayer:
		return node
	for child in node.get_children():
		var found := _find_animation_player(child)
		if found:
			return found
	return null

func _find_skeleton(node: Node) -> Skeleton3D:
	if node is Skeleton3D:
		return node
	for child in node.get_children():
		var found := _find_skeleton(child)
		if found:
			return found
	return null

func _smooth(value: float) -> float:
	value = clampf(value, 0.0, 1.0)
	return value * value * (3.0 - 2.0 * value)

func _process(_delta: float) -> void:
	var t := float(capture_frame) / float(FPS) if not capture_dir.is_empty() else fmod(Time.get_ticks_msec() / 1000.0, 6.0)
	_apply_timeline(t)

func _apply_timeline(t: float) -> void:
	if not godot_actor or not linux_actor:
		return
	var approaching := t < 1.65
	var leaving := t >= 4.55
	var walking := approaching or leaving
	var meet_weight := 1.0
	if approaching:
		meet_weight = _smooth(t / 1.65)
	elif leaving:
		meet_weight = 1.0 - _smooth((t - 4.55) / 1.45)
	godot_actor.position = GODOT_START.lerp(GODOT_MEET, meet_weight)
	linux_actor.position = LINUX_START.lerp(LINUX_MEET, meet_weight)
	var waving := t >= 1.85 and t < 4.45
	linux_actor.set("wave", waving)
	linux_actor.set("walking", walking)
	var direction_weight := 1.0 - meet_weight
	godot_actor.rotation.y = deg_to_rad(-16.0 * direction_weight)
	linux_actor.rotation.y = deg_to_rad(16.0 * direction_weight)
	if godot_animation:
		if waving and not was_waving and godot_animation.has_animation("idle"):
			godot_animation.play("idle")
			godot_animation.seek(0.0, true)
			godot_animation.pause()
			if arm_bone >= 0:
				arm_base = godot_skeleton.get_bone_pose_rotation(arm_bone)
		elif not waving:
			var wanted := "running" if walking else "idle"
			if godot_animation.current_animation != wanted and godot_animation.has_animation(wanted):
				godot_animation.play(wanted, 0.12)
	if waving and godot_skeleton and arm_bone >= 0:
		var wave_angle := -1.35 + sin((t - 1.85) * 7.2) * 0.22
		godot_skeleton.set_bone_pose_rotation(arm_bone, arm_base * Quaternion(Vector3(0, 0, 1), wave_angle))
	# A friendly whole-body wave remains readable even on the Vita-sized GIF.
	if waving:
		godot_actor.rotation.z = sin((t - 1.85) * 5.6) * 0.075
		linux_actor.rotation.z = -sin((t - 1.85) * 5.6) * 0.055
	else:
		godot_actor.rotation.z = 0.0
		linux_actor.rotation.z = 0.0
	was_waving = waving

func _capture_next() -> void:
	if capture_frame >= FRAME_COUNT:
		get_tree().quit()
		return
	_apply_timeline(float(capture_frame) / float(FPS))
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	image.save_png(capture_dir.path_join("meeting_%03d.png" % capture_frame))
	capture_frame += 1
	_capture_next.call_deferred()
