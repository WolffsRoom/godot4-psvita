extends Control
## v0.4 shader stage: eight canvas_item shaders that cover the GXM translator
## contract (UV, uniform defaults, helper functions with uniforms and TIME,
## mod/atan/PI, TEXTURE_PIXEL_SIZE, a second sampler, modulate semantics and
## the screen texture with mip levels). Each tile says what it should look
## like; on the host the tiles are Godot's own reference rendering.
##
## On the Vita the runtime logs every translation and compile in
## godot_stderr.log; at the end of the stage those lines are read back and put
## in the summary, so selftest_report.txt says which shaders compiled.

signal selftest_finished(summary: Dictionary)

const ICON := preload("res://gxm3d/assets/godot_icon.png")
const LOGO := preload("res://gxm3d/assets/godot_logo.png")
const TILES := [
	{"shader": "uv_gradient", "label": "UV: vermelho → direita,\nverde ↓", "textured": false},
	{"shader": "uniform_tint", "label": "Padrões dos uniforms:\nícone rosado", "textured": true},
	{"shader": "time_wave_helper", "label": "Função auxiliar + TIME:\n4 faixas andando", "textured": false},
	{"shader": "rings_mod_atan", "label": "mod/atan/PI:\nanéis em 4 setores", "textured": false},
	{"shader": "outline", "label": "TEXTURE_PIXEL_SIZE:\ncontorno amarelo", "textured": true},
	{"shader": "dissolve_sampler", "label": "2º sampler:\nícone se dissolvendo", "textured": true},
	{"shader": "modulate_color", "label": "Modulate azul,\ncírculo invertido", "textured": true, "modulate": Color(0.35, 0.55, 1.0)},
	{"shader": "screen_blur", "label": "Tela + mips:\nfundo desfocado", "textured": false},
]
const DURATION_S := 15.0

var selftest := false
var elapsed := 0.0
var done := false

func _ready() -> void:
	selftest = has_meta("selftest")
	size = Vector2(960, 544)
	# Busy background: the blur tile must have something to blur.
	for i in 24:
		var stripe := ColorRect.new()
		stripe.color = Color.from_hsv(float(i) / 24.0, 0.7, 0.9) if i % 2 == 0 else Color(0.08, 0.09, 0.12)
		stripe.position = Vector2(i * 40, 0)
		stripe.size = Vector2(40, 544)
		add_child(stripe)
	var title := Label.new()
	title.text = "Shaders GXM (v0.4) - compare cada quadro com a descrição"
	title.position = Vector2(20, 8)
	title.add_theme_font_size_override("font_size", 18)
	add_child(title)
	for i in TILES.size():
		var info: Dictionary = TILES[i]
		var origin := Vector2(20 + (i % 4) * 235, 44 + int(i / 4) * 250)
		var back := ColorRect.new()
		back.color = Color(0, 0, 0, 0.55)
		back.position = origin - Vector2(4, 4)
		back.size = Vector2(218, 238)
		add_child(back)
		var material := ShaderMaterial.new()
		material.shader = load("res://shaders/%s.gdshader" % info.shader)
		if info.shader == "dissolve_sampler":
			material.set_shader_parameter("mask_texture", LOGO)
		var tile: Control
		if info.textured:
			var rect := TextureRect.new()
			rect.texture = ICON
			rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
			rect.stretch_mode = TextureRect.STRETCH_SCALE
			tile = rect
		else:
			var color_rect := ColorRect.new()
			color_rect.color = Color.WHITE
			tile = color_rect
		tile.material = material
		tile.position = origin
		tile.size = Vector2(170, 170) if info.textured else Vector2(210, 170)
		if info.has("modulate"):
			tile.modulate = info.modulate
		add_child(tile)
		var label := Label.new()
		label.text = info.label
		label.position = origin + Vector2(0, 176)
		label.add_theme_font_size_override("font_size", 14)
		add_child(label)

func _process(delta: float) -> void:
	elapsed += delta
	var quick := OS.get_cmdline_user_args().has("--quick")
	if not done and elapsed >= (3.0 if quick else DURATION_S):
		done = true
		var summary := collect_log_summary()
		if selftest:
			selftest_finished.emit(summary)
		else:
			printerr("[shadertest] ", JSON.stringify(summary))

static func shader_mode() -> String:
	var cfg := FileAccess.open("app0:shacc.cfg", FileAccess.READ)
	if not cfg:
		return "n/a"
	var mode := cfg.get_line().strip_edges()
	cfg.close()
	return mode

## Reads the runtime's shader lines back from godot_stderr.log (Vita only).
static func collect_log_summary() -> Dictionary:
	var summary := {"mode": shader_mode(), "translated": {}, "compiled": 0, "failed": 0, "gpu_draws": 0, "diagnostics": []}
	var folder := ""
	var cfg := FileAccess.open("app0:game_data_folder.cfg", FileAccess.READ)
	if cfg:
		folder = cfg.get_line().strip_edges()
		cfg.close()
	if folder.is_empty():
		summary["log"] = "unavailable"
		return summary
	var log := FileAccess.open("ux0:data/%s/logs/godot_stderr.log" % folder, FileAccess.READ)
	if not log:
		summary["log"] = "unavailable"
		return summary
	summary["log"] = "ux0:data/%s/logs/godot_stderr.log" % folder
	while not log.eof_reached():
		var line := log.get_line()
		if line.begins_with("[gxm2d-shader] path=res://shaders/"):
			var name := line.get_slice("path=res://shaders/", 1).get_slice(".gdshader", 0)
			summary.translated[name] = line.get_slice(" reason=", 1)
		elif line.begins_with("[gxm2d-shader] compile result"):
			if line.ends_with("native_program=0"):
				summary.failed += 1
			else:
				summary.compiled += 1
		elif line.begins_with("[gxm-shacc] GPU DRAW submitted"):
			summary.gpu_draws += 1
		elif line.begins_with("[vita-shacc] compiler") or line.begins_with("[gxm-shacc] patcher") or line.begins_with("[gxm-shacc] fragment register failed"):
			if summary.diagnostics.size() < 24:
				summary.diagnostics.append(line)
	log.close()
	return summary
