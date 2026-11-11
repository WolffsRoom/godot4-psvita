extends Node
## GXM3D feature test and benchmark for the native GXM 3D renderer.
##
## Scene: Tux and the Godot Pixel Plush on a stage, with the Godot logo on
## the back wall. Each item checks one renderer path:
##   - pixel penguins walking in front: Sprite3D frame updates;
##   - floating Godot icons passing in front of and behind the characters:
##     sorted transparency (their clear pixels must not cut holes);
##   - the frame counter on the display: Label3D text updated every frame;
##   - one icon flashing red and one fading: Sprite3D modulate (vertex tint);
##   - the Pixel Plush: skinned animation.
## Sprites and labels are not billboards: the GXM 3D path draws them with
## the node transform, so the camera stays in front of the stage.
## Benchmark: timed phases. Results on screen, in stderr ("[bench] RESULT")
## and in user://benchmark_gxm3d.txt.
##
## Pad: Cross runs the benchmark (and returns from the results), START
## quits. Host: Enter / Escape. Command line after "--":
##   --bench  start the benchmark at once
##   --quick  short phases, then quit (host smoke test)

const WARMUP_S := 2.0
const MEASURE_S := 8.0
const SHEET_FRAME := 32
const SHEET_FRAMES := 4
const GODOT_BLUE := Color("478cbf")
const PANEL_BG := Color(0.05, 0.07, 0.11, 0.86)
const TEXT_DIM := Color(0.68, 0.74, 0.82)

var quick := false
var phases: Array[Dictionary] = []
var phase_index := -1
var phase_time := 0.0
var last_ticks := 0
var samples := PackedFloat32Array()
var results: Array[Dictionary] = []
var mode := "scene" # scene, bench, results
var stage: Node
var updater := Callable()
var rng := RandomNumberGenerator.new()
var frame_counter := 0
var prev_buttons := {}

# Set by the v0.3 self test runner (meta "selftest"): the scene is shown
# for SELFTEST_SCENE_S, then the benchmark runs, then selftest_finished.
const SELFTEST_SCENE_S := 12.0
const SELFTEST_RESULTS_S := 4.0
signal selftest_finished(summary: Dictionary)
var selftest := false
var selftest_done := false

var penguin_sheet: ImageTexture
var floor_texture: ImageTexture
var godot_icon: Texture2D
var godot_logo: Texture2D
var plush_scene: PackedScene

var ui_info: Control
var ui_bench: Control
var ui_results: Control
var fps_label: Label
var ms_label: Label
var bench_title: Label
var bench_bar: ProgressBar
var results_grid: GridContainer

func _ready() -> void:
	var args := OS.get_cmdline_user_args()
	quick = args.has("--quick")
	selftest = has_meta("selftest")
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	penguin_sheet = _make_penguin_sheet()
	floor_texture = _make_floor_texture()
	godot_icon = load("res://gxm3d/assets/godot_icon.png") as Texture2D
	godot_logo = load("res://gxm3d/assets/godot_logo.png") as Texture2D
	plush_scene = load("res://gxm3d/models/pixel_plush/godot_pixel_plush.gltf") as PackedScene
	phases = [
		{"name": "full_scene", "label": "Full scene", "build": _build_scene},
		{"name": "sprite3d_static_200", "label": "Sprite3D static x200", "build": _build_sprites.bind(200, false, false)},
		{"name": "sprite3d_animated_200", "label": "Sprite3D animated x200", "build": _build_sprites.bind(200, true, false)},
		{"name": "sprite3d_animated_tint_200", "label": "Animated + tint x200", "build": _build_sprites.bind(200, true, true)},
		{"name": "transparent_overlap_300", "label": "Transparency overlap x300", "build": _build_overlap},
		{"name": "label3d_live_24", "label": "Live Label3D x24", "build": _build_labels},
		{"name": "skinned_plush_6", "label": "Skinned Pixel Plush x6", "build": _build_plush_crowd},
	]
	_build_ui()
	printerr("[bench] READY test=gxm3d phases=%d quick=%s plush=%s icon=%s" % [phases.size(), quick, plush_scene != null, godot_icon != null])
	if args.has("--bench") or quick:
		_start_bench()
	else:
		_show_scene()

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
	if start and not selftest:
		get_tree().quit()
		return
	phase_time += delta
	if updater.is_valid():
		updater.call(phase_time)
	fps_label.text = "%d FPS" % Engine.get_frames_per_second()
	ms_label.text = "%.2f ms" % frame_ms
	match mode:
		"scene":
			if cross or (selftest and phase_time >= SELFTEST_SCENE_S):
				_start_bench()
		"bench":
			if phase_time > _warmup():
				samples.append(frame_ms)
			bench_bar.value = clampf(phase_time / (_warmup() + _measure()), 0.0, 1.0) * 100.0
			if phase_time >= _warmup() + _measure():
				_finish_phase()
				_next_phase()
		"results":
			if selftest:
				if phase_time >= SELFTEST_RESULTS_S and not selftest_done:
					selftest_done = true
					selftest_finished.emit({"results": results})
			elif cross:
				_show_scene()
			elif quick:
				get_tree().quit()

