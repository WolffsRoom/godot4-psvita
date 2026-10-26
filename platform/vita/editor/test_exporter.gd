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
	assert(not platform._safe_path("../outside"))
	assert(not platform._safe_path("C:/outside"))
	assert(platform._safe_path(".godot/imported/texture.ctex"))
	var sfo: PackedByteArray = platform.make_sfo("Jogo ç", "TEST40001", "02.03")
	assert(sfo.decode_u32(0) == 0x46535000)
	assert(sfo.decode_u32(16) == 10)
	print("PASS: Vita preset validation, debug dialog compatibility, path validation and SFO header")
	quit()
