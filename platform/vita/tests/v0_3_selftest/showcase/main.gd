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
var character_ground_y := -0.08
var character_animation_name := ""
var giant_character: Node3D
var giant_model: Node3D
var giant_animation: AnimationPlayer
var giant_scale := 2.2
var rolling_time := 0.0
var animation_enabled := true
var tilt_enabled := true
var elapsed := 0.0
var camera_yaw := 0.0
var camera_pitch := 0.18
var camera_distance := 8.0
var camera_target := Vector3(0.0, 0.8, 0.0)
var capture_mode := false
var capture_phase := -1
var feature_label: Label
var diagnostic_label: Label
var meter_bars: Array[ColorRect] = []
var rotating_nodes: Array[Node3D] = []
var audio_probe: AudioStreamPlayer
var stream_probe_count := 0
var stream_probe_ms := 0.0
var stream_probe_bytes := 0
var presentation_layer: CanvasLayer
var intro_panel: ColorRect
var intro_progress: ColorRect
var tour_panel: ColorRect
var tour_title: Label
var tour_description: Label
var presentation_active := true
var presentation_time := 0.0
var presentation_step := -1
var capture_control_time := 0.0
var first_person := false
var movement_smoothed := Vector3.ZERO
var jump_buffer := 0.0
var coyote_time := 0.0
var holograms: Array[Sprite3D] = []
var harbor_grid_texture: ImageTexture
var ui_font: FontFile

# --- v0.3 self test (meta "selftest" set by the runner) ---------------------
# After the guided tour the scene drives itself: a balanced A/B of PCK reads
# with the prefetch paused and active, then about a minute of movement with
# the audio loop playing, first person, the 2D card and the giant scale.
# GODOT_VITA_PCK_PREFETCH_PAUSE=1 makes the runtime skip the read-ahead
# window for the following reads (platform/vita/vita_pck_prefetch.cpp).
const SELFTEST_AB_PATTERN := [false, true, true, false, false, true, true, false, false, true, true, false]
const SELFTEST_AB_INTERVAL := 2.5
const SELFTEST_WALK_S := 45.0
signal selftest_finished(summary: Dictionary)
var selftest := false
var selftest_time := 0.0
var selftest_step := -1
var selftest_probe_index := 0
var selftest_ab: Array[Dictionary] = []
var selftest_frames := PackedFloat32Array()
var selftest_hop := 0.0
var selftest_roll := 0.0
var selftest_done := false
var tour_titles := [
	"PORTAL DE STREAMING PCK",
	"TERMINAIS DE CARGA TEXTURIZADOS",
	"NÚCLEOS DE LUZ E TRANSPARÊNCIA",
	"HOLOGRAMAS 2D NO MUNDO 3D",
	"LINUX PIXEL PLUSH ESCALÁVEL",
	"PERSONAGEM, ÁUDIO E CÂMERA"
]
var tour_descriptions := [
	"Lê 1,5 MiB em blocos de 64 KiB. Prova o FileAccessPack e alimenta a telemetria do prefetch.",
	"Geometria repetida com materiais procedurais. Prova upload de textura, UV, batching e profundidade.",
	"Luzes móveis, objetos transparentes e anéis animados. Prova ordenação alpha e atualização 3D por frame.",
	"Painéis Sprite3D recebem arte 2D dinâmica. Prova composição imersiva de canvas, textura e espaço 3D.",
	"Tux recriado no estilo Pixel Plush ganha escala de 1,6× a 3,2×. Prova geometria low-poly, animação, transformação e custo de pixels.",
	"Pixel Plush animado, WAV PCM em loop e câmera livre. Prova animação, mixer dedicado e entrada do Vita."
]

func _ready() -> void:
	capture_mode = "--capture" in OS.get_cmdline_user_args()
	selftest = has_meta("selftest")
	ui_font = load("res://showcase/Branding-Medium.otf") as FontFile
	_build_environment()
	_build_camera()
	_build_world()
	_build_character()
	_build_giant_character()
	_build_sprite_showcase()
	_build_audio_probe()
	_build_3d_hud()
	_build_2d_preview()
	_build_presentation_ui()
	_run_stream_probe()
	_update_mode()

func _exit_tree() -> void:
	if audio_probe:
		audio_probe.stop()
		audio_probe.stream = null
		audio_probe.free()
		audio_probe = null

func _build_environment() -> void:
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color("07152d")
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("88b8db")
	environment.ambient_light_energy = 0.72
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

