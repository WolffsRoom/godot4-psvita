extends Node3D

var world_3d: Node3D
var camera: Camera3D
var hero: AnimatedSprite3D
var glass_sprites: Array[Sprite3D] = []
var moving_lights: Array[OmniLight3D] = []
var preview_2d: CanvasLayer
var sprite_2d: Sprite2D
var hud_3d: CanvasLayer
var status_label: Label
var mode_2d := false
var character: Node3D
var character_model: Node3D
var character_animation: AnimationPlayer
var character_velocity := Vector3.ZERO
var character_ground_y := -0.22
var character_animation_name := ""
var rolling_time := 0.0
var animation_enabled := true
var tilt_enabled := true
var elapsed := 0.0
var camera_yaw := 0.0
var camera_pitch := -0.18
var camera_distance := 8.0
var camera_target := Vector3(0.0, 0.8, 0.0)

func _ready() -> void:
	_build_environment()
	_build_camera()
	_build_world()
	_build_character()
	_build_sprite_showcase()
	_build_3d_hud()
	_build_2d_preview()
	_update_mode()

func _build_environment() -> void:
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color("8fcdf4")
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("d8efff")
	environment.ambient_light_energy = 1.25
	var world_environment := WorldEnvironment.new()
	world_environment.environment = environment
	add_child(world_environment)

func _build_camera() -> void:
	camera = Camera3D.new()
	add_child(camera)
	_update_camera()

func _material(color: Color) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = 0.72
	return material

func _mesh_node(mesh: PrimitiveMesh, position: Vector3, color: Color) -> MeshInstance3D:
	var node := MeshInstance3D.new()
	mesh.material = _material(color)
	node.mesh = mesh
	node.position = position
	world_3d.add_child(node)
	return node

func _box(position: Vector3, size: Vector3, color: Color) -> void:
	var mesh := BoxMesh.new()
	mesh.size = size
	_mesh_node(mesh, position, color)

func _build_world() -> void:
	world_3d = Node3D.new()
	world_3d.name = "BrightGXM3DWorld"
	add_child(world_3d)
	_box(Vector3(0.0, -0.35, 0.0), Vector3(10.0, 0.25, 10.0), Color("e9f4ff"))
	for z in range(-3, 4):
		var shade := 0.42 + float(z + 3) * 0.045
		_box(Vector3(-3.8, 0.35, float(z)), Vector3(0.55, 1.35, 0.55), Color(shade, 0.56, 0.92))
		_box(Vector3(3.8, 0.35, float(z)), Vector3(0.55, 1.35, 0.55), Color(0.95, shade, 0.42))
		_box(Vector3(0.0, -0.18, float(z)), Vector3(0.10, 0.035, 0.65), Color("167dba"))
	for x in [-2.8, -1.4, 1.4, 2.8]:
		_box(Vector3(x, 0.02, -3.8), Vector3(0.75, 0.5, 0.75), Color("70d6a4"))
	var sphere := SphereMesh.new()
	sphere.radius = 0.68
	sphere.height = 1.36
	_mesh_node(sphere, Vector3(-2.1, 0.5, -1.3), Color("ff5c8a"))
	var cylinder := CylinderMesh.new()
	cylinder.top_radius = 0.52
	cylinder.bottom_radius = 0.78
	cylinder.height = 1.45
	_mesh_node(cylinder, Vector3(2.15, 0.38, -1.5), Color("32c7d9"))
	var torus := TorusMesh.new()
	torus.inner_radius = 0.42
	torus.outer_radius = 0.95
	var torus_node := _mesh_node(torus, Vector3(0.0, 1.1, -2.5), Color("ffd166"))
	torus_node.rotation_degrees.x = 72.0
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52.0, -28.0, 0.0)
	sun.light_color = Color("fff0d0")
	sun.light_energy = 1.1
	world_3d.add_child(sun)
	for data in [[Color("ff4d8d"), 0.0], [Color("56d6ff"), PI]]:
		var light := OmniLight3D.new()
		light.light_color = data[0]
		light.light_energy = 2.4
		light.omni_range = 4.5
		world_3d.add_child(light)
		moving_lights.append(light)

func _find_animation_player(node: Node) -> AnimationPlayer:
	if node is AnimationPlayer:
		return node
	for child in node.get_children():
		var found := _find_animation_player(child)
		if found:
			return found
	return null

