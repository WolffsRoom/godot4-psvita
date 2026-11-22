@tool
extends SceneTree

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await process_frame
	var platform = load("res://addons/psvita/platform.gd").new()
	var preset: EditorExportPreset = platform.create_preset()
	assert(platform._get_name() == "PS Vita")
	assert(platform._has_valid_export_configuration(preset, false))
	assert(platform._has_valid_export_configuration(preset, true))
	preset.set("application/title_id", "invalid/../")
	assert(not platform._has_valid_export_configuration(preset, false))
	preset.set("application/title_id", "GDOT40001")
	preset.set("application/version", "1.0")
	assert(not platform._has_valid_export_configuration(preset, false))
	preset.set("application/version", "01.00")
	preset.set("custom_template/release", "res://missing.zip")
	assert(not platform._has_valid_export_configuration(preset, false))
	ProjectSettings.set_setting("application/run/main_scene", "")
	ProjectSettings.set_setting("application/run/main_scene.Vita", "res://main.tscn")
	ProjectSettings.set_setting("rendering/renderer/rendering_method.Vita", "gl_compatibility")
	ProjectSettings.set_setting("display/window/size/viewport_width.Vita", 960)
	ProjectSettings.set_setting("display/window/size/viewport_height.Vita", 544)
	ProjectSettings.set_setting("threading/worker_pool/max_threads.Vita", 2)
	assert(preset.get_project_setting("application/run/main_scene") == "res://main.tscn")
	platform._has_valid_project_configuration(preset)
	assert(platform.get_config_error() != "Set a main scene before exporting.")
	assert(not platform._safe_path("../outside"))
	assert(not platform._safe_path("C:/outside"))
	assert(platform._safe_path(".godot/imported/texture.ctex"))
	var sfo: PackedByteArray = platform.make_sfo("Jogo ç", "TEST40001", "02.03")
	assert(sfo.decode_u32(0) == 0x46535000)
	assert(sfo.decode_u32(16) == 11)
	print("PASS: Vita preset validation, main-scene feature override, path validation and SFO header")
	quit()