func _make_grid_texture(primary: Color, accent: Color) -> ImageTexture:
	var image := Image.create(128, 128, false, Image.FORMAT_RGBA8)
	image.fill(primary)
	for y in range(128):
		for x in range(128):
			var grid := x % 32 < 3 or y % 32 < 3
			var stripe := (x + y) % 64 < 5
			if grid:
				image.set_pixel(x, y, accent)
			elif stripe:
				image.set_pixel(x, y, primary.lightened(0.12))
	return ImageTexture.create_from_image(image)

func _textured_box(position: Vector3, size: Vector3, texture: Texture2D, tint: Color = Color.WHITE) -> MeshInstance3D:
	var mesh := BoxMesh.new()
	mesh.size = size
	var material := StandardMaterial3D.new()
	material.albedo_texture = texture
	material.albedo_color = tint
	material.roughness = 0.68
	mesh.material = material
	var node := MeshInstance3D.new()
	node.mesh = mesh
	node.position = position
	world_3d.add_child(node)
	return node

func _build_world() -> void:
	world_3d = Node3D.new()
	world_3d.name = "PCKNeonHarbor"
	add_child(world_3d)
	harbor_grid_texture = _make_grid_texture(Color("173653"), Color("35e0d0"))
	# Central dock, two loading wings and side observation platforms.
	_box(Vector3(0.0, -0.42, 0.0), Vector3(18.0, 0.24, 18.0), Color("081a31"))
	_textured_box(Vector3(0.0, -0.18, 0.1), Vector3(4.4, 0.18, 15.6), harbor_grid_texture, Color("d5edf4"))
	_textured_box(Vector3(-5.35, 0.08, -0.2), Vector3(3.4, 0.52, 15.0), harbor_grid_texture, Color("5478a0"))
	_textured_box(Vector3(5.35, 0.08, -0.2), Vector3(3.4, 0.52, 15.0), harbor_grid_texture, Color("5478a0"))
	_box(Vector3(-7.7, 0.34, 3.8), Vector3(1.3, 0.78, 5.6), Color("263f72"))
	_box(Vector3(7.7, 0.34, -3.8), Vector3(1.3, 0.78, 5.6), Color("263f72"))
	for z in range(-7, 8):
		var lane_color := Color("35e0d0") if z % 2 == 0 else Color("ffcf4a")
		_box(Vector3(0.0, -0.045, float(z)), Vector3(0.14, 0.035, 0.55), lane_color)
		for side in [-1.0, 1.0]:
			var cargo_color := Color("ff557f") if (z + int(side)) % 3 == 0 else Color("4f7dff")
			var cargo := _textured_box(Vector3(side * 5.35, 0.55, float(z)), Vector3(1.55, 0.72, 0.68), harbor_grid_texture, cargo_color)
			cargo.rotation_degrees.y = float(z % 2) * 4.0
	# A three-ring PCK gateway at the far end of the dock.
	for i in range(3):
		var ring := TorusMesh.new()
		ring.inner_radius = 0.72 + i * 0.18
		ring.outer_radius = 0.92 + i * 0.18
		var ring_node := _mesh_node(ring, Vector3(0.0, 1.25, -6.6), Color("35e0d0") if i % 2 == 0 else Color("ffcf4a"))
		ring_node.rotation_degrees.x = 90.0
		rotating_nodes.append(ring_node)
	# Scanner pylons and floating data cores.
	for side in [-1.0, 1.0]:
		_box(Vector3(side * 1.65, 0.75, -5.6), Vector3(0.32, 2.0, 0.42), Color("9b6dff"))
		_box(Vector3(side * 1.65, 1.82, -5.6), Vector3(0.68, 0.16, 0.68), Color("35e0d0"))
		var core := SphereMesh.new()
		core.radius = 0.34
		core.height = 0.68
		var core_node := _mesh_node(core, Vector3(side * 3.0, 1.15, -1.8), Color("ffcf4a"))
		rotating_nodes.append(core_node)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-48.0, -34.0, 0.0)
	sun.light_color = Color("bde8ff")
	sun.light_energy = 1.35
	world_3d.add_child(sun)
	for data in [[Color("ff4d8d"), 0.0], [Color("35e0d0"), PI]]:
		var light := OmniLight3D.new()
		light.light_color = data[0]
		light.light_energy = 3.2
		light.omni_range = 5.2
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
	var plush_scene := load("res://showcase/models/pixel_plush/godot_pixel_plush.gltf") as PackedScene
	if not plush_scene:
		push_error("Não foi possível carregar o personagem Pixel Plush")
		return
	character_model = plush_scene.instantiate()
	character_model.scale = Vector3.ONE * 0.72
	character_model.rotation_degrees.y = 180.0
	character.add_child(character_model)
	character_animation = _find_animation_player(character_model)
	_play_character_animation("idle")