func _build_character() -> void:
	character = Node3D.new()
	character.name = "PixelPlushCharacter"
	character.position = Vector3(0.0, character_ground_y, 1.7)
	world_3d.add_child(character)
	var plush_scene := load("res://models/pixel_plush/godot_pixel_plush.gltf") as PackedScene
	if not plush_scene:
		push_error("Não foi possível carregar o personagem Pixel Plush")
		return
	character_model = plush_scene.instantiate()
	character_model.scale = Vector3.ONE * 0.72
	character_model.rotation_degrees.y = 180.0
	character.add_child(character_model)
	character_animation = _find_animation_player(character_model)
	_play_character_animation("idle")

func _play_character_animation(name: String) -> void:
	if not character_animation or character_animation_name == name:
		return
	if character_animation.has_animation(name):
		character_animation.play(name, 0.15)
		character_animation_name = name

func _make_sprite_texture(primary: Color, accent: Color, phase: int) -> ImageTexture:
	var image := Image.create(48, 64, false, Image.FORMAT_RGBA8)
	image.fill(Color(0, 0, 0, 0))
	for y in range(8, 61):
		for x in range(7, 41):
			var dx := absf(float(x) - 24.0)
			var body := dx < (14.0 - absf(float(y) - 34.0) * 0.16)
			if body:
				var checker := ((x / 6 + y / 6 + phase) as int) % 2 == 0
				image.set_pixel(x, y, primary if checker else accent)
	for eye_x in [18, 30]:
		image.set_pixel(eye_x, 22, Color.WHITE)
		image.set_pixel(eye_x, 23, Color.WHITE)
	return ImageTexture.create_from_image(image)

func _make_glass_texture(color: Color) -> ImageTexture:
	var image := Image.create(64, 64, false, Image.FORMAT_RGBA8)
	image.fill(Color(0, 0, 0, 0))
	for y in range(64):
		for x in range(64):
			var edge: int = mini(mini(x, 63 - x), mini(y, 63 - y))
			if edge < 5 or ((x + y) % 17 < 3):
				image.set_pixel(x, y, Color(color.r, color.g, color.b, 0.54))
	return ImageTexture.create_from_image(image)

func _build_sprite_showcase() -> void:
	var frames := SpriteFrames.new()
	frames.add_animation("pulse")
	frames.set_animation_speed("pulse", 5.0)
	frames.set_animation_loop("pulse", true)
	frames.add_frame("pulse", _make_sprite_texture(Color("ffcf4a"), Color("f45b69"), 0))
	frames.add_frame("pulse", _make_sprite_texture(Color("5ce1e6"), Color("7b61ff"), 1))
	hero = AnimatedSprite3D.new()
	hero.sprite_frames = frames
	hero.animation = "pulse"
	hero.pixel_size = 0.019
	hero.position = Vector3(0.0, 0.72, 0.0)
	hero.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	hero.play()
	world_3d.add_child(hero)
	var colors := [Color("ff4d8d"), Color("56d6ff"), Color("ffd166"), Color("8d7aff")]
	var positions := [Vector3(-1.35, 0.8, 1.1), Vector3(0.75, 0.9, 0.45), Vector3(1.45, 0.82, -0.75), Vector3(-0.7, 0.76, -1.4)]
	for i in range(4):
		var sprite := Sprite3D.new()
		sprite.texture = _make_glass_texture(colors[i])
		sprite.pixel_size = 0.022
		sprite.position = positions[i]
		sprite.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		sprite.modulate = Color(colors[i], 0.88)
		glass_sprites.append(sprite)
		world_3d.add_child(sprite)

func _label(parent: Node, text: String, position: Vector2, size: int) -> Label:
	var label := Label.new()
	label.text = text
	label.position = position
	label.add_theme_font_size_override("font_size", size)
	label.add_theme_color_override("font_shadow_color", Color(0.0, 0.0, 0.0, 0.88))
	label.add_theme_constant_override("shadow_offset_x", 3)
	label.add_theme_constant_override("shadow_offset_y", 3)
	label.add_theme_constant_override("shadow_outline_size", 2)
	parent.add_child(label)
	return label

func _build_3d_hud() -> void:
	hud_3d = CanvasLayer.new()
	add_child(hud_3d)
	var panel := ColorRect.new()
	panel.position = Vector2(18, 16)
	panel.size = Vector2(680, 102)
	panel.color = Color(0.025, 0.055, 0.12, 0.86)
	hud_3d.add_child(panel)
	_label(hud_3d, "GODOT 4 VITA v0.2  |  GXM3D", Vector2(36, 27), 25)
	status_label = _label(hud_3d, "", Vector2(37, 66), 17)
	_label(hud_3d, "Esquerdo: andar  Direito: câmera  X: pular  O: rolar  L/R: zoom  SELECT: 2D", Vector2(20, 505), 16)

func _polygon(parent: Node, points: PackedVector2Array, color: Color) -> Polygon2D:
	var polygon := Polygon2D.new()
	polygon.polygon = points
	polygon.color = color
	parent.add_child(polygon)
	return polygon