# --- Flow -------------------------------------------------------------------

func _reset_stage() -> void:
	if stage:
		stage.queue_free()
	updater = Callable()
	phase_time = 0.0
	stage = Node.new()
	add_child(stage)
	move_child(stage, 0) # Below the UI layer.

func _set_ui(which: String) -> void:
	ui_info.visible = which == "scene"
	ui_bench.visible = which == "bench"
	ui_results.visible = which == "results"

func _show_scene() -> void:
	mode = "scene"
	_set_ui(mode)
	_reset_stage()
	rng.seed = 42
	_build_scene()
	last_ticks = Time.get_ticks_usec()

func _start_bench() -> void:
	mode = "bench"
	_set_ui(mode)
	results.clear()
	phase_index = -1
	printerr("[bench] START test=gxm3d")
	_next_phase()

func _next_phase() -> void:
	phase_index += 1
	if phase_index >= phases.size():
		_reset_stage()
		_build_backdrop_only()
		_report()
		return
	_reset_stage()
	samples = PackedFloat32Array()
	rng.seed = 1234 + phase_index
	bench_title.text = "BENCHMARK  %d / %d   %s" % [phase_index + 1, phases.size(), phases[phase_index].label]
	bench_bar.value = 0.0
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
		"label": phases[phase_index].label,
		"frames": count,
		"avg_ms": avg,
		"p50_ms": _pct(sorted, 0.50),
		"p95_ms": _pct(sorted, 0.95),
		"p99_ms": _pct(sorted, 0.99),
		"max_ms": sorted[count - 1] if count else 0.0,
		"fps": 1000.0 / avg if avg > 0.0 else 0.0,
	}
	results.append(result)
	printerr("[bench] RESULT test=gxm3d phase=%s frames=%d avg_ms=%.3f p50_ms=%.3f p95_ms=%.3f p99_ms=%.3f max_ms=%.3f fps=%.1f" % [
		result.phase, result.frames, result.avg_ms, result.p50_ms, result.p95_ms, result.p99_ms, result.max_ms, result.fps])

func _pct(sorted: PackedFloat32Array, q: float) -> float:
	if sorted.is_empty():
		return 0.0
	return sorted[clampi(int(ceil(q * sorted.size())) - 1, 0, sorted.size() - 1)]

func _report() -> void:
	mode = "results"
	_set_ui(mode)
	var lines := PackedStringArray()
	lines.append("phase,frames,avg_ms,p50_ms,p95_ms,p99_ms,max_ms,fps")
	for child in results_grid.get_children():
		child.queue_free()
	for header in ["Phase", "Avg", "P95", "FPS"]:
		results_grid.add_child(_label(header, 15, GODOT_BLUE, HORIZONTAL_ALIGNMENT_LEFT if header == "Phase" else HORIZONTAL_ALIGNMENT_RIGHT))
	for r in results:
		lines.append("%s,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f" % [r.phase, r.frames, r.avg_ms, r.p50_ms, r.p95_ms, r.p99_ms, r.max_ms, r.fps])
		results_grid.add_child(_label(r.label, 15, Color.WHITE))
		results_grid.add_child(_label("%.2f ms" % r.avg_ms, 15, Color.WHITE, HORIZONTAL_ALIGNMENT_RIGHT))
		results_grid.add_child(_label("%.2f ms" % r.p95_ms, 15, TEXT_DIM, HORIZONTAL_ALIGNMENT_RIGHT))
		results_grid.add_child(_label("%.1f" % r.fps, 15, Color("8fdc8f"), HORIZONTAL_ALIGNMENT_RIGHT))
	var path := "user://benchmark_gxm3d.txt"
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file:
		file.store_string("\n".join(lines) + "\n")
		file.close()
	printerr("[bench] DONE test=gxm3d results=%s" % ProjectSettings.globalize_path(path))

