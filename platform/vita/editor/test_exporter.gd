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
	# Feature overrides (lowercase and legacy ".Vita" keys) must reach the
	# project validator, including a Vita-only main scene. Ported from
	# WolffsRoom/godot4-psvita#9 by gtyire40-cpu.
	assert("vita" in platform._get_platform_features())
	ProjectSettings.set_setting("application/run/main_scene", "")
	ProjectSettings.set_setting("application/run/main_scene.Vita", "res://main.tscn")
	ProjectSettings.set_setting("rendering/renderer/rendering_method.vita", "gl_compatibility")
	ProjectSettings.set_setting("display/window/size/viewport_width.vita", 960)
	ProjectSettings.set_setting("display/window/size/viewport_height.vita", 544)
	ProjectSettings.set_setting("threading/worker_pool/max_threads.vita", 2)
	assert(preset.get_project_setting("application/run/main_scene") == "res://main.tscn")
	assert(preset.get_project_setting("rendering/renderer/rendering_method") == "gl_compatibility")
	platform._has_valid_project_configuration(preset)
	assert(platform.get_config_error() != "Set a main scene before exporting.")
	assert(platform.get_config_error() != "Select the Compatibility renderer for this Vita runtime.")
	assert(not platform._safe_path("../outside"))
	assert(not platform._safe_path("C:/outside"))
	assert(platform._safe_path(".godot/imported/texture.ctex"))
	var sfo: PackedByteArray = platform.make_sfo("Jogo ç", "TEST40001", "02.03")
	assert(sfo.decode_u32(0) == 0x46535000)
	assert(sfo.decode_u32(16) == 11)
	print("PASS: Vita preset validation, debug dialog compatibility, feature overrides, path validation and SFO header")
	quit()
