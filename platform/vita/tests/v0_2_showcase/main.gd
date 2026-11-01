extends Node3D

var hero: AnimatedSprite3D
var glass_sprites: Array[Sprite3D] = []
var animation_enabled := true
var tilt_enabled := true
var elapsed := 0.0
var status_label: Label

func _ready() -> void:
	_build_camera()
	_build_world()
	_build_sprite_showcase()
	_build_ui()
	_update_status()

func _build_camera() -> void:
	var camera := Camera3D.new()
	camera.position = Vector3(0.0, 2.8, 8.0)
	camera.look_at_from_position(camera.position, Vector3(0.0, 0.7, 0.0), Vector3.UP)
	add_child(camera)

func _material(color: Color) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	return material

func _box(position: Vector3, size: Vector3, color: Color) -> void:
	var node := MeshInstance3D.new()
	var mesh := BoxMesh.new()
	mesh.size = size
	mesh.material = _material(color)
	node.mesh = mesh
	node.position = position
	add_child(node)

func _build_world() -> void:
	_box(Vector3(0.0, -0.35, 0.0), Vector3(10.0, 0.25, 10.0), Color("263552"))
	for z in range(-3, 4):
		var shade := 0.18 + float(z + 3) * 0.035
		_box(Vector3(-3.8, 0.35, float(z)), Vector3(0.55, 1.35, 0.55), Color(shade, 0.32, 0.52))
		_box(Vector3(3.8, 0.35, float(z)), Vector3(0.55, 1.35, 0.55), Color(0.48, shade, 0.32))
	for z in range(-3, 4):
		_box(Vector3(0.0, -0.18, float(z)), Vector3(0.08, 0.03, 0.65), Color("66d9ef"))

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
	hero.pixel_size = 0.018
	hero.position = Vector3(0.0, 0.65, 0.0)
	hero.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	hero.play()
	add_child(hero)

	var glass_colors := [Color("ff4d8d"), Color("56d6ff"), Color("ffd166")]
	var positions := [Vector3(-1.25, 0.7, 1.0), Vector3(0.8, 0.8, 0.35), Vector3(1.35, 0.75, -0.8)]
	for i in range(3):
		var sprite := Sprite3D.new()
		sprite.texture = _make_glass_texture(glass_colors[i])
		sprite.pixel_size = 0.022
		sprite.position = positions[i]
		sprite.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		sprite.modulate = Color(glass_colors[i], 0.88)
		glass_sprites.append(sprite)
		add_child(sprite)

func _build_ui() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	var panel := ColorRect.new()
	panel.position = Vector2(20, 18)
	panel.size = Vector2(560, 94)
	panel.color = Color(0.025, 0.035, 0.075, 0.82)
	layer.add_child(panel)
	var title := Label.new()
	title.position = Vector2(38, 28)
	title.text = "GODOT 4 VITA v0.2  |  GXM3D SHOWCASE"
	title.add_theme_font_size_override("font_size", 24)
	layer.add_child(title)
	status_label = Label.new()
	status_label.position = Vector2(39, 66)
	status_label.add_theme_font_size_override("font_size", 17)
	layer.add_child(status_label)
	var help := Label.new()
	help.position = Vector2(22, 500)
	help.text = "X: tilt-shift   O: animation   |   transparent sprites are depth-sorted"
	help.add_theme_font_size_override("font_size", 16)
	layer.add_child(help)

func _process(delta: float) -> void:
	elapsed += delta
	for i in range(glass_sprites.size()):
		var sprite := glass_sprites[i]
		sprite.position.y = 0.72 + sin(elapsed * 1.35 + i * 1.7) * 0.18
		sprite.modulate.a = 0.68 + sin(elapsed * 1.8 + i) * 0.18
	hero.modulate = Color(1.0, 0.72 + sin(elapsed * 2.2) * 0.28, 1.0, 1.0)
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

func _update_status() -> void:
	status_label.text = "AnimatedSprite3D: %s   |   tilt-shift: %s   |   tint + alpha sorting: ON" % [
		"PLAYING" if animation_enabled else "PAUSED",
		"ON" if tilt_enabled else "OFF"
	]
