@tool
extends SceneTree

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	assert(ClassDB.class_exists("VitaExportEditorPlugin"))
	assert(ClassDB.class_exists("VitaExportPlatform"))
	var platform = load(OS.get_executable_path().get_base_dir().path_join("vita/platform.gd")).new()
	var preset: EditorExportPreset = platform.create_preset()
	assert(preset.get("livearea/style") == "a5")
	assert(preset.get("textures/gpu_format") == "RGBA4444")
	assert(preset.get("data/separate_folder") == true)
	assert(preset.get("performance/capunlocker") == "optional")
	for folder in ["meujogovita", "My_Game-2", "a".repeat(64)]:
		assert(platform.valid_data_folder(folder))
	for folder in ["", "../outside", "ux0:data/game", "CON", "nul", "COM1", "a".repeat(65), "game\n", "game\\other", "game/other", "jogo ç"]:
		assert(not platform.valid_data_folder(folder))
	assert(platform._has_valid_export_configuration(preset, false))
	assert(platform._has_valid_export_configuration(preset, true))
	print("PASS: native module, a5, RGBA4444, CapUnlocker optional, split-data defaults, folder validation, template and PNG8")
	preset = null
	platform = null
	quit()
