extends Node
## GXM3D test scene and benchmark for the native GXM 3D renderer.
##
## Showcase (default): a Tux built from primitive meshes in a small Linux
## scene, with what to look for listed on screen:
##   - walking pixel-art penguins (Sprite3D frames) must animate, not freeze;
##   - their transparent pixels must not cut holes in Tux or the terminal
##     panel behind them (sorted transparent pass, no depth writes);
##   - the 3D counter (Label3D) must change every frame;
##   - the red flash and the fade come from Sprite3D modulate (vertex tint).
## Benchmark: timed phases, results on screen, in stderr as "[bench] RESULT"
## lines and in user://benchmark_gxm3d_tux.txt.
##
## Pad: Cross starts the benchmark (and returns from the results),
## START quits. Host: Enter / Escape. Command line after "--":
##   --bench  start the benchmark at once
##   --quick  short phases, then quit (host smoke test)

const WARMUP_S := 2.0
const MEASURE_S := 8.0
const SHEET_FRAME := 32
const SHEET_FRAMES := 4

var quick := false
var phases: Array[Dictionary] = []
var phase_index := -1
var phase_time := 0.0
var last_ticks := 0
var samples := PackedFloat32Array()
var results: Array[Dictionary] = []
var mode := "showcase" # showcase, bench, results
var stage: Node
var hud: Label
var panel: ColorRect
var updater := Callable()
var sheet: ImageTexture
var checker: ImageTexture
var hole_texture: ImageTexture
var rng := RandomNumberGenerator.new()
var frame_counter := 0
var prev_buttons := {}

func _ready() -> void:
	var args := OS.get_cmdline_user_args()
	quick = args.has("--quick")
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	sheet = _make_tux_sheet()
	checker = _make_checker()
	hole_texture = _make_hole_texture()
	phases = [
		{"name": "showcase_tux_scene", "build": _build_showcase},
		{"name": "sprite3d_static_200", "build": _build_sprites.bind(200, false, false)},
		{"name": "sprite3d_animated_200", "build": _build_sprites.bind(200, true, false)},
		{"name": "sprite3d_animated_tint_200", "build": _build_sprites.bind(200, true, true)},
		{"name": "transparent_overlap_300", "build": _build_overlap},
		{"name": "label3d_dynamic_24", "build": _build_labels},
	]
	var layer := CanvasLayer.new()
	layer.layer = 100
	add_child(layer)
	panel = ColorRect.new()
	panel.color = Color(0, 0, 0, 0.55)
	panel.position = Vector2(8, 8)
	layer.add_child(panel)
	hud = Label.new()
	hud.position = Vector2(16, 12)
	hud.add_theme_font_size_override("font_size", 16)
	layer.add_child(hud)
	printerr("[bench] READY test=gxm3d_tux phases=%d quick=%s" % [phases.size(), quick])
	if args.has("--bench") or quick:
		_start_bench()
	else:
		_show_showcase()

func _warmup() -> float:
	return 0.2 if quick else WARMUP_S

func _measure() -> float:
	return 0.5 if quick else MEASURE_S

## Edge-triggered: true only on the frame the button goes down.
func _pressed(button: JoyButton, key: Key) -> bool:
	var down := Input.is_joy_button_pressed(0, button) or Input.is_key_pressed(key)
	var was: bool = prev_buttons.get(button, false)
	prev_buttons[button] = down
	return down and not was

func _process(delta: float) -> void:
	var now := Time.get_ticks_usec()
	var frame_ms := (now - last_ticks) / 1000.0
	last_ticks = now
	frame_counter += 1
	var start := _pressed(JOY_BUTTON_START, KEY_ESCAPE)
	var cross := _pressed(JOY_BUTTON_A, KEY_ENTER)
	if start:
		get_tree().quit()
		return
	phase_time += delta
	if updater.is_valid():
		updater.call(phase_time)
	var fps := Engine.get_frames_per_second()
	match mode:
		"showcase":
			hud.text = ("GXM3D Tux test   %.0f FPS  %.2f ms\n" % [fps, frame_ms]
				+ "- pinguins andando devem animar (Sprite3D)\n"
				+ "- pixels transparentes nao podem furar o Tux nem o painel\n"
				+ "- contador 3D muda todo quadro (Label3D)\n"
				+ "- flash vermelho e fade (modulate / tint)\n"
				+ "X: benchmark   START: sair")
			panel.size = Vector2(560, 132)
			if cross:
				_start_bench()
		"bench":
			if phase_time > _warmup():
				samples.append(frame_ms)
			hud.text = "BENCHMARK [%d/%d] %s\n%.0f FPS  %.2f ms   START: sair" % [phase_index + 1, phases.size(), phases[phase_index].name, fps, frame_ms]
			panel.size = Vector2(560, 50)
			if phase_time >= _warmup() + _measure():
				_finish_phase()
				_next_phase()
		"results":
			if cross:
				_show_showcase()
			elif quick:
				get_tree().quit()