# --- UI ---------------------------------------------------------------------

func _panel(parent: Node, rect: Rect2, accent := true) -> PanelContainer:
	var panel := PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = PANEL_BG
	style.set_corner_radius_all(10)
	style.content_margin_left = 16
	style.content_margin_right = 16
	style.content_margin_top = 10
	style.content_margin_bottom = 10
	if accent:
		style.border_width_left = 4
		style.border_color = GODOT_BLUE
	panel.add_theme_stylebox_override("panel", style)
	panel.position = rect.position
	panel.custom_minimum_size = rect.size
	parent.add_child(panel)
	return panel

func _label(text: String, size: int, color: Color, align := HORIZONTAL_ALIGNMENT_LEFT) -> Label:
	var label := Label.new()
	label.text = text
	label.horizontal_alignment = align
	label.add_theme_font_size_override("font_size", size)
	label.add_theme_color_override("font_color", color)
	return label

func _build_ui() -> void:
	var layer := CanvasLayer.new()
	layer.layer = 100
	add_child(layer)
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(root)
	# Frame rate, always visible.
	var stats := _panel(root, Rect2(828, 12, 120, 56), false)
	var stats_box := VBoxContainer.new()
	stats_box.add_theme_constant_override("separation", 0)
	stats.add_child(stats_box)
	fps_label = _label("-- FPS", 20, Color.WHITE, HORIZONTAL_ALIGNMENT_RIGHT)
	ms_label = _label("-- ms", 13, TEXT_DIM, HORIZONTAL_ALIGNMENT_RIGHT)
	stats_box.add_child(fps_label)
	stats_box.add_child(ms_label)
	# Scene: what each element tests.
	ui_info = Control.new()
	ui_info.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(ui_info)
	var info := _panel(ui_info, Rect2(12, 12, 330, 0))
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 3)
	info.add_child(box)
	box.add_child(_label("GXM3D FEATURE TEST", 19, GODOT_BLUE))
	box.add_child(_label("Godot 4 on PS Vita  |  native GXM renderer", 12, TEXT_DIM))
	var checks := [
		[Color("ffd166"), "Walking penguins", "Sprite3D frame updates"],
		[Color("5ce1e6"), "Floating Godot icons", "sorted transparency"],
		[Color("8fdc8f"), "Frame counter", "live Label3D text"],
		[Color("ff6b6b"), "Flash and fade", "vertex tint (modulate)"],
		[Color("c792ea"), "Pixel Plush", "skinned animation"],
	]
	for check in checks:
		var row := HBoxContainer.new()
		row.add_theme_constant_override("separation", 8)
		var dot := ColorRect.new()
		dot.color = check[0]
		dot.custom_minimum_size = Vector2(8, 8)
		dot.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		row.add_child(dot)
		row.add_child(_label(check[1], 13, Color.WHITE))
		row.add_child(_label(check[2], 13, TEXT_DIM))
		box.add_child(row)
	var footer := _panel(ui_info, Rect2(12, 500, 0, 32), false)
	footer.add_child(_label("[X] Run benchmark      [START] Quit", 13, Color.WHITE))
	# Benchmark progress.
	ui_bench = Control.new()
	ui_bench.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(ui_bench)
	var bench := _panel(ui_bench, Rect2(12, 12, 460, 0))
	var bench_box := VBoxContainer.new()
	bench.add_child(bench_box)
	bench_title = _label("BENCHMARK", 15, Color.WHITE)
	bench_box.add_child(bench_title)
	bench_bar = ProgressBar.new()
	bench_bar.show_percentage = false
	bench_bar.custom_minimum_size = Vector2(420, 8)
	var fill := StyleBoxFlat.new()
	fill.bg_color = GODOT_BLUE
	fill.set_corner_radius_all(4)
	var track := StyleBoxFlat.new()
	track.bg_color = Color(1, 1, 1, 0.12)
	track.set_corner_radius_all(4)
	bench_bar.add_theme_stylebox_override("fill", fill)
	bench_bar.add_theme_stylebox_override("background", track)
	bench_box.add_child(bench_bar)
	# Results table.
	ui_results = Control.new()
	ui_results.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(ui_results)
	var table := _panel(ui_results, Rect2(220, 70, 520, 0))
	var table_box := VBoxContainer.new()
	table_box.add_theme_constant_override("separation", 8)
	table.add_child(table_box)
	table_box.add_child(_label("BENCHMARK RESULTS", 19, GODOT_BLUE))
	results_grid = GridContainer.new()
	results_grid.columns = 4
	results_grid.add_theme_constant_override("h_separation", 28)
	results_grid.add_theme_constant_override("v_separation", 4)
	table_box.add_child(results_grid)
	table_box.add_child(_label("Saved to user://benchmark_gxm3d.txt      [X] Back to scene   [START] Quit", 12, TEXT_DIM))
	_set_ui("scene")