func _build_giant_character() -> void:
	giant_character = Node3D.new()
	giant_character.name = "LinuxPixelPlushPerformanceProbe"
	giant_character.position = Vector3(0.0, character_ground_y, -8.0)
	world_3d.add_child(giant_character)
	var plush_scene := load("res://showcase/models/linux_pixel_plush/linux_pixel_plush.tscn") as PackedScene
	if not plush_scene:
		return
	giant_model = plush_scene.instantiate()
	giant_model.scale = Vector3.ONE * giant_scale
	giant_model.rotation_degrees.y = 0.0
	giant_character.add_child(giant_model)
	giant_animation = _find_animation_player(giant_model)

func _change_giant_scale(amount: float) -> void:
	giant_scale = clampf(giant_scale + amount, 1.6, 3.2)
	if giant_model:
		giant_model.scale = Vector3.ONE * giant_scale
	feature_label.text = "LINUX PIXEL PLUSH: %.1fx  •  ↑/↓: ESCALA  •  □: CÂMERA  •  △: PCK" % giant_scale

func _play_character_animation(name: String) -> void:
	if not character_animation or character_animation_name == name:
		return
	if character_animation.has_animation(name):
		character_animation.play(name, 0.15)
		character_animation_name = name

func _build_audio_probe() -> void:
	# A bundled PCM chord exercises import, PCK reads and AudioStreamWAV mixing.
	var wave := load("res://showcase/audio_probe.wav") as AudioStreamWAV
	if not wave:
		push_error("Audio probe WAV could not be loaded")
		return
	wave.loop_mode = AudioStreamWAV.LOOP_FORWARD
	wave.loop_begin = 0
	wave.loop_end = int(wave.get_length() * wave.mix_rate)
	audio_probe = AudioStreamPlayer.new()
	audio_probe.name = "DedicatedMixerProbe"
	audio_probe.stream = wave
	audio_probe.volume_db = -16.0
	add_child(audio_probe)
	audio_probe.play()

func _run_stream_probe() -> void:
	var started := Time.get_ticks_usec()
	# The deterministic 1.5 MiB blob is embedded in game.pck. Reading it in
	# 64 KiB chunks produces consecutive FileAccessPack reads large enough to
	# exercise the v0.3 read-ahead window on Vita.
	var probe := FileAccess.open("res://showcase/pck_probe.bin", FileAccess.READ)
	stream_probe_bytes = 0
	if probe:
		while not probe.eof_reached():
			var chunk := probe.get_buffer(64 * 1024)
			stream_probe_bytes += chunk.size()
		probe.close()
	stream_probe_ms = float(Time.get_ticks_usec() - started) / 1000.0
	if stream_probe_bytes > 0:
		stream_probe_count += 1
	print("[showcase-v03] pck_probe=%d bytes=%d load_ms=%.3f host=%s" % [stream_probe_count, stream_probe_bytes, stream_probe_ms, OS.get_name()])
	_update_status()

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

func _make_hologram_texture(side: int) -> ImageTexture:
	var image := Image.create(256, 144, false, Image.FORMAT_RGBA8)
	image.fill(Color(0.01, 0.05, 0.12, 0.82))
	for y in range(8, 136):
		for x in range(8, 248):
			if x % 32 < 2 or y % 24 < 2:
				image.set_pixel(x, y, Color(0.15, 0.85, 0.92, 0.32))
	for i in range(7):
		var height := 18 + ((i * 29 + side * 17) % 78)
		for y in range(126 - height, 126):
			for x in range(22 + i * 30, 42 + i * 30):
				image.set_pixel(x, y, Color("35e0d0") if i % 2 == 0 else Color("ffcf4a"))
	for x in range(20, 236):
		var wave_y := 45 + int(sin(float(x) * 0.08 + side) * 18.0)
		for offset in range(-2, 3):
			image.set_pixel(x, wave_y + offset, Color("ff557f"))
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
	for side in [-1, 1]:
		var hologram := Sprite3D.new()
		hologram.texture = _make_hologram_texture(side)
		hologram.pixel_size = 0.009
		hologram.position = Vector3(float(side) * 2.8, 1.65, -3.4)
		hologram.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		hologram.modulate = Color(0.78, 1.0, 1.0, 0.9)
		world_3d.add_child(hologram)
		holograms.append(hologram)

