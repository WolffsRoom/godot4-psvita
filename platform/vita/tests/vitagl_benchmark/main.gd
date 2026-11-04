extends Node
## Renderer benchmark: the same scenes run in the GXM (libvita2d) and the
## VitaGL VPKs, which differ only in eboot.bin. Each phase builds a scene,
## warms up, then records the time between frames. Results go to stderr as
## "[bench] RESULT ..." lines and to user://benchmark_<backend>.txt.
## Everything is procedural: no imported textures, meshes or fonts.
##
## Pad: Cross restarts the run once it is finished, START quits.

const WARMUP_S := 2.0
const MEASURE_S := 10.0
const SPRITES := 1000
const TEXTURE_SIZE := 32

var quick := false # Host smoke test: --quick after "--" shortens phases.
var backend := "gxm"
var phases: Array[Dictionary] = []
var phase_index := -1
var phase_time := 0.0
var last_ticks := 0
var samples := PackedFloat32Array()
var results: Array[Dictionary] = []
var stage: Node
var hud: Label
var updater := Callable()
var textures: Array[ImageTexture] = []
var rng := RandomNumberGenerator.new()
var finished := false

func _ready() -> void:
	quick = OS.get_cmdline_user_args().has("--quick")
	if FileAccess.file_exists("app0:vitagl.cfg") or FileAccess.file_exists("app0:/vitagl.cfg"):
		backend = "vitagl"
	elif OS.get_name() != "PS Vita" and not FileAccess.file_exists("app0:gxm2d.cfg"):
		backend = "host"
	for i in 8:
		textures.append(_make_texture(i))
	phases = [
		{"name": "2d_sprites_same_texture", "build": _build_sprites.bind(1)},
		{"name": "2d_sprites_mixed_textures", "build": _build_sprites.bind(8)},
		{"name": "2d_ui_text_clip", "build": _build_ui},
		{"name": "2d_subviewport", "build": _build_subviewport},
		{"name": "3d_boxes_textured", "build": _build_boxes},
		{"name": "3d_spheres_dense", "build": _build_spheres},
		{"name": "3d_vertex_lit", "build": _build_lit},
		{"name": "3d_transparent", "build": _build_transparent},
	]
	var layer := CanvasLayer.new()
	layer.layer = 100
	add_child(layer)
	var panel := ColorRect.new()
	panel.color = Color(0, 0, 0, 0.6)
	panel.position = Vector2(8, 8)
	panel.size = Vector2(560, 64)
	layer.add_child(panel)
	hud = Label.new()
	hud.position = Vector2(16, 12)
	hud.add_theme_font_size_override("font_size", 18)
	layer.add_child(hud)
	printerr("[bench] READY backend=%s phases=%d warmup_s=%.1f measure_s=%.1f quick=%s" % [backend, phases.size(), _warmup(), _measure(), quick])
	_next_phase()

func _warmup() -> float:
	return 0.2 if quick else WARMUP_S

func _measure() -> float:
	return 0.5 if quick else MEASURE_S

func _process(delta: float) -> void:
	var now := Time.get_ticks_usec()
	var frame_ms := (now - last_ticks) / 1000.0
	last_ticks = now
	if finished:
		if Input.is_joy_button_pressed(0, JOY_BUTTON_A):
			_restart()
		elif Input.is_joy_button_pressed(0, JOY_BUTTON_START) or quick:
			get_tree().quit()
		return
	if Input.is_joy_button_pressed(0, JOY_BUTTON_START):
		get_tree().quit()
		return
	phase_time += delta
	if updater.is_valid():
		updater.call(phase_time)
	if phase_time > _warmup():
		samples.append(frame_ms)
	var fps := Engine.get_frames_per_second()
	hud.text = "%s  [%d/%d] %s\n%.0f FPS  %.2f ms" % [backend.to_upper(), phase_index + 1, phases.size(), phases[phase_index].name, fps, frame_ms]
	if phase_time >= _warmup() + _measure():
		_finish_phase()
		_next_phase()

func _next_phase() -> void:
	if stage:
		stage.queue_free()
		stage = null
	updater = Callable()
	phase_index += 1
	if phase_index >= phases.size():
		_report()
		return
	phase_time = 0.0
	samples = PackedFloat32Array()
	rng.seed = 1234 + phase_index
	stage = Node.new()
	add_child(stage)
	move_child(stage, 0) # Below the HUD layer.
	var build: Callable = phases[phase_index].build
	build.call()
	last_ticks = Time.get_ticks_usec()

func _finish_phase() -> void:
	var sorted := samples.duplicate()
	sorted.sort()
	var count := sorted.size()
	var total := 0.0
	for value in sorted:
		total += value
	var avg := total / maxi(count, 1)
	var result := {
		"phase": phases[phase_index].name,
		"frames": count,
		"avg_ms": avg,
		"p50_ms": _pct(sorted, 0.50),
		"p95_ms": _pct(sorted, 0.95),
		"p99_ms": _pct(sorted, 0.99),
		"max_ms": sorted[count - 1] if count else 0.0,
		"fps": 1000.0 / avg if avg > 0.0 else 0.0,
	}
	results.append(result)
	printerr("[bench] RESULT backend=%s phase=%s frames=%d avg_ms=%.3f p50_ms=%.3f p95_ms=%.3f p99_ms=%.3f max_ms=%.3f fps=%.1f" % [
		backend, result.phase, result.frames, result.avg_ms, result.p50_ms, result.p95_ms, result.p99_ms, result.max_ms, result.fps])

