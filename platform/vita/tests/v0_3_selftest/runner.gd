extends Node
## Godot4Vita v0.3 self test: runs every v0.3 device test in one app, with no
## input needed. Each stage is the original test scene (copied into its own
## folder) started with the "selftest" meta, which makes it run on its own and
## emit selftest_finished(summary) when done.
##
##   1. bench     renderer benchmark, 8 phases of 12 s (2D and 3D)
##   2. gxm3d     GXM 3D feature scene for 12 s, then 7 benchmark phases
##   3. showcase  guided tour, PCK read A/B with the prefetch paused/active,
##                about a minute of movement with the audio loop playing
##
## The run takes about 6 minutes. START skips the current stage. At the end a
## summary is shown, written to the app's log folder and to user://, and the
## app quits by itself so the runtime writes its shutdown telemetry (the final
## "[vita-io] PCK prefetch requests=..." line). Cross quits at once.
##
## Command line after "--": --quick shortens the benchmark phases (host smoke).

const STAGES := [
	{"id": "bench", "scene": "res://bench/main.tscn", "title": "Benchmark de renderização 2D/3D", "detail": "8 fases de 12 s. Não mexa no console.", "timeout_s": 150.0, "vsync": false, "tilt_shift": false},
	{"id": "gxm3d", "scene": "res://gxm3d/main.tscn", "title": "GXM 3D: Tux, Pixel Plush e transparência", "detail": "12 s de cena (observe a tela), depois 7 fases de benchmark.", "timeout_s": 130.0, "vsync": false, "tilt_shift": false},
	{"id": "showcase", "scene": "res://showcase/main.tscn", "title": "Showcase v0.3: PCK, prefetch e áudio", "detail": "Tour, 12 leituras do PCK (prefetch A/B) e 1 min andando. Ouça o áudio.", "timeout_s": 200.0, "vsync": true, "tilt_shift": true},
]
const INTRO_S := 3.0
const END_QUIT_S := 25.0

var quick := false
var stage_index := -1
var stage: Node
var stage_started_ms := 0
var stage_time := 0.0
var intro_time := 0.0
var in_intro := false
var finished := false
var end_time := 0.0
var report := {}
var log_lines := PackedStringArray()
var prev_start := false
var prev_cross := false

var layer: CanvasLayer
var card: ColorRect
var card_title: Label
var card_detail: Label
var summary_label: Label

func _ready() -> void:
	quick = OS.get_cmdline_user_args().has("--quick")
	_build_ui()
	var capunlocker := OS.get_environment("GODOT_VITA_CAPUNLOCKER")
	var prefetch := OS.get_environment("GODOT_VITA_PCK_PREFETCH")
	report = {
		"version": ProjectSettings.get_setting("application/config/version", "0.3"),
		"os": OS.get_name(),
		"capunlocker": capunlocker,
		"pck_prefetch": prefetch,
		"stages": {},
	}
	_log("[selftest] BEGIN os=%s capunlocker=%s pck_prefetch=%s quick=%s stages=%d" % [OS.get_name(), capunlocker, prefetch, quick, STAGES.size()])
	_log_memory("begin")
	_next_stage()

func _log(line: String) -> void:
	printerr(line)
	log_lines.append(line)

func _log_memory(label: String) -> void:
	# The Performance memory monitors read 0 on Vita; there the runtime's own
	# "[vita-memory]" lines in godot_stderr.log are the memory record.
	if OS.get_name() == "Vita":
		return
	_log("[selftest] memory at=%s static=%d video=%d texture=%d" % [label,
			int(Performance.get_monitor(Performance.MEMORY_STATIC)),
			int(Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED)),
			int(Performance.get_monitor(Performance.RENDER_TEXTURE_MEM_USED))])

func _pressed(button: JoyButton, key: Key, prev: bool) -> Array:
	var down := Input.is_joy_button_pressed(0, button) or Input.is_key_pressed(key)
	return [down and not prev, down]