func _reset_stage() -> void:
	if stage:
		stage.queue_free()
	updater = Callable()
	phase_time = 0.0
	stage = Node.new()
	add_child(stage)
	move_child(stage, 0) # Below the HUD layer.

func _show_showcase() -> void:
	mode = "showcase"
	_reset_stage()
	rng.seed = 42
	_build_showcase()
	last_ticks = Time.get_ticks_usec()

func _start_bench() -> void:
	mode = "bench"
	results.clear()
	phase_index = -1
	printerr("[bench] START test=gxm3d_tux")
	_next_phase()

func _next_phase() -> void:
	phase_index += 1
	if phase_index >= phases.size():
		_reset_stage()
		_report()
		return
	_reset_stage()
	samples = PackedFloat32Array()
	rng.seed = 1234 + phase_index
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
	printerr("[bench] RESULT test=gxm3d_tux phase=%s frames=%d avg_ms=%.3f p50_ms=%.3f p95_ms=%.3f p99_ms=%.3f max_ms=%.3f fps=%.1f" % [
		result.phase, result.frames, result.avg_ms, result.p50_ms, result.p95_ms, result.p99_ms, result.max_ms, result.fps])

func _pct(sorted: PackedFloat32Array, q: float) -> float:
	if sorted.is_empty():
		return 0.0
	return sorted[clampi(int(ceil(q * sorted.size())) - 1, 0, sorted.size() - 1)]

func _report() -> void:
	mode = "results"
	var lines := PackedStringArray()
	lines.append("phase,frames,avg_ms,p50_ms,p95_ms,p99_ms,max_ms,fps")
	var table := "Benchmark concluido.  X: voltar a cena   START: sair\n"
	for r in results:
		lines.append("%s,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f" % [r.phase, r.frames, r.avg_ms, r.p50_ms, r.p95_ms, r.p99_ms, r.max_ms, r.fps])
		table += "%-28s %6.2f ms  p95 %6.2f  %5.1f FPS\n" % [r.phase, r.avg_ms, r.p95_ms, r.fps]
	var path := "user://benchmark_gxm3d_tux.txt"
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file:
		file.store_string("\n".join(lines) + "\n")
		file.close()
	printerr("[bench] DONE test=gxm3d_tux results=%s" % ProjectSettings.globalize_path(path))
	panel.size = Vector2(620, 40 + 22 * results.size())
	hud.text = table

# --- Procedural assets ------------------------------------------------------

## Four-frame walking penguin, 32x32 per frame, transparent background.
func _make_tux_sheet() -> ImageTexture:
	var s := SHEET_FRAME
	var image := Image.create(s * SHEET_FRAMES, s, false, Image.FORMAT_RGBA8)
	image.fill(Color(0, 0, 0, 0))
	var black := Color(0.08, 0.08, 0.1)
	var white := Color(0.96, 0.96, 0.96)
	var yellow := Color(1.0, 0.75, 0.1)
	for f in SHEET_FRAMES:
		var ox := f * s
		var bob := 1 if f % 2 == 1 else 0
		var step: int = [0, 2, 0, -2][f]
		for y in s:
			for x in s:
				var px := x - 16.0 + 0.5
				var py := y - 14.0 - bob + 0.5
				# Body and head ellipses.
				if (px * px) / 81.0 + (py - 4.0) * (py - 4.0) / 121.0 < 1.0 or (px * px) / 36.0 + (py + 7.0) * (py + 7.0) / 30.0 < 1.0:
					image.set_pixel(ox + x, y, black)
				if (px * px) / 36.0 + (py - 6.0) * (py - 6.0) / 64.0 < 1.0:
					image.set_pixel(ox + x, y, white)
				# Eyes and beak.
				if (abs(px - 2.5) < 1.5 or abs(px + 2.5) < 1.5) and abs(py + 8.0) < 1.5:
					image.set_pixel(ox + x, y, white)
				if abs(px) < 2.5 - (py + 4.5) and py > -5.5 and py < -3.0:
					image.set_pixel(ox + x, y, yellow)
		# Feet, swapping forward/back per frame.
		for foot in [-1, 1]:
			var fx: int = 16 + foot * 4 + (step if foot < 0 else -step)
			for dx in range(-3, 3):
				for dy in 2:
					var px2 := ox + fx + dx
					var py2 := 27 + dy
					if px2 >= ox and px2 < ox + s and py2 < s:
						image.set_pixel(px2, py2, yellow)
	return ImageTexture.create_from_image(image)

