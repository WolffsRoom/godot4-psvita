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
	assert(preset.get("data/package_format") == "PCK")
	assert(preset.get("performance/capunlocker") == "optional")
	assert(preset.get("performance/pck_prefetch") == true)
	assert(preset.get("rendering/shader_compiler") == "disabled")
	assert("vita" in platform._get_platform_features())
	# String enums are stored literally: options must list plain values, and
	# legacy "Label:value" selections from v0.3 presets must still validate
	# and be rewritten (issue #7).
	var options := {}
	for option in platform._get_export_options():
		options[option.name] = option
	assert(options["performance/capunlocker"].hint_string == "disabled,optional,required")
	assert(options["rendering/shader_compiler"].hint_string == "disabled,safe,experimental")
	for mode in ["disabled", "optional", "required"]:
		preset.set("performance/capunlocker", mode)
		assert(platform._has_valid_export_configuration(preset, false))
	for mode in ["disabled", "safe", "experimental"]:
		preset.set("rendering/shader_compiler", mode)
		assert(platform._has_valid_export_configuration(preset, false))
	preset.set("performance/capunlocker", "Obrigatório:required")
	preset.set("rendering/shader_compiler", "Desativado:disabled")
	assert(platform._has_valid_export_configuration(preset, false))
	assert(preset.get("performance/capunlocker") == "required")
	assert(preset.get("rendering/shader_compiler") == "disabled")
	preset.set("performance/capunlocker", "always")
	assert(not platform._has_valid_export_configuration(preset, false))
	preset.set("performance/capunlocker", "optional")
	for folder in ["meujogovita", "My_Game-2", "a".repeat(64)]:
		assert(platform.valid_data_folder(folder))
	for folder in ["", "../outside", "ux0:data/game", "CON", "nul", "COM1", "a".repeat(65), "game\n", "game\\other", "game/other", "jogo ç"]:
		assert(not platform.valid_data_folder(folder))
	assert(platform._has_valid_export_configuration(preset, false))
	assert(platform._has_valid_export_configuration(preset, true))
	print("PASS: native module, a5, RGBA4444, CapUnlocker optional, plain enum values with legacy migration, PCK and split-data defaults, folder validation, template and PNG8")
	preset = null
	platform = null
	quit()