func _label(parent: Node, text: String, position: Vector2, size: int) -> Label:
	var label := Label.new()
	label.text = text
	label.position = position
	if ui_font:
		label.add_theme_font_override("font", ui_font)
	label.add_theme_font_size_override("font_size", size)
	label.add_theme_color_override("font_shadow_color", Color(0.0, 0.0, 0.0, 0.88))
	label.add_theme_constant_override("shadow_offset_x", 3)
	label.add_theme_constant_override("shadow_offset_y", 3)
	label.add_theme_constant_override("shadow_outline_size", 2)
	parent.add_child(label)
	return label

func _build_input_bar(parent: Node, two_dimensional: bool = false) -> void:
	var bar := ColorRect.new()
	bar.position = Vector2(18, 472)
	bar.size = Vector2(924, 58)
	bar.color = Color(0.015, 0.03, 0.07, 0.9)
	parent.add_child(bar)
	var items := []
	if two_dimensional:
		items = [["select", ""], ["start", ""], ["triangle", "TESTAR PCK"]]
	else:
		items = [["analog_l", "MOVER"], ["analog_r", "CÂM."], ["cross", "PULAR"], ["circle", "ROLAR"], ["square", "1ª P."], ["triangle", "PCK"], ["up", "TAM+"], ["down", "TAM-"], ["select", ""]]
	var item_width := 280.0 if two_dimensional else 100.0
	var start_x := 34.0 if two_dimensional else 8.0
	for i in range(items.size()):
		var icon := TextureRect.new()
		icon.texture = load("res://showcase/glyphs/%s.png" % items[i][0])
		icon.position = Vector2(start_x + i * item_width, 14)
		icon.size = Vector2(30, 30)
		icon.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		icon.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		bar.add_child(icon)
		var text_label := _label(bar, items[i][1], Vector2(start_x + 34 + i * item_width, 17), 13 if not two_dimensional else 16)
		text_label.add_theme_color_override("font_color", Color("e8f7ff"))

func _build_3d_hud() -> void:
	hud_3d = CanvasLayer.new()
	add_child(hud_3d)
	var panel := ColorRect.new()
	panel.position = Vector2(18, 16)
	panel.size = Vector2(720, 126)
	panel.color = Color(0.025, 0.055, 0.12, 0.86)
	hud_3d.add_child(panel)
	_label(hud_3d, "GODOT 4 VITA v0.3  |  NEON PCK HARBOR", Vector2(36, 25), 25)
	status_label = _label(hud_3d, "", Vector2(37, 60), 16)
	feature_label = _label(hud_3d, "PREFETCH: CONFIGURADO  •  WAV: TOCANDO  •  CPU3: RESERVADA PARA I/O", Vector2(37, 89), 15)
	_build_input_bar(hud_3d)