func _make_checker() -> ImageTexture:
	var image := Image.create(64, 64, false, Image.FORMAT_RGBA8)
	for y in 64:
		for x in 64:
			var on := ((x / 8) + (y / 8)) % 2 == 0
			image.set_pixel(x, y, Color(0.18, 0.2, 0.24) if on else Color(0.1, 0.11, 0.14))
	return ImageTexture.create_from_image(image)

## Ring with a fully transparent centre: the hole must show what is behind.
func _make_hole_texture() -> ImageTexture:
	var image := Image.create(32, 32, false, Image.FORMAT_RGBA8)
	for y in 32:
		for x in 32:
			var d := Vector2(x - 15.5, y - 15.5).length()
			image.set_pixel(x, y, Color(0.3, 0.9, 1.0, 0.85) if d < 15.0 and d > 9.0 else Color(0, 0, 0, 0))
	return ImageTexture.create_from_image(image)

func _material(color: Color, texture: Texture2D = null, shaded := false) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_PER_VERTEX if shaded else BaseMaterial3D.SHADING_MODE_UNSHADED
	material.albedo_color = color
	material.albedo_texture = texture
	return material

func _mesh(parent: Node3D, mesh: Mesh, material: Material, position: Vector3, scale := Vector3.ONE) -> MeshInstance3D:
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	instance.material_override = material
	instance.position = position
	instance.scale = scale
	parent.add_child(instance)
	return instance

## Tux from primitives: about 2.6 units tall, feet at y = 0.
func _make_tux(parent: Node3D) -> Node3D:
	var tux := Node3D.new()
	parent.add_child(tux)
	var black := _material(Color(0.07, 0.07, 0.09), null, true)
	var white := _material(Color(0.95, 0.95, 0.95), null, true)
	var yellow := _material(Color(1.0, 0.72, 0.08), null, true)
	var sphere := SphereMesh.new()
	sphere.radial_segments = 16
	sphere.rings = 8
	_mesh(tux, sphere, black, Vector3(0, 1.0, 0), Vector3(1.6, 2.0, 1.4))      # body
	_mesh(tux, sphere, white, Vector3(0, 0.9, 0.32), Vector3(1.25, 1.6, 0.9))  # belly
	_mesh(tux, sphere, black, Vector3(0, 2.15, 0.05), Vector3(1.1, 1.0, 1.0))  # head
	for side in [-1, 1]:
		_mesh(tux, sphere, white, Vector3(side * 0.2, 2.25, 0.45), Vector3(0.28, 0.36, 0.2))
		_mesh(tux, sphere, black, Vector3(side * 0.2, 2.24, 0.54), Vector3(0.12, 0.16, 0.08))
		var wing := _mesh(tux, sphere, black, Vector3(side * 0.82, 1.15, 0), Vector3(0.3, 1.2, 0.6))
		wing.rotation.z = side * 0.25
		_mesh(tux, sphere, yellow, Vector3(side * 0.38, 0.06, 0.3), Vector3(0.55, 0.16, 0.8)) # feet
	var beak := PrismMesh.new()
	beak.size = Vector3(0.45, 0.25, 0.35)
	var b := _mesh(tux, beak, yellow, Vector3(0, 2.0, 0.6))
	b.rotation.x = PI / 2.0
	return tux

func _sprite(parent: Node3D, position: Vector3, frame: int) -> Sprite3D:
	var sprite := Sprite3D.new()
	sprite.texture = sheet
	sprite.hframes = SHEET_FRAMES
	sprite.frame = frame
	sprite.pixel_size = 0.03
	sprite.billboard = BaseMaterial3D.BILLBOARD_FIXED_Y
	sprite.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	sprite.position = position
	parent.add_child(sprite)
	return sprite

func _camera(world: Node3D, distance: float, height: float, speed: float) -> Callable:
	var camera := Camera3D.new()
	world.add_child(camera)
	camera.current = true
	var orbit := func(t: float) -> void:
		camera.position = Vector3(sin(t * speed) * distance, height, cos(t * speed) * distance)
		camera.look_at(Vector3(0, 1.2, 0))
	orbit.call(0.0)
	return orbit

func _world() -> Node3D:
	var world := Node3D.new()
	stage.add_child(world)
	var sun := DirectionalLight3D.new()
	sun.rotation = Vector3(-0.8, 0.5, 0)
	world.add_child(sun)
	var floor_mesh := BoxMesh.new()
	floor_mesh.size = Vector3(20, 0.2, 20)
	_mesh(world, floor_mesh, _material(Color.WHITE, checker), Vector3(0, -0.1, 0))
	return world

# --- Scenes -----------------------------------------------------------------