func _pct(sorted: PackedFloat32Array, q: float) -> float:
	if sorted.is_empty():
		return 0.0
	return sorted[clampi(int(ceil(q * sorted.size())) - 1, 0, sorted.size() - 1)]

func _report() -> void:
	finished = true
	var lines := PackedStringArray()
	lines.append("backend=%s" % backend)
	lines.append("phase,frames,avg_ms,p50_ms,p95_ms,p99_ms,max_ms,fps")
	var table := "%s - finished. Cross: run again  START: quit\n" % backend.to_upper()
	for r in results:
		lines.append("%s,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f" % [r.phase, r.frames, r.avg_ms, r.p50_ms, r.p95_ms, r.p99_ms, r.max_ms, r.fps])
		table += "%-28s %6.2f ms  p95 %6.2f  %5.1f FPS\n" % [r.phase, r.avg_ms, r.p95_ms, r.fps]
	var path := "user://benchmark_%s.txt" % backend
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file:
		file.store_string("\n".join(lines) + "\n")
		file.close()
	printerr("[bench] DONE backend=%s results=%s" % [backend, ProjectSettings.globalize_path(path)])
	hud.get_parent().get_child(0).size = Vector2(620, 40 + 22 * results.size())
	hud.text = table

func _restart() -> void:
	finished = false
	results.clear()
	phase_index = -1
	printerr("[bench] RESTART backend=%s" % backend)
	_next_phase()

# --- Assets -----------------------------------------------------------------

func _make_texture(index: int) -> ImageTexture:
	var hue := index / 8.0
	var a := Color.from_hsv(hue, 0.7, 1.0)
	var b := Color.from_hsv(fmod(hue + 0.5, 1.0), 0.6, 0.35)
	var image := Image.create(TEXTURE_SIZE, TEXTURE_SIZE, false, Image.FORMAT_RGBA8)
	for y in TEXTURE_SIZE:
		for x in TEXTURE_SIZE:
			var dx := x - TEXTURE_SIZE / 2.0 + 0.5
			var dy := y - TEXTURE_SIZE / 2.0 + 0.5
			var inside := dx * dx + dy * dy < (TEXTURE_SIZE / 2.0) * (TEXTURE_SIZE / 2.0)
			var checker := ((x / 8) + (y / 8)) % 2 == 0
			image.set_pixel(x, y, (a if checker else b) if inside else Color(0, 0, 0, 0))
	return ImageTexture.create_from_image(image)

# --- 2D phases --------------------------------------------------------------

## Same texture: everything batches into a few draws. Mixed: the texture
## changes on every sprite, so every sprite is its own draw (per-draw cost).
func _build_sprites(texture_count: int) -> void:
	var root := Node2D.new()
	root.position = Vector2(480, 272)
	stage.add_child(root)
	for i in SPRITES:
		var sprite := Sprite2D.new()
		sprite.texture = textures[i % texture_count]
		sprite.position = Vector2(rng.randf_range(-520, 520), rng.randf_range(-320, 320))
		sprite.rotation = rng.randf() * TAU
		sprite.modulate.a = 0.9
		root.add_child(sprite)
	updater = func(t: float) -> void:
		root.rotation = t * 0.25
		root.scale = Vector2.ONE * (0.85 + 0.15 * sin(t))

func _build_ui() -> void:
	var text := "The quick brown fox jumps over the lazy dog. 0123456789\n".repeat(8)
	var labels: Array[Label] = []
	for row in 4:
		for column in 6:
			var panel := ColorRect.new()
			panel.color = Color.from_hsv((row * 6 + column) / 24.0, 0.5, 0.25)
			panel.position = Vector2(8 + column * 158, 80 + row * 114)
			panel.size = Vector2(150, 106)
			panel.clip_contents = true
			stage.add_child(panel)
			var label := Label.new()
			label.text = text
			label.add_theme_font_size_override("font_size", 12)
			panel.add_child(label)
			labels.append(label)
	updater = func(t: float) -> void:
		for i in labels.size():
			labels[i].position = Vector2(-20.0 + 20.0 * sin(t + i), -fmod(t * 30.0 + i * 7.0, 300.0))