func _build_presentation_ui() -> void:
	presentation_layer = CanvasLayer.new()
	presentation_layer.layer = 20
	add_child(presentation_layer)
	intro_panel = ColorRect.new()
	intro_panel.size = Vector2(960, 544)
	intro_panel.color = Color("040b19")
	presentation_layer.add_child(intro_panel)
	var logo := TextureRect.new()
	logo.texture = load("res://showcase/godot4vita_logo.png")
	logo.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	logo.stretch_mode = TextureRect.STRETCH_SCALE
	intro_panel.add_child(logo)
	logo.position = Vector2(170, 65)
	logo.size = Vector2(620, 220)
	var intro_title := _label(intro_panel, "GODOT 4 VITA", Vector2(230, 292), 42)
	intro_title.size = Vector2(500, 54)
	intro_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	var intro_subtitle := _label(intro_panel, "v0.3 DEVELOPMENT SHOWCASE", Vector2(230, 347), 21)
	intro_subtitle.size = Vector2(500, 32)
	intro_subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	var loading_track := ColorRect.new()
	loading_track.position = Vector2(280, 410)
	loading_track.size = Vector2(400, 10)
	loading_track.color = Color("182f4d")
	intro_panel.add_child(loading_track)
	intro_progress = ColorRect.new()
	intro_progress.position = Vector2(280, 410)
	intro_progress.size = Vector2(0, 10)
	intro_progress.color = Color("35e0d0")
	intro_panel.add_child(intro_progress)
	var loading_label := _label(intro_panel, "CARREGANDO NEON PCK HARBOR", Vector2(280, 435), 16)
	loading_label.size = Vector2(400, 28)
	loading_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	var start_glyph := TextureRect.new()
	start_glyph.texture = load("res://showcase/glyphs/start.png")
	start_glyph.position = Vector2(366, 496)
	start_glyph.size = Vector2(48, 24)
	start_glyph.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	start_glyph.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	intro_panel.add_child(start_glyph)
	_label(intro_panel, "PULAR APRESENTAÇÃO", Vector2(425, 498), 14)
	tour_panel = ColorRect.new()
	tour_panel.position = Vector2(34, 354)
	tour_panel.size = Vector2(892, 144)
	tour_panel.color = Color(0.015, 0.035, 0.08, 0.92)
	tour_panel.visible = false
	presentation_layer.add_child(tour_panel)
	tour_title = _label(tour_panel, "", Vector2(22, 16), 24)
	tour_description = _label(tour_panel, "", Vector2(22, 55), 17)
	tour_description.size = Vector2(840, 70)
	tour_description.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	hud_3d.visible = false

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
	background.color = Color("091b3a")
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
	ground.color = Color("0c2940")
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
	_label(preview_2d, "GODOT4VITA v0.3 • DIAGNÓSTICO AO VIVO", Vector2(28, 22), 30)
	_label(preview_2d, "Modelo e textura: recarga real • WAV PCM em loop • canvas e transparência", Vector2(30, 65), 18)
	_label(preview_2d, "ASSET PROBE", Vector2(117, 170), 20)
	diagnostic_label = _label(preview_2d, "", Vector2(555, 154), 16)
	diagnostic_label.add_theme_color_override("font_color", Color.WHITE)
	for i in range(8):
		var bar := ColorRect.new()
		bar.position = Vector2(420 + i * 42, 415)
		bar.size = Vector2(25, 20 + (i % 4) * 14)
		bar.color = Color("5ce1e6") if i % 2 == 0 else Color("ffd166")
		preview_2d.add_child(bar)
		meter_bars.append(bar)
	_build_input_bar(preview_2d, true)

func _process_presentation(delta: float) -> void:
	presentation_time += delta
	intro_progress.size.x = minf(400.0, presentation_time / 3.2 * 400.0)
	if presentation_time < 3.2:
		intro_panel.visible = true
		tour_panel.visible = false
		return
	intro_panel.visible = false
	var step := int((presentation_time - 3.2) / 3.0)
	if step >= tour_titles.size():
		_finish_presentation()
		return
	if step != presentation_step:
		presentation_step = step
		tour_title.text = "%02d  •  %s" % [step + 1, tour_titles[step]]
		tour_description.text = tour_descriptions[step]
		tour_panel.visible = true
		if step == 0:
			_run_stream_probe()
	var positions := [
		Vector3(0.0, 3.4, 4.8),
		Vector3(-8.2, 3.8, 2.6),
		Vector3(7.2, 3.0, 1.8),
		Vector3(-5.0, 2.7, -0.4),
		Vector3(5.8, 4.0, -3.4),
		Vector3(0.0, 2.5, 5.4)
	]
	var targets := [
		Vector3(0.0, 1.25, -6.6),
		Vector3(-5.35, 0.6, -1.0),
		Vector3(3.0, 1.15, -1.8),
		Vector3(-2.8, 1.65, -3.4),
		Vector3(0.0, 2.2, -8.0),
		character.position + Vector3(0.0, 0.9, 0.0)
	]
	camera.position = camera.position.lerp(positions[step], minf(delta * 2.4, 1.0))
	camera.look_at(targets[step], Vector3.UP)
	_play_character_animation("idle")

func _finish_presentation() -> void:
	if not presentation_active:
		return
	presentation_active = false
	intro_panel.visible = false
	tour_panel.visible = false
	hud_3d.visible = true
	camera_yaw = 0.0
	camera_pitch = 0.18
	camera_distance = 7.5
	camera_target = character.position + Vector3(0.0, 0.9, 0.0)
	_update_camera()
	feature_label.text = "CONTROLE LIBERADO  •  □: 1ª PESSOA  •  ↑/↓: GIGANTE  •  △: PCK"

func _toggle_first_person() -> void:
	first_person = not first_person
	if character_model:
		character_model.visible = not first_person
	feature_label.text = ("1ª PESSOA ATIVA" if first_person else "3ª PESSOA ATIVA") + "  •  □: ALTERNAR  •  △: TESTE PCK"
	_update_camera()

func _deadzone(value: float) -> float:
	return 0.0 if absf(value) < 0.16 else value