func _build_showcase() -> void:
	var world := _world()
	var tux := _make_tux(world)
	# Terminal panel behind Tux, with the system name on it.
	var screen := BoxMesh.new()
	screen.size = Vector3(7, 3.6, 0.2)
	_mesh(world, screen, _material(Color(0.02, 0.12, 0.05)), Vector3(0, 2.2, -3.5))
	var terminal := Label3D.new()
	terminal.text = "$ uname -s\nLinux\n$ _"
	terminal.font_size = 48
	terminal.pixel_size = 0.01
	terminal.modulate = Color(0.3, 1.0, 0.4)
	terminal.position = Vector3(0, 2.4, -3.35)
	world.add_child(terminal)
	var title := Label3D.new()
	title.text = "LINUX"
	title.font_size = 96
	title.pixel_size = 0.02
	title.outline_size = 12
	title.position = Vector3(0, 4.8, -3.4)
	world.add_child(title)
	var counter := Label3D.new()
	counter.font_size = 40
	counter.pixel_size = 0.01
	counter.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	counter.position = Vector3(0, 3.4, 0)
	world.add_child(counter)
	# Walkers on a circle through the front and back of Tux.
	var walkers: Array[Sprite3D] = []
	for i in 8:
		walkers.append(_sprite(world, Vector3.ZERO, i % SHEET_FRAMES))
	var flash := _sprite(world, Vector3(-3.0, 0.6, 1.5), 0)
	var fade := _sprite(world, Vector3(3.0, 0.6, 1.5), 0)
	# Rings with see-through centres right in front of Tux and the panel.
	for i in 3:
		var ring := Sprite3D.new()
		ring.texture = hole_texture
		ring.pixel_size = 0.04
		ring.position = Vector3(-1.6 + i * 1.6, 1.4 + (i % 2) * 0.6, 1.2)
		world.add_child(ring)
	var orbit := _camera(world, 8.5, 3.2, 0.25)
	updater = func(t: float) -> void:
		orbit.call(t)
		tux.rotation.y = sin(t * 0.8) * 0.5
		tux.rotation.z = sin(t * 4.0) * 0.05
		counter.text = "quadro %d" % frame_counter
		var frame := int(t * 8.0) % SHEET_FRAMES
		for i in walkers.size():
			var a := t * 0.4 + i * TAU / walkers.size()
			walkers[i].position = Vector3(cos(a) * 2.6, 0.5, sin(a) * 2.6)
			walkers[i].frame = (frame + i) % SHEET_FRAMES
		flash.frame = frame
		flash.modulate = Color(1.0, 0.25, 0.25) if fmod(t, 1.0) < 0.15 else Color.WHITE
		fade.frame = frame
		fade.modulate.a = 0.5 + 0.5 * sin(t * 2.0)

func _build_sprites(count: int, animate: bool, tint: bool) -> void:
	var world := _world()
	_make_tux(world)
	var sprites: Array[Sprite3D] = []
	for i in count:
		var p := Vector3(rng.randf_range(-8, 8), 0.5, rng.randf_range(-8, 8))
		sprites.append(_sprite(world, p, i % SHEET_FRAMES))
	var orbit := _camera(world, 11.0, 5.0, 0.3)
	updater = func(t: float) -> void:
		orbit.call(t)
		if animate:
			var frame := int(t * 12.0)
			for i in sprites.size():
				sprites[i].frame = (frame + i) % SHEET_FRAMES
		if tint:
			for i in sprites.size():
				sprites[i].modulate = Color.from_hsv(fmod(t * 0.5 + i * 0.01, 1.0), 0.4, 1.0)

func _build_overlap() -> void:
	var world := _world()
	_make_tux(world)
	var box := BoxMesh.new()
	for i in 12:
		_mesh(world, box, _material(Color.from_hsv(i / 12.0, 0.6, 0.9), null, true), Vector3(rng.randf_range(-6, 6), 0.5, rng.randf_range(-6, 6)))
	for i in 300:
		var ring := Sprite3D.new()
		ring.texture = hole_texture if i % 2 == 0 else sheet
		if i % 2 == 1:
			ring.hframes = SHEET_FRAMES
			ring.frame = i % SHEET_FRAMES
		ring.pixel_size = 0.03
		ring.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		ring.position = Vector3(rng.randf_range(-7, 7), rng.randf_range(0.3, 3.5), rng.randf_range(-7, 7))
		world.add_child(ring)
	updater = _camera(world, 11.0, 4.0, 0.35)

func _build_labels() -> void:
	var world := _world()
	_make_tux(world)
	var labels: Array[Label3D] = []
	for i in 24:
		var label := Label3D.new()
		label.font_size = 32
		label.pixel_size = 0.01
		label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		label.position = Vector3((i % 6 - 2.5) * 2.4, 0.6 + (i / 6) * 0.9, -2.0 + (i / 6) * 0.3)
		world.add_child(label)
		labels.append(label)
	var orbit := _camera(world, 10.0, 3.5, 0.3)
	updater = func(t: float) -> void:
		orbit.call(t)
		for i in labels.size():
			labels[i].text = "tux %d: %d" % [i, frame_counter + i]