# --- Procedural assets ------------------------------------------------------

## Four-frame walking penguin, 32x32 per frame, transparent background.
func _make_penguin_sheet() -> ImageTexture:
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
				if (px * px) / 81.0 + (py - 4.0) * (py - 4.0) / 121.0 < 1.0 or (px * px) / 36.0 + (py + 7.0) * (py + 7.0) / 30.0 < 1.0:
					image.set_pixel(ox + x, y, black)
				if (px * px) / 36.0 + (py - 6.0) * (py - 6.0) / 64.0 < 1.0:
					image.set_pixel(ox + x, y, white)
				if (absf(px - 2.5) < 1.5 or absf(px + 2.5) < 1.5) and absf(py + 8.0) < 1.5:
					image.set_pixel(ox + x, y, white)
				if absf(px) < 2.5 - (py + 4.5) and py > -5.5 and py < -3.0:
					image.set_pixel(ox + x, y, yellow)
		for foot in [-1, 1]:
			var fx: int = 16 + foot * 4 + (step if foot < 0 else -step)
			for dx in range(-3, 3):
				for dy in 2:
					var px2 := ox + fx + dx
					var py2 := 27 + dy
					if px2 >= ox and px2 < ox + s and py2 < s:
						image.set_pixel(px2, py2, yellow)
	return ImageTexture.create_from_image(image)

## Dark tiles with thin blue grid lines, 16x16 tiles over the whole floor
## (the GXM 3D path ignores uv1_scale, so the grid is baked in).
func _make_floor_texture() -> ImageTexture:
	var image := Image.create(256, 256, false, Image.FORMAT_RGBA8)
	for y in 256:
		for x in 256:
			var line := x % 16 == 0 or y % 16 == 0
			var tile := ((x / 16) + (y / 16)) % 2 == 0
			var base := Color(0.075, 0.09, 0.13) if tile else Color(0.06, 0.072, 0.105)
			image.set_pixel(x, y, Color(0.2, 0.36, 0.55) if line else base)
	return ImageTexture.create_from_image(image)

func _material(color: Color, texture: Texture2D = null, shaded := false) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_PER_VERTEX if shaded else BaseMaterial3D.SHADING_MODE_UNSHADED
	material.albedo_color = color
	material.albedo_texture = texture
	return material

func _mesh(parent: Node3D, mesh: Mesh, material: Material, at: Vector3, size := Vector3.ONE) -> MeshInstance3D:
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	instance.material_override = material
	instance.position = at
	instance.scale = size
	parent.add_child(instance)
	return instance

func _cylinder(radius: float, height: float) -> CylinderMesh:
	var cylinder := CylinderMesh.new()
	cylinder.top_radius = radius
	cylinder.bottom_radius = radius
	cylinder.height = height
	cylinder.radial_segments = 32
	cylinder.rings = 1
	return cylinder