func _build_subviewport() -> void:
	var viewport := SubViewport.new()
	viewport.size = Vector2i(480, 272)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	viewport.transparent_bg = false
	stage.add_child(viewport)
	var root := Node2D.new()
	root.position = Vector2(240, 136)
	viewport.add_child(root)
	for i in 300:
		var sprite := Sprite2D.new()
		sprite.texture = textures[i % 2]
		sprite.position = Vector2(rng.randf_range(-260, 260), rng.randf_range(-150, 150))
		root.add_child(sprite)
	var view := TextureRect.new()
	view.texture = viewport.get_texture()
	view.position = Vector2.ZERO
	view.size = Vector2(960, 544)
	view.stretch_mode = TextureRect.STRETCH_SCALE
	stage.add_child(view)
	updater = func(t: float) -> void:
		root.rotation = -t * 0.4

# --- 3D phases --------------------------------------------------------------

func _world() -> Node3D:
	var world := Node3D.new()
	stage.add_child(world)
	var camera := Camera3D.new()
	world.add_child(camera)
	camera.current = true
	camera.position = Vector3(0, 6, 14)
	camera.look_at(Vector3.ZERO)
	var orbit := func(t: float) -> void:
		camera.position = Vector3(sin(t * 0.3) * 14.0, 6.0, cos(t * 0.3) * 14.0)
		camera.look_at(Vector3.ZERO)
	updater = orbit
	return world

func _material(color: Color, texture: Texture2D = null, shaded := false, alpha := 1.0) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_PER_VERTEX if shaded else BaseMaterial3D.SHADING_MODE_UNSHADED
	material.albedo_color = Color(color, alpha)
	material.albedo_texture = texture
	if alpha < 1.0:
		material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	return material

func _grid(world: Node3D, mesh: Mesh, materials: Array, side: int, spacing: float) -> Array[MeshInstance3D]:
	var instances: Array[MeshInstance3D] = []
	for z in side:
		for x in side:
			var instance := MeshInstance3D.new()
			instance.mesh = mesh
			instance.material_override = materials[(x + z * side) % materials.size()]
			instance.position = Vector3((x - (side - 1) / 2.0) * spacing, 0, (z - (side - 1) / 2.0) * spacing)
			world.add_child(instance)
			instances.append(instance)
	return instances

func _spin(instances: Array[MeshInstance3D], orbit: Callable) -> void:
	updater = func(t: float) -> void:
		orbit.call(t)
		for i in instances.size():
			instances[i].rotation = Vector3(t * 0.5 + i * 0.1, t * 0.7, 0)

func _build_boxes() -> void:
	var world := _world()
	var box := BoxMesh.new()
	box.size = Vector3.ONE
	var materials := []
	for i in 4:
		materials.append(_material(Color.WHITE, textures[i]))
	_spin(_grid(world, box, materials, 8, 1.6), updater)

func _build_spheres() -> void:
	var world := _world()
	var sphere := SphereMesh.new()
	sphere.radius = 0.4
	sphere.height = 0.8
	sphere.radial_segments = 16
	sphere.rings = 8
	var materials := []
	for i in 8:
		materials.append(_material(Color.from_hsv(i / 8.0, 0.6, 1.0)))
	_spin(_grid(world, sphere, materials, 16, 0.85), updater)

func _build_lit() -> void:
	var world := _world()
	var sun := DirectionalLight3D.new()
	sun.rotation = Vector3(-0.9, 0.6, 0)
	sun.light_color = Color(1.0, 0.95, 0.85)
	world.add_child(sun)
	var lamps: Array[OmniLight3D] = []
	for i in 2:
		var lamp := OmniLight3D.new()
		lamp.light_color = Color(0.3, 0.6, 1.0) if i == 0 else Color(1.0, 0.4, 0.2)
		lamp.omni_range = 6.0
		world.add_child(lamp)
		lamps.append(lamp)
	var box := BoxMesh.new()
	var sphere := SphereMesh.new()
	sphere.radial_segments = 16
	sphere.rings = 8
	var materials := [_material(Color(0.8, 0.8, 0.8), textures[0], true), _material(Color(0.9, 0.7, 0.5), null, true)]
	var instances := _grid(world, box, materials, 8, 1.6)
	for i in range(1, instances.size(), 2):
		instances[i].mesh = sphere
	var orbit := updater
	updater = func(t: float) -> void:
		orbit.call(t)
		# Moving lights force new vertex colours every frame.
		lamps[0].position = Vector3(sin(t) * 5.0, 1.5, cos(t) * 5.0)
		lamps[1].position = Vector3(cos(t * 0.7) * 4.0, 1.0, sin(t * 0.7) * 4.0)
		for i in instances.size():
			instances[i].rotation = Vector3(0, t * 0.5 + i * 0.1, 0)

func _build_transparent() -> void:
	var world := _world()
	var floor := MeshInstance3D.new()
	var plane := BoxMesh.new()
	plane.size = Vector3(16, 0.2, 16)
	floor.mesh = plane
	floor.material_override = _material(Color(0.25, 0.3, 0.35), textures[1])
	floor.position = Vector3(0, -1, 0)
	world.add_child(floor)
	var box := BoxMesh.new()
	var materials := []
	for i in 4:
		materials.append(_material(Color.from_hsv(i / 4.0, 0.7, 1.0), null, false, 0.45))
	_spin(_grid(world, box, materials, 8, 1.6), updater)