func _process(delta: float) -> void:
	var start: Array = _pressed(JOY_BUTTON_START, KEY_ESCAPE, prev_start)
	prev_start = start[1]
	var cross: Array = _pressed(JOY_BUTTON_A, KEY_ENTER, prev_cross)
	prev_cross = cross[1]
	if finished:
		end_time += delta
		var left := maxf(0.0, END_QUIT_S - end_time)
		card_detail.text = "Fechando sozinho em %d s para gravar os logs finais. X fecha agora." % int(ceil(left))
		if cross[0] or end_time >= END_QUIT_S or (quick and end_time >= 1.0):
			_log("[selftest] QUIT")
			set_process(false)
			get_tree().quit()
		return
	if in_intro:
		intro_time += delta
		if intro_time >= (0.2 if quick else INTRO_S):
			_start_stage()
		return
	stage_time += delta
	if start[0]:
		_end_stage("skipped", {})
	elif stage_time >= STAGES[stage_index].timeout_s:
		_end_stage("timeout", {})

func _next_stage() -> void:
	stage_index += 1
	if stage_index >= STAGES.size():
		_finish()
		return
	var info: Dictionary = STAGES[stage_index]
	card_title.text = "ETAPA %d DE %d  •  %s" % [stage_index + 1, STAGES.size(), info.title]
	card_detail.text = info.detail + "\nSTART pula esta etapa."
	card.visible = true
	in_intro = true
	intro_time = 0.0

func _start_stage() -> void:
	in_intro = false
	card.visible = false
	var info: Dictionary = STAGES[stage_index]
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if info.vsync else DisplayServer.VSYNC_DISABLED)
	ProjectSettings.set_setting("rendering/vita/tilt_shift", info.tilt_shift)
	var packed := load(info.scene) as PackedScene
	if not packed:
		_log("[selftest] STAGE id=%s status=load_failed scene=%s" % [info.id, info.scene])
		report.stages[info.id] = {"status": "load_failed"}
		_next_stage()
		return
	stage = packed.instantiate()
	stage.set_meta("selftest", true)
	stage.connect("selftest_finished", _on_stage_finished, CONNECT_ONE_SHOT)
	stage_time = 0.0
	stage_started_ms = Time.get_ticks_msec()
	_log("[selftest] STAGE_BEGIN id=%s index=%d" % [info.id, stage_index + 1])
	add_child(stage)
	move_child(stage, 0) # Below the runner's card layer.

func _on_stage_finished(summary: Dictionary) -> void:
	# Deferred: the stage emits from its own _process.
	_end_stage.call_deferred("ok", summary)

func _end_stage(status: String, summary: Dictionary) -> void:
	if not stage:
		return
	var info: Dictionary = STAGES[stage_index]
	var seconds := (Time.get_ticks_msec() - stage_started_ms) / 1000.0
	summary["status"] = status
	summary["seconds"] = seconds
	report.stages[info.id] = summary
	_log("[selftest] STAGE_END id=%s status=%s seconds=%.1f" % [info.id, status, seconds])
	_log_stage_summary(info.id, summary)
	remove_child(stage)
	stage.queue_free()
	stage = null
	await get_tree().process_frame
	await get_tree().process_frame
	_log_memory("after_" + info.id)
	_next_stage()

func _log_stage_summary(id: String, summary: Dictionary) -> void:
	for r in summary.get("results", []):
		_log("[selftest] result stage=%s phase=%s avg_ms=%.3f p95_ms=%.3f fps=%.1f" % [id, r.phase, r.avg_ms, r.p95_ms, r.fps])
	if id == "showcase" and summary.has("probes"):
		_log("[selftest] pck_ab median_on_ms=%.3f median_off_ms=%.3f probes=%d" % [summary.median_on_ms, summary.median_off_ms, summary.probes.size()])

func _ab_verdict(showcase: Dictionary) -> String:
	if not showcase.has("probes"):
		return "sem dados"
	if report.capunlocker != "1" or report.pck_prefetch != "1":
		return "não vale: prefetch inativo (CapUnlocker=%s, prefetch=%s)" % [report.capunlocker, report.pck_prefetch]
	var on: float = showcase.median_on_ms
	var off: float = showcase.median_off_ms
	if off <= 0.0:
		return "sem dados"
	return "ligado %.2f ms x pausado %.2f ms (%+.0f%%)" % [on, off, (on - off) / off * 100.0]