## Tux from primitives: about 2.6 units tall, feet at y = 0.
func _make_tux(parent: Node3D) -> Node3D:
	var tux := Node3D.new()
	parent.add_child(tux)
	var black := _material(Color(0.09, 0.09, 0.12), null, true)
	var white := _material(Color(0.97, 0.97, 0.97), null, true)
	var yellow := _material(Color(1.0, 0.72, 0.08), null, true)
	var sphere := SphereMesh.new()
	sphere.radial_segments = 16
	sphere.rings = 8
	_mesh(tux, sphere, black, Vector3(0, 1.0, 0), Vector3(1.6, 2.0, 1.4))
	_mesh(tux, sphere, white, Vector3(0, 0.9, 0.32), Vector3(1.25, 1.6, 0.9))
	_mesh(tux, sphere, black, Vector3(0, 2.15, 0.05), Vector3(1.1, 1.0, 1.0))
	for side in [-1, 1]:
		_mesh(tux, sphere, white, Vector3(side * 0.2, 2.25, 0.45), Vector3(0.28, 0.36, 0.2))
		_mesh(tux, sphere, black, Vector3(side * 0.2, 2.24, 0.54), Vector3(0.12, 0.16, 0.08))
		var wing := _mesh(tux, sphere, black, Vector3(side * 0.82, 1.15, 0), Vector3(0.3, 1.2, 0.6))
		wing.rotation.z = side * 0.25
		_mesh(tux, sphere, yellow, Vector3(side * 0.38, 0.06, 0.3), Vector3(0.55, 0.16, 0.8))
	var beak := PrismMesh.new()
	beak.size = Vector3(0.45, 0.25, 0.35)
	var b := _mesh(tux, beak, yellow, Vector3(0, 2.0, 0.6))
	b.rotation.x = PI / 2.0
	return tux

func _make_plush(parent: Node3D, animation: String) -> Node3D:
	if not plush_scene:
		return null
	var plush := plush_scene.instantiate() as Node3D
	parent.add_child(plush)
	var player := _find_animation_player(plush)
	if player and player.has_animation(animation):
		player.get_animation(animation).loop_mode = Animation.LOOP_LINEAR
		player.play(animation)
	return plush

func _find_animation_player(node: Node) -> AnimationPlayer:
	if node is AnimationPlayer:
		return node
	for child in node.get_children():
		var found := _find_animation_player(child)
		if found:
			return found
	return null

func _penguin(parent: Node3D, at: Vector3, frame: int) -> Sprite3D:
	var sprite := Sprite3D.new()
	sprite.texture = penguin_sheet
	sprite.hframes = SHEET_FRAMES
	sprite.frame = frame
	sprite.pixel_size = 0.025
	sprite.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	sprite.position = at
	parent.add_child(sprite)
	return sprite

func _icon(parent: Node3D, at: Vector3, scale_px: float) -> Sprite3D:
	var sprite := Sprite3D.new()
	sprite.texture = godot_icon
	sprite.pixel_size = scale_px
	sprite.position = at
	parent.add_child(sprite)
	return sprite

# --- Stage ------------------------------------------------------------------