func _update_camera() -> void:
	var direction := Vector3(cos(camera_pitch) * sin(camera_yaw), sin(camera_pitch), cos(camera_pitch) * cos(camera_yaw))
	if first_person and character:
		var head := character.position + Vector3(0.0, 1.15, 0.0)
		camera.position = head
		camera.look_at(head - direction * 10.0, Vector3.UP)
	else:
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
	jump_buffer = maxf(0.0, jump_buffer - delta)
	coyote_time = maxf(0.0, coyote_time - delta)
	if Input.is_action_just_pressed("jump"):
		jump_buffer = 0.14
	if grounded:
		character.position.y = character_ground_y
		character_velocity.y = 0.0
		coyote_time = 0.12
	if jump_buffer > 0.0 and coyote_time > 0.0:
		character_velocity.y = 5.7
		jump_buffer = 0.0
		coyote_time = 0.0
		grounded = false
	if grounded and Input.is_action_just_pressed("roll"):
		rolling_time = 0.58
	rolling_time = maxf(0.0, rolling_time - delta)
	var speed := 5.4 if rolling_time > 0.0 else 3.6
	var desired := movement * speed
	var acceleration := 18.0 if movement.length_squared() > 0.01 else 24.0
	movement_smoothed.x = move_toward(movement_smoothed.x, desired.x, acceleration * delta)
	movement_smoothed.z = move_toward(movement_smoothed.z, desired.z, acceleration * delta)
	character_velocity.x = movement_smoothed.x
	character_velocity.z = movement_smoothed.z
	character_velocity.y -= 14.5 * delta
	character.position += character_velocity * delta
	character.position.x = clampf(character.position.x, -8.1, 8.1)
	character.position.z = clampf(character.position.z, -8.1, 8.1)
	if character.position.y < character_ground_y:
		character.position.y = character_ground_y
		character_velocity.y = 0.0
		grounded = true
	if movement_smoothed.length_squared() > 0.025 and not first_person:
		character.rotation.y = lerp_angle(character.rotation.y, atan2(movement_smoothed.x, movement_smoothed.z), minf(delta * 11.0, 1.0))
	if rolling_time > 0.0:
		_play_character_animation("rolling")
	elif not grounded:
		_play_character_animation("jump" if character_velocity.y > 0.0 else "falling")
	elif movement_smoothed.length_squared() > 0.025:
		_play_character_animation("running")
	else:
		_play_character_animation("idle")

func _process(delta: float) -> void:
	elapsed += delta
	if selftest and not presentation_active:
		_process_selftest(delta)
	elif selftest:
		_process_presentation(delta)
	elif presentation_active:
		if Input.is_action_just_pressed("switch_to_3d"):
			_finish_presentation()
		else:
			_process_presentation(delta)
	else:
		if Input.is_action_just_pressed("switch_to_2d"):
			mode_2d = true
			_update_mode()
		if Input.is_action_just_pressed("switch_to_3d"):
			mode_2d = false
			_update_mode()
		if Input.is_action_just_pressed("toggle_first_person") and not mode_2d:
			_toggle_first_person()
		if Input.is_action_just_pressed("toggle_animation"):
			_run_stream_probe()
		if Input.is_action_just_pressed("giant_grow"):
			_change_giant_scale(0.2)
		if Input.is_action_just_pressed("giant_shrink"):
			_change_giant_scale(-0.2)
		if capture_mode:
			_process_capture_demo(delta)
		if not mode_2d:
			if not capture_mode:
				_process_character(delta)
			_process_camera(delta)
	if selftest_done:
		return
	if not mode_2d:
		for i in range(glass_sprites.size()):
			var sprite := glass_sprites[i]
			sprite.position.y = 0.78 + sin(elapsed * 1.35 + i * 1.7) * 0.20
			sprite.modulate.a = 0.68 + sin(elapsed * 1.8 + i) * 0.18
		for i in range(moving_lights.size()):
			moving_lights[i].position = Vector3(cos(elapsed * 0.8 + i * PI) * 2.8, 1.7, sin(elapsed * 0.8 + i * PI) * 2.8)
		for i in range(rotating_nodes.size()):
			rotating_nodes[i].rotation.y += delta * (0.32 + i * 0.08)
			rotating_nodes[i].position.y += sin(elapsed * 1.4 + i) * delta * 0.035
		for i in range(holograms.size()):
			holograms[i].modulate.a = 0.72 + sin(elapsed * 2.0 + i) * 0.18
		hero.modulate = Color(1.0, 0.72 + sin(elapsed * 2.2) * 0.28, 1.0, 1.0)
	else:
		sprite_2d.rotation = sin(elapsed * 1.2) * 0.12
		sprite_2d.modulate = Color(1.0, 0.76 + sin(elapsed * 2.0) * 0.24, 1.0, 0.82 + sin(elapsed * 1.7) * 0.16)
		for i in range(meter_bars.size()):
			meter_bars[i].size.y = 22.0 + (sin(elapsed * 3.2 + i * 0.8) * 0.5 + 0.5) * 54.0
			meter_bars[i].position.y = 478.0 - meter_bars[i].size.y