func _build_2d_preview() -> void:
	preview_2d = CanvasLayer.new()
	add_child(preview_2d)
	var background := ColorRect.new()
	background.size = Vector2(960, 544)
	background.color = Color("263d78")
	preview_2d.add_child(background)
	for i in range(7):
		var glow := Polygon2D.new()
		glow.polygon = PackedVector2Array([Vector2(-40 + i * 170, 0), Vector2(80 + i * 170, 0), Vector2(310 + i * 170, 544), Vector2(100 + i * 170, 544)])
		glow.color = Color(0.15 + i * 0.025, 0.28, 0.58 + i * 0.025, 0.16)
		preview_2d.add_child(glow)
	for star_position in [Vector2(90, 110), Vector2(180, 76), Vector2(370, 135), Vector2(560, 82), Vector2(720, 126), Vector2(860, 64)]:
		var star := ColorRect.new()
		star.position = star_position
		star.size = Vector2(7, 7)
		star.rotation = PI / 4.0
		star.color = Color("ffe89a")
		preview_2d.add_child(star)
	_polygon(preview_2d, PackedVector2Array([Vector2(0, 365), Vector2(155, 205), Vector2(290, 365)]), Color("6058a8"))
	_polygon(preview_2d, PackedVector2Array([Vector2(180, 365), Vector2(405, 172), Vector2(590, 365)]), Color("7266ba"))
	_polygon(preview_2d, PackedVector2Array([Vector2(475, 365), Vector2(700, 218), Vector2(960, 365)]), Color("514c99"))
	var ground := ColorRect.new()
	ground.position = Vector2(0, 350)
	ground.size = Vector2(960, 194)
	ground.color = Color("163f54")
	preview_2d.add_child(ground)
	var grass := ColorRect.new()
	grass.position = Vector2(0, 350)
	grass.size = Vector2(960, 18)
	grass.color = Color("4ed49a")
	preview_2d.add_child(grass)
	var card := NinePatchRect.new()
	card.position = Vector2(52, 142)
	card.size = Vector2(310, 180)
	card.texture = _make_glass_texture(Color("56d6ff"))
	card.patch_margin_left = 12
	card.patch_margin_top = 12
	card.patch_margin_right = 12
	card.patch_margin_bottom = 12
	card.modulate = Color(0.45, 0.9, 1.0, 0.88)
	preview_2d.add_child(card)
	_polygon(preview_2d, PackedVector2Array([Vector2(610, 170), Vector2(890, 205), Vector2(820, 330), Vector2(565, 305)]), Color("ff5c8a"))
	var line := Line2D.new()
	line.points = PackedVector2Array([Vector2(430, 470), Vector2(545, 405), Vector2(670, 476), Vector2(850, 395)])
	line.width = 12.0
	line.default_color = Color("ffd166")
	preview_2d.add_child(line)
	sprite_2d = Sprite2D.new()
	sprite_2d.texture = _make_sprite_texture(Color("ffcf4a"), Color("7b61ff"), 0)
	sprite_2d.position = Vector2(205, 252)
	sprite_2d.scale = Vector2(2.4, 2.4)
	preview_2d.add_child(sprite_2d)
	_label(preview_2d, "GODOT4VITA • MUNDO 2D", Vector2(28, 22), 30)
	_label(preview_2d, "Camadas • montanhas • NinePatch • Polygon2D • Line2D • transparência", Vector2(30, 65), 18)
	_label(preview_2d, "PIXEL HERO", Vector2(126, 170), 20)
	_label(preview_2d, "START: controlar o Pixel Plush em 3D", Vector2(28, 500), 18)

func _deadzone(value: float) -> float:
	return 0.0 if absf(value) < 0.16 else value

func _update_camera() -> void:
	var direction := Vector3(cos(camera_pitch) * sin(camera_yaw), sin(camera_pitch), cos(camera_pitch) * cos(camera_yaw))
	camera.position = camera_target + direction * camera_distance
	camera.look_at(camera_target, Vector3.UP)

func _process_camera(delta: float) -> void:
	var rx := _deadzone(Input.get_joy_axis(0, JoyAxis.JOY_AXIS_RIGHT_X))
	var ry := _deadzone(Input.get_joy_axis(0, JoyAxis.JOY_AXIS_RIGHT_Y))
	camera_yaw -= rx * delta * 1.8
	camera_pitch = clampf(camera_pitch - ry * delta * 1.35, -0.72, 0.38)
	if character:
		camera_target = camera_target.lerp(character.position + Vector3(0.0, 0.9, 0.0), minf(delta * 7.0, 1.0))
	if Input.is_joy_button_pressed(0, JoyButton.JOY_BUTTON_LEFT_SHOULDER):
		camera_distance = maxf(3.5, camera_distance - delta * 3.2)
	if Input.is_joy_button_pressed(0, JoyButton.JOY_BUTTON_RIGHT_SHOULDER):
		camera_distance = minf(12.0, camera_distance + delta * 3.2)
	_update_camera()