func _finish() -> void:
	finished = true
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED)
	var lines := PackedStringArray()
	lines.append("GODOT4VITA v0.3 - TESTE AUTOMÁTICO CONCLUÍDO")
	lines.append("CapUnlocker: %s   Prefetch no boot: %s" % ["sim" if report.capunlocker == "1" else "não", "ativo" if report.pck_prefetch == "1" else "inativo"])
	lines.append("")
	for info in STAGES:
		var s: Dictionary = report.stages.get(info.id, {"status": "missing"})
		var line := "%-9s %-8s" % [info.id, s.get("status", "?")]
		var results: Array = s.get("results", [])
		if not results.is_empty():
			var worst: Dictionary = results[0]
			for r in results:
				if r.avg_ms > worst.avg_ms:
					worst = r
			line += "  %d fases, mais lenta: %s %.1f ms" % [results.size(), worst.phase, worst.avg_ms]
		if info.id == "showcase" and s.has("frames"):
			line += "  %.1f ms/frame (p95 %.1f)" % [s.avg_ms, s.p95_ms]
		lines.append(line)
	lines.append("")
	lines.append("PCK A/B: " + _ab_verdict(report.stages.get("showcase", {})))
	var text := "\n".join(lines)
	for line in lines:
		_log("[selftest] " + line)
	var saved := _write_report(text)
	_log("[selftest] DONE report=%s" % ", ".join(saved))
	card_title.text = "TESTE CONCLUÍDO"
	summary_label.text = text + "\n\nRelatório: " + (saved[0] if not saved.is_empty() else "(não gravado)")
	summary_label.visible = true
	card.visible = true

func _write_report(text: String) -> PackedStringArray:
	var body := text + "\n\n--- log ---\n" + "\n".join(log_lines) + "\n\n--- json ---\n" + JSON.stringify(report, "  ") + "\n"
	var targets := PackedStringArray()
	# The loader keeps the logs in ux0:data/<folder>/logs; the export writes
	# the folder name to app0:game_data_folder.cfg.
	var cfg := FileAccess.open("app0:game_data_folder.cfg", FileAccess.READ)
	if cfg:
		var folder := cfg.get_line().strip_edges()
		cfg.close()
		if not folder.is_empty():
			targets.append("ux0:data/%s/logs/selftest_report.txt" % folder)
	targets.append("user://selftest_report.txt")
	var saved := PackedStringArray()
	for path in targets:
		var file := FileAccess.open(path, FileAccess.WRITE)
		if file:
			file.store_string(body)
			file.close()
			saved.append(ProjectSettings.globalize_path(path))
	return saved

func _build_ui() -> void:
	layer = CanvasLayer.new()
	layer.layer = 120
	add_child(layer)
	card = ColorRect.new()
	card.color = Color(0.03, 0.05, 0.09, 0.94)
	card.position = Vector2.ZERO
	card.size = Vector2(960, 544)
	layer.add_child(card)
	var accent := ColorRect.new()
	accent.color = Color("478cbf")
	accent.position = Vector2(48, 60)
	accent.size = Vector2(6, 64)
	card.add_child(accent)
	card_title = Label.new()
	card_title.position = Vector2(70, 58)
	card_title.size = Vector2(840, 40)
	card_title.add_theme_font_size_override("font_size", 26)
	card.add_child(card_title)
	card_detail = Label.new()
	card_detail.position = Vector2(70, 98)
	card_detail.size = Vector2(840, 60)
	card_detail.add_theme_font_size_override("font_size", 17)
	card_detail.add_theme_color_override("font_color", Color(0.68, 0.74, 0.82))
	card.add_child(card_detail)
	summary_label = Label.new()
	summary_label.position = Vector2(70, 170)
	summary_label.size = Vector2(840, 340)
	summary_label.add_theme_font_size_override("font_size", 15)
	summary_label.visible = false
	card.add_child(summary_label)