func _process_capture_demo(delta: float) -> void:
	capture_control_time += delta
	var phase := 0 if capture_control_time < 2.5 else (1 if capture_control_time < 5.0 else (2 if capture_control_time < 7.0 else 3))
	if phase != capture_phase:
		capture_phase = phase
		if phase == 1 and not first_person:
			_toggle_first_person()
		if phase == 2 and first_person:
			_toggle_first_person()
		mode_2d = phase == 2
		_update_mode()
		if phase == 0 or phase == 3:
			_run_stream_probe()
		if phase == 3:
			_change_giant_scale(0.6)
	if phase == 0:
		character.position.z = maxf(-4.8, character.position.z - delta * 1.15)
		_play_character_animation("running")
		camera_distance = minf(10.5, camera_distance + delta * 0.65)
	elif phase == 1:
		character.position.z = maxf(-6.0, character.position.z - delta * 0.8)
		_play_character_animation("running")
		camera_yaw -= delta * 0.18
	elif phase == 3:
		_play_character_animation("idle")
		camera_yaw += delta * 0.22
		camera_distance = minf(12.0, camera_distance + delta * 0.5)
	if capture_control_time >= 8.2 and audio_probe and audio_probe.playing:
		audio_probe.stop()
		audio_probe.stream = null
	if capture_control_time >= 8.4:
		get_tree().quit()

func _update_mode() -> void:
	world_3d.visible = not mode_2d
	camera.current = not mode_2d
	hud_3d.visible = not mode_2d and not presentation_active
	preview_2d.visible = mode_2d
	_update_status()

func _update_status() -> void:
	if not status_label:
		return
	status_label.text = "TESTE PCK #%d: %.2f ms   |   ÁUDIO: %s   |   GXM 3D + CANVAS" % [
		stream_probe_count,
		stream_probe_ms,
		"PLAYING" if audio_probe and audio_probe.playing else "STOPPED"
	]
	if diagnostic_label:
		diagnostic_label.text = "TESTE REAL DO GAME.PCK\nLeituras concluídas: %d\nBytes lidos: %d\nÚltima carga: %.2f ms\n\nPrefetch: configurado\nÁudio PCM: %s" % [stream_probe_count, stream_probe_bytes, stream_probe_ms, "tocando" if audio_probe and audio_probe.playing else "parado"]

func _selftest_probe(prefetch: bool) -> void:
	OS.set_environment("GODOT_VITA_PCK_PREFETCH_PAUSE", "0" if prefetch else "1")
	_run_stream_probe()
	OS.set_environment("GODOT_VITA_PCK_PREFETCH_PAUSE", "0")
	selftest_ab.append({"prefetch": prefetch, "bytes": stream_probe_bytes, "load_ms": stream_probe_ms})
	print("[selftest] pck_ab index=%d prefetch=%s bytes=%d load_ms=%.3f" % [selftest_ab.size(), "on" if prefetch else "off", stream_probe_bytes, stream_probe_ms])
	feature_label.text = "TESTE AUTOMÁTICO  •  PCK %d/%d  •  PREFETCH %s  •  %.2f ms" % [selftest_ab.size(), SELFTEST_AB_PATTERN.size(), "LIGADO" if prefetch else "PAUSADO", stream_probe_ms]

func _selftest_median(prefetch: bool) -> float:
	var values: Array[float] = []
	for probe in selftest_ab:
		if probe.prefetch == prefetch:
			values.append(probe.load_ms)
	if values.is_empty():
		return 0.0
	values.sort()
	return values[values.size() / 2] if values.size() % 2 else (values[values.size() / 2 - 1] + values[values.size() / 2]) * 0.5