func _process_character(delta: float) -> void:
	if not character:
		return
	var lx := _deadzone(Input.get_joy_axis(0, JoyAxis.JOY_AXIS_LEFT_X))
	var ly := _deadzone(Input.get_joy_axis(0, JoyAxis.JOY_AXIS_LEFT_Y))
	var camera_right := Vector3(cos(camera_yaw), 0.0, -sin(camera_yaw))
	var camera_forward := Vector3(-sin(camera_yaw), 0.0, -cos(camera_yaw))
	var movement := camera_right * lx + camera_forward * ly
	if movement.length_squared() > 1.0:
		movement = movement.normalized()
	var grounded := character.position.y <= character_ground_y + 0.01
	if grounded:
		character.position.y = character_ground_y
		character_velocity.y = 0.0
		if Input.is_action_just_pressed("jump"):
			character_velocity.y = 5.4
			grounded = false
	if grounded and Input.is_action_just_pressed("roll"):
		rolling_time = 0.58
	rolling_time = maxf(0.0, rolling_time - delta)
	var speed := 5.2 if rolling_time > 0.0 else 3.1
	character_velocity.x = movement.x * speed
	character_velocity.z = movement.z * speed
	character_velocity.y -= 12.5 * delta
	character.position += character_velocity * delta
	character.position.x = clampf(character.position.x, -4.3, 4.3)
	character.position.z = clampf(character.position.z, -4.3, 4.3)
	if character.position.y < character_ground_y:
		character.position.y = character_ground_y
		character_velocity.y = 0.0
		grounded = true
	if movement.length_squared() > 0.025:
		character.rotation.y = lerp_angle(character.rotation.y, atan2(movement.x, movement.z), minf(delta * 11.0, 1.0))
	if rolling_time > 0.0:
		_play_character_animation("rolling")
	elif not grounded:
		_play_character_animation("jump" if character_velocity.y > 0.0 else "falling")
	elif movement.length_squared() > 0.025:
		_play_character_animation("running")
	else:
		_play_character_animation("idle")

func _process(delta: float) -> void:
	elapsed += delta
	if Input.is_action_just_pressed("switch_to_2d"):
		mode_2d = true
		_update_mode()
	if Input.is_action_just_pressed("switch_to_3d"):
		mode_2d = false
		_update_mode()
	if not mode_2d:
		_process_character(delta)
		_process_camera(delta)
		for i in range(glass_sprites.size()):
			var sprite := glass_sprites[i]
			sprite.position.y = 0.78 + sin(elapsed * 1.35 + i * 1.7) * 0.20
			sprite.modulate.a = 0.68 + sin(elapsed * 1.8 + i) * 0.18
		for i in range(moving_lights.size()):
			moving_lights[i].position = Vector3(cos(elapsed * 0.8 + i * PI) * 2.8, 1.7, sin(elapsed * 0.8 + i * PI) * 2.8)
		hero.modulate = Color(1.0, 0.72 + sin(elapsed * 2.2) * 0.28, 1.0, 1.0)
	else:
		sprite_2d.rotation = sin(elapsed * 1.2) * 0.12
		sprite_2d.modulate = Color(1.0, 0.76 + sin(elapsed * 2.0) * 0.24, 1.0, 0.82 + sin(elapsed * 1.7) * 0.16)
	if Input.is_action_just_pressed("toggle_tilt_shift"):
		tilt_enabled = not tilt_enabled
		ProjectSettings.set_setting("rendering/vita/tilt_shift", tilt_enabled)
		_update_status()
	if Input.is_action_just_pressed("toggle_animation"):
		animation_enabled = not animation_enabled
		if animation_enabled:
			hero.play()
		else:
			hero.pause()
		_update_status()

func _update_mode() -> void:
	world_3d.visible = not mode_2d
	camera.current = not mode_2d
	hud_3d.visible = not mode_2d
	preview_2d.visible = mode_2d
	_update_status()

func _update_status() -> void:
	if not status_label:
		return
	status_label.text = "AnimatedSprite3D: %s   |   tilt-shift: %s   |   luzes móveis + alpha sorting" % [
		"PLAYING" if animation_enabled else "PAUSED",
		"ON" if tilt_enabled else "OFF"
	]