## Floor, round stage and the curved back wall with the Godot logo.
func _environment(full: bool) -> Node3D:
	var world := Node3D.new()
	stage.add_child(world)
	var sun := DirectionalLight3D.new()
	sun.rotation = Vector3(-0.9, 0.5, 0)
	sun.light_color = Color(1.0, 0.97, 0.92)
	world.add_child(sun)
	var fill := OmniLight3D.new()
	fill.light_color = GODOT_BLUE.lightened(0.3)
	fill.omni_range = 9.0
	fill.position = Vector3(0, 3.5, 3.0)
	world.add_child(fill)
	var floor_mesh := BoxMesh.new()
	floor_mesh.size = Vector3(40, 0.2, 40)
	var floor_material := _material(Color.WHITE, floor_texture)
	_mesh(world, floor_mesh, floor_material, Vector3(0, -0.1, 0))
	_mesh(world, _cylinder(5.6, 0.12), _material(GODOT_BLUE), Vector3(0, 0.02, 0))
	_mesh(world, _cylinder(5.4, 0.16), _material(Color(0.11, 0.14, 0.2)), Vector3(0, 0.04, 0))
	if not full:
		return world
	# Curved wall of panels with light strips between them.
	var panel_mesh := BoxMesh.new()
	panel_mesh.size = Vector3(1.6, 5.0, 0.2)
	var strip_mesh := BoxMesh.new()
	strip_mesh.size = Vector3(0.06, 5.0, 0.22)
	var panel_material := _material(Color(0.13, 0.16, 0.22))
	var strip_material := _material(GODOT_BLUE.lightened(0.2))
	for i in 9:
		var angle := deg_to_rad(-64.0 + i * 16.0)
		var at := Vector3(sin(angle) * 7.5, 2.5, -cos(angle) * 7.5)
		var p := _mesh(world, panel_mesh, panel_material, at)
		p.rotation.y = -angle
		var strip_angle := angle + deg_to_rad(8.0)
		if i < 8:
			var s := _mesh(world, strip_mesh, strip_material, Vector3(sin(strip_angle) * 7.4, 2.5, -cos(strip_angle) * 7.4))
			s.rotation.y = -strip_angle
	var logo := Sprite3D.new()
	logo.texture = godot_logo
	logo.pixel_size = 0.0058
	logo.position = Vector3(0, 3.7, -7.2)
	world.add_child(logo)
	var caption := Label3D.new()
	caption.text = "NATIVE GXM 3D"
	caption.font_size = 52
	caption.pixel_size = 0.0065
	caption.modulate = TEXT_DIM
	caption.position = Vector3(0, 2.25, -7.15)
	world.add_child(caption)
	return world

func _camera(world: Node3D, distance: float, height: float, sweep: float, target := Vector3(0, 1.3, 0)) -> Callable:
	var camera := Camera3D.new()
	camera.fov = 60.0
	world.add_child(camera)
	camera.current = true
	var move := func(t: float) -> void:
		var a := sin(t * 0.22) * sweep
		camera.position = Vector3(sin(a) * distance, height, cos(a) * distance)
		camera.look_at(target)
	move.call(0.0)
	return move

func _build_backdrop_only() -> void:
	var world := _environment(true)
	updater = _camera(world, 9.5, 3.2, 0.3)

# --- Scenes -----------------------------------------------------------------

func _build_scene() -> void:
	var world := _environment(true)
	# Pedestals.
	for x in [-2.3, 2.3]:
		_mesh(world, _cylinder(1.25, 0.45), _material(Color(0.82, 0.85, 0.9), null, true), Vector3(x, 0.3, 0))
		_mesh(world, _cylinder(1.3, 0.06), _material(GODOT_BLUE), Vector3(x, 0.54, 0))
	var tux := _make_tux(world)
	tux.position = Vector3(-2.3, 0.57, 0)
	tux.scale = Vector3.ONE * 0.75
	var plush := _make_plush(world, "idle")
	if plush:
		plush.position = Vector3(2.3, 0.57, 0)
		plush.scale = Vector3.ONE * 0.8
	var names := [["TUX", -2.3], ["GODOT PLUSH", 2.3]]
	for entry in names:
		var tag := Label3D.new()
		tag.text = entry[0]
		tag.font_size = 32
		tag.pixel_size = 0.008
		tag.outline_size = 8
		tag.position = Vector3(entry[1], 0.3, 1.32)
		world.add_child(tag)
	# Display between the pedestals with the live counter.
	var display := BoxMesh.new()
	display.size = Vector3(1.7, 0.75, 0.08)
	_mesh(world, display, _material(Color(0.03, 0.05, 0.08)), Vector3(0, 1.55, -0.9))
	var counter := Label3D.new()
	counter.font_size = 34
	counter.pixel_size = 0.008
	counter.modulate = Color("8fdc8f")
	counter.position = Vector3(0, 1.55, -0.85)
	world.add_child(counter)
	# Godot icons orbiting through the space between the characters.
	var icons: Array[Sprite3D] = []
	for i in 6:
		icons.append(_icon(world, Vector3.ZERO, 0.0032))
	# Penguins walking back and forth along the front of the stage.
	var penguins: Array[Sprite3D] = []
	for i in 5:
		penguins.append(_penguin(world, Vector3.ZERO, i % SHEET_FRAMES))
	var move := _camera(world, 9.5, 3.3, 0.45)
	updater = func(t: float) -> void:
		move.call(t)
		tux.rotation.y = sin(t * 0.9) * 0.35
		tux.rotation.z = sin(t * 4.0) * 0.04
		if plush:
			plush.rotation.y = sin(t * 0.7) * 0.4
		counter.text = "FRAME %06d" % frame_counter
		for i in icons.size():
			var a := t * 0.5 + i * TAU / icons.size()
			icons[i].position = Vector3(cos(a) * 3.4, 2.0 + sin(t * 1.6 + i) * 0.35, sin(a) * 1.8 + 0.4)
			icons[i].modulate = Color.WHITE
		icons[0].modulate = Color(1.0, 0.3, 0.3) if fmod(t, 1.2) < 0.18 else Color.WHITE
		icons[3].modulate.a = 0.5 + 0.5 * sin(t * 2.0)
		var frame := int(t * 8.0)
		for i in penguins.size():
			var x := fmod(t * 0.9 + i * 1.7, 8.0) - 4.0
			var forward := int(t * 0.9 / 8.0 + i * 0.2) % 2 == 0
			penguins[i].position = Vector3(x if forward else -x, 0.48, 3.6 + (i % 2) * 0.5)
			penguins[i].flip_h = not forward
			penguins[i].frame = (frame + i) % SHEET_FRAMES