func _process_selftest(delta: float) -> void:
	if selftest_done:
		return
	selftest_time += delta
	var ab_end := SELFTEST_AB_PATTERN.size() * SELFTEST_AB_INTERVAL + 1.0
	var walk_end := ab_end + SELFTEST_WALK_S
	var fp_end := walk_end + 6.0
	var card_end := fp_end + 8.0
	var giant_end := card_end + 8.0
	var step := 0 if selftest_time < ab_end else (1 if selftest_time < walk_end else (2 if selftest_time < fp_end else (3 if selftest_time < card_end else (4 if selftest_time < giant_end else 5))))
	if step != selftest_step:
		selftest_step = step
		printerr("[selftest] showcase step=%d t=%.1f" % [step, selftest_time])
		match step:
			1:
				feature_label.text = "TESTE AUTOMÁTICO  •  ANDANDO COM ÁUDIO (%d s)  •  OUÇA SE HÁ ESTALOS" % int(SELFTEST_WALK_S)
			2:
				_toggle_first_person()
			3:
				if first_person:
					_toggle_first_person()
				mode_2d = true
				_update_mode()
			4:
				mode_2d = false
				_update_mode()
				# Back to the spawn point, facing the giant, so it stays in frame.
				character.position = Vector3(0.0, character_ground_y, 1.7)
				character.rotation.y = PI
				camera_yaw = 0.0
				camera_pitch = 0.22
				camera_distance = 8.5
			5:
				_selftest_finish()
				return
	if not mode_2d:
		selftest_frames.append(delta * 1000.0)
	match step:
		0:
			_play_character_animation("idle")
			if selftest_probe_index < SELFTEST_AB_PATTERN.size() and selftest_time >= 1.0 + selftest_probe_index * SELFTEST_AB_INTERVAL:
				_selftest_probe(SELFTEST_AB_PATTERN[selftest_probe_index])
				selftest_probe_index += 1
			camera_yaw += delta * 0.12
		1, 2:
			var t := selftest_time - ab_end
			var angle := t * 0.55
			var target := Vector3(cos(angle) * 4.2, character_ground_y, sin(angle) * 4.2)
			var move := target - Vector3(character.position.x, character_ground_y, character.position.z)
			character.position.x = target.x
			character.position.z = target.z
			if move.length_squared() > 0.000001 and not first_person:
				character.rotation.y = atan2(move.x, move.z)
			selftest_hop = maxf(0.0, selftest_hop - delta)
			selftest_roll = maxf(0.0, selftest_roll - delta)
			if step == 1 and fmod(t, 6.0) < delta:
				selftest_hop = 0.8
			if step == 1 and fmod(t + 3.0, 9.0) < delta and selftest_hop <= 0.0:
				selftest_roll = 0.58
			if selftest_hop > 0.0:
				var p := 1.0 - selftest_hop / 0.8
				character.position.y = character_ground_y + 4.8 * p * (1.0 - p)
				_play_character_animation("jump" if p < 0.5 else "falling")
			else:
				character.position.y = character_ground_y
				_play_character_animation("rolling" if selftest_roll > 0.0 else "running")
			if step == 2:
				camera_yaw = -angle - PI * 0.5
			else:
				camera_yaw += delta * 0.18
		4:
			_play_character_animation("idle")
			if fmod(selftest_time - card_end, 2.0) < delta:
				_change_giant_scale(0.4)
			camera_yaw = sin((selftest_time - card_end) * 0.4) * 0.6
			camera_distance = minf(12.0, camera_distance + delta * 0.4)
	if not mode_2d and character:
		camera_target = camera_target.lerp(character.position + Vector3(0.0, 0.9, 0.0), minf(delta * 7.0, 1.0))
		_update_camera()

func _selftest_finish() -> void:
	selftest_done = true
	var sorted := selftest_frames.duplicate()
	sorted.sort()
	var total := 0.0
	for value in sorted:
		total += value
	var avg := total / maxi(sorted.size(), 1)
	var summary := {
		"probes": selftest_ab,
		"median_on_ms": _selftest_median(true),
		"median_off_ms": _selftest_median(false),
		"frames": sorted.size(),
		"avg_ms": avg,
		"p95_ms": sorted[clampi(int(ceil(0.95 * sorted.size())) - 1, 0, sorted.size() - 1)] if sorted.size() else 0.0,
		"max_ms": sorted[sorted.size() - 1] if sorted.size() else 0.0,
		"audio_playing": audio_probe != null and audio_probe.playing,
	}
	printerr("[selftest] showcase frames=%d avg_ms=%.3f p95_ms=%.3f max_ms=%.3f audio_playing=%s" % [summary.frames, summary.avg_ms, summary.p95_ms, summary.max_ms, summary.audio_playing])
	if audio_probe:
		audio_probe.stop()
	selftest_finished.emit(summary)