func _build_sprites(count: int, animate: bool, tint: bool) -> void:
	var world := _environment(false)
	var sprites: Array[Sprite3D] = []
	for i in count:
		var p := Vector3(rng.randf_range(-9, 9), 0.42, rng.randf_range(-9, 9))
		sprites.append(_penguin(world, p, i % SHEET_FRAMES))
	var move := _camera(world, 12.0, 5.5, 0.6)
	updater = func(t: float) -> void:
		move.call(t)
		if animate:
			var frame := int(t * 12.0)
			for i in sprites.size():
				sprites[i].frame = (frame + i) % SHEET_FRAMES
		if tint:
			for i in sprites.size():
				sprites[i].modulate = Color.from_hsv(fmod(t * 0.5 + i * 0.01, 1.0), 0.45, 1.0)

func _build_overlap() -> void:
	var world := _environment(true)
	var tux := _make_tux(world)
	tux.scale = Vector3.ONE * 0.9
	for i in 300:
		var sprite: Sprite3D
		if i % 2 == 0:
			sprite = _icon(world, Vector3.ZERO, 0.0025)
		else:
			sprite = _penguin(world, Vector3.ZERO, i % SHEET_FRAMES)
		sprite.position = Vector3(rng.randf_range(-6, 6), rng.randf_range(0.4, 4.0), rng.randf_range(-6, 5))
	updater = _camera(world, 10.0, 3.5, 0.5)

func _build_labels() -> void:
	var world := _environment(false)
	var labels: Array[Label3D] = []
	for i in 24:
		var label := Label3D.new()
		label.font_size = 28
		label.pixel_size = 0.006
		label.modulate = Color("8fdc8f") if i % 2 == 0 else Color.WHITE
		label.position = Vector3((i % 4 - 1.5) * 2.2, 0.9 + (i / 4) * 0.55, -1.0)
		world.add_child(label)
		labels.append(label)
	var move := _camera(world, 9.0, 3.0, 0.45)
	updater = func(t: float) -> void:
		move.call(t)
		for i in labels.size():
			labels[i].text = "NODE %02d  %06d" % [i, frame_counter + i]

func _build_plush_crowd() -> void:
	var world := _environment(true)
	var animations := ["idle", "running", "jump", "rolling", "falling", "idle"]
	for i in 6:
		var plush := _make_plush(world, animations[i])
		if plush:
			plush.position = Vector3((i % 3 - 1) * 2.4, 0.08, -1.0 + (i / 3) * 2.4)
			plush.scale = Vector3.ONE * 0.7
	updater = _camera(world, 9.0, 3.4, 0.45)
