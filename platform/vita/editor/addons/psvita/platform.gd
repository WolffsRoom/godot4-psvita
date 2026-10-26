@tool
extends EditorExportPlatformExtension

const TrophyTools = preload("trophy_tools.gd")

const TEMPLATE = "res://addons/psvita/templates/vita_release.zip"
const DEFAULT_DIR = "res://addons/psvita/defaults"
const LIVEAREA_STYLES = ["a5", "a1", "a2", "a3", "a4", "ad0", "nsx1", "ps1emu", "psmobile", "pspemu", "vd", "browser", "content_manager", "kids", "music"]
const ASSETS = {
	"assets/icon_128x128": ["sce_sys/icon0.png", Vector2i(128, 128)],
	"assets/background_840x500": ["sce_sys/livearea/contents/bg0.png", Vector2i(840, 500)],
	"assets/startup_280x158": ["sce_sys/livearea/contents/startup.png", Vector2i(280, 158)],
}
var logo: Texture2D

func _get_name() -> String:
	return "PS Vita"

func _get_os_name() -> String:
	return "Vita"

func _get_logo() -> Texture2D:
	if logo == null:
		var img := Image.load_from_file("res://addons/psvita/iconGodotPSV.png")
		if img == null:
			return null
		var size := maxi(32, roundi(32 * EditorInterface.get_editor_scale()))
		img.resize(size, size, Image.INTERPOLATE_LANCZOS)
		logo = ImageTexture.create_from_image(img)
	return logo

func _get_platform_features() -> PackedStringArray:
	return PackedStringArray(["Vita", "mobile", "arm32"])

func _get_preset_features(_preset: EditorExportPreset) -> PackedStringArray:
	# Uncompressed/lossless imports: this renderer uploads CPU RGBA images.
	return PackedStringArray()

func _get_binary_extensions(_preset: EditorExportPreset) -> PackedStringArray:
	return PackedStringArray(["vpk"])

func _get_export_options() -> Array[Dictionary]:
	var options: Array[Dictionary] = [
		{"name": "data/separate_folder", "type": TYPE_BOOL, "default_value": true},
		{"name": "data/folder_name", "type": TYPE_STRING, "default_value": "godotvitagame"},
		{"name": "performance/capunlocker", "type": TYPE_STRING, "hint": PROPERTY_HINT_ENUM, "hint_string": "Desativado:disabled,Opcional:optional,Obrigatório:required", "default_value": "optional"},
		{"name": "textures/gpu_format", "type": TYPE_STRING, "hint": PROPERTY_HINT_ENUM, "hint_string": "RGBA4444,RGBA8888", "default_value": "RGBA4444"},
		{"name": "livearea/style", "type": TYPE_STRING, "hint": PROPERTY_HINT_ENUM, "hint_string": ",".join(LIVEAREA_STYLES), "default_value": "a5"},
		{"name": "application/title", "type": TYPE_STRING, "default_value": "Godot Vita Game"},
		{"name": "application/title_id", "type": TYPE_STRING, "default_value": "GDOT40001"},
		{"name": "application/version", "type": TYPE_STRING, "default_value": "01.00"},
		{"name": "custom_template/release", "type": TYPE_STRING, "hint": PROPERTY_HINT_GLOBAL_FILE, "hint_string": "*.zip", "default_value": ""},
	]
	for key in ASSETS:
		options.append({"name": key, "type": TYPE_STRING, "hint": PROPERTY_HINT_FILE, "hint_string": "*.png", "default_value": ""})
	return options

func _template_path(preset: EditorExportPreset) -> String:
	var path: String = preset.get("custom_template/release")
	return TEMPLATE if path.is_empty() else ResourceUID.ensure_path(path)

static func default_asset_path(key: String) -> String:
	return DEFAULT_DIR.path_join(str(ASSETS[key][0]).get_file())

static func validate_asset(path: String, size: Vector2i) -> String:
	path = ResourceUID.ensure_path(path)
	var data := FileAccess.get_file_as_bytes(path)
	if path.get_extension().to_lower() != "png" or data.size() < 33 or data.slice(0, 8) != PackedByteArray([137, 80, 78, 71, 13, 10, 26, 10]):
		return "Arquivo PNG inválido: " + path
	# PNG-8 means indexed palette, not RGB/RGBA with 8 bits per channel.
	if data.slice(12, 16).get_string_from_ascii() != "IHDR" or data[24] != 8 or data[25] != 3:
		return "Use PNG indexado de 8 bits (PNG-8): " + path
	var img := Image.new()
	if img.load_png_from_buffer(data) != OK or img.get_size() != size:
		return "Dimensões PNG esperadas: %s — %s" % [size, path]
	return ""

func _asset_path(preset: EditorExportPreset, key: String) -> String:
	var path: String = preset.get(key)
	return default_asset_path(key) if path.is_empty() else ResourceUID.ensure_path(path)

static func valid_data_folder(folder: String) -> bool:
	var regex := RegEx.new()
	regex.compile("\\A[A-Za-z0-9_-]{1,64}\\z")
	if regex.search(folder) == null:
		return false
	return folder.to_upper() not in ["CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"]

func _configuration_error(preset: EditorExportPreset, _debug: bool) -> String:
	var trophies := TrophyTools.read_config()
	if trophies.get("enabled", false):
		var trophy_error := TrophyTools.validate(trophies)
		if not trophy_error.is_empty():
			return "Troféus: " + trophy_error
	if preset.get("data/separate_folder") and not valid_data_folder(str(preset.get("data/folder_name"))):
		return "Nome da pasta de dados: use 1 a 64 letras, números, hífen ou sublinhado; não use nomes reservados do Windows."
	if str(preset.get("textures/gpu_format")) not in ["RGBA4444", "RGBA8888"]:
		return "Selecione RGBA4444 ou RGBA8888 para as texturas da GPU."
	if str(preset.get("performance/capunlocker")) not in ["disabled", "optional", "required"]:
		return "Selecione um modo válido para o CapUnlocker."
	if str(preset.get("livearea/style")) not in LIVEAREA_STYLES:
		return "Selecione um modelo LiveArea válido."
	var engine := Engine.get_version_info()
	if engine.major != 4 or engine.minor != 8 or engine.status != "dev7":
		return "This template requires Godot 4.8-dev7 (matching resource format)."
	var regex := RegEx.new()
	regex.compile("^[A-Z0-9]{9}$")
	if regex.search(str(preset.get("application/title_id"))) == null:
		return "Title ID must contain exactly 9 uppercase letters or digits."
	regex.compile("^[0-9]{2}\\.[0-9]{2}$")
	if regex.search(str(preset.get("application/version"))) == null:
		return "Version must use NN.NN, for example 01.00."
	var title: String = preset.get("application/title")
	if title.is_empty() or title.to_utf8_buffer().has(0) or title.to_utf8_buffer().size() > 127:
		return "Application title must contain 1 to 127 UTF-8 bytes."
	if not FileAccess.file_exists(_template_path(preset)):
		return "Install the Vita runtime template or select Custom Template / Release."
	for key in ASSETS:
		var asset_error := validate_asset(_asset_path(preset, key), ASSETS[key][1])
		if not asset_error.is_empty():
			return asset_error
	return ""

static func make_livearea_xml(style: String) -> PackedByteArray:
	if style not in LIVEAREA_STYLES:
		return PackedByteArray()
	return ("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
		"<livearea style=\"%s\" format-ver=\"01.00\" content-rev=\"1\">\n" % style +
		"  <livearea-background><image>bg0.png</image></livearea-background>\n" +
		"  <gate><startup-image>startup.png</startup-image></gate>\n" +
		"</livearea>\n").to_utf8_buffer()

func _has_valid_export_configuration(preset: EditorExportPreset, debug: bool) -> bool:
	var error := _configuration_error(preset, debug)
	set_config_error(error)
	set_config_missing_templates(not FileAccess.file_exists(_template_path(preset)))
	return error.is_empty()

func _has_valid_project_configuration(preset: EditorExportPreset) -> bool:
	var error := ""
	if ProjectSettings.get_setting("application/run/main_scene", "") == "":
		error = "Set a main scene before exporting."
	elif preset.get_project_setting("rendering/renderer/rendering_method") != "gl_compatibility":
		error = "Select the Compatibility renderer for this Vita runtime."
	elif preset.get_project_setting("display/window/size/viewport_width") != 960 or preset.get_project_setting("display/window/size/viewport_height") != 544:
		error = "Set the viewport to 960 x 544 for the current Vita renderer."
	elif preset.get_project_setting("threading/worker_pool/max_threads") not in [1, 2]:
		error = "Set Threading / Worker Pool / Max Threads to 2 for Vita."
	elif preset.get_encrypt_pck() or preset.get_encrypt_directory():
		error = "Encrypted resources are not supported by this Vita template."
	elif preset.get_script_export_mode() != 0:
		error = "Select Script export mode: Text (development runtime compatibility)."
	set_config_error(error)
	return error.is_empty()

func _get_export_option_warning(_preset: EditorExportPreset, option: StringName) -> String:
	if option == &"data/separate_folder":
		return "Ativado: gera VPK + pasta de dados para ux0:data/. Desativado: inclui todos os dados no VPK."
	if option == &"data/folder_name":
		return "Copie esta pasta inteira para ux0:data/ no Vita. Use um nome exclusivo para cada jogo."
	if option == &"textures/gpu_format" and _preset.get(option) == "RGBA4444":
		return "16 bits por pixel na GPU; reduz precisão de cores e alpha. Cópias em RAM não mudam. Economia depende do alinhamento das alocações."
	if option == &"performance/capunlocker":
		return "Opcional continua sem o plugin. Obrigatório interrompe a abertura e solicita a instalação. Quando detectado, o áudio usa o quarto núcleo."
	if option == &"livearea/style" and str(_preset.get(option)) not in ["a1", "a2", "a3", "a4", "a5"]:
		return "Modelo especial do sistema/emulador: compatibilidade com aplicativos homebrew ainda não validada."
	if option == &"application/title":
		return "Experimental GXM 2D runtime. 3D, custom shaders and GPU particles are not supported."
	return ""

func _fail(message: String, error: int = ERR_INVALID_DATA) -> int:
	add_message(EXPORT_MESSAGE_ERROR, "PS Vita", message)
	return error

static func _safe_path(path: String) -> bool:
	return not path.is_empty() and not path.begins_with("/") and not path.contains(":") and not path.contains("\\") and not ".." in path.split("/")

# Vita PARAM.SFO: sorted keys, little-endian entries, padded value capacities.
static func make_sfo(title: String, title_id: String, version: String) -> PackedByteArray:
	var values := {"APP_VER": version, "ATTRIBUTE": 0x8000, "ATTRIBUTE2": 12, "CATEGORY": "gd", "PARENTAL_LEVEL": 1, "PSP2_DISP_VER": "01.000", "PSP2_SYSTEM_VER": 0, "STITLE": title.substr(0, 12), "TITLE": title, "TITLE_ID": title_id, "VERSION": version}
	var keys := PackedByteArray()
	var data := PackedByteArray()
	var entries := PackedByteArray()
	for key in values:
		var entry := PackedByteArray()
		entry.resize(16)
		entry.encode_u16(0, keys.size())
		keys.append_array(str(key).to_utf8_buffer())
		keys.append(0)
		var value := PackedByteArray()
		if values[key] is int:
			entry.encode_u16(2, 0x0404)
			value.resize(4)
			value.encode_u32(0, values[key])
		else:
			entry.encode_u16(2, 0x0204)
			value = str(values[key]).to_utf8_buffer()
			value.append(0)
		entry.encode_u32(4, value.size())
		var capacity := (value.size() + 3) & ~3
		entry.encode_u32(8, capacity)
		entry.encode_u32(12, data.size())
		value.resize(capacity)
		data.append_array(value)
		entries.append_array(entry)
	var header := PackedByteArray()
	header.resize(20)
	header.encode_u32(0, 0x46535000)
	header.encode_u32(4, 0x00000101)
	header.encode_u32(8, 20 + entries.size())
	keys.resize((keys.size() + 3) & ~3)
	header.encode_u32(12, 20 + entries.size() + keys.size())
	header.encode_u32(16, values.size())
	header.append_array(entries)
	header.append_array(keys)
	header.append_array(data)
	return header

func _put(zip: ZIPPacker, path: String, data: PackedByteArray) -> int:
	var error := zip.start_file(path)
	if error != OK:
		return error
	error = zip.write_file(data)
	var close_error := zip.close_file()
	return error if error != OK else close_error

func _export_project(preset: EditorExportPreset, debug: bool, path: String, _flags: int) -> int:
	if not _has_valid_export_configuration(preset, debug) or not _has_valid_project_configuration(preset):
		return _fail(get_config_error())
	var trophy_package := {}
	var trophy_config := TrophyTools.read_config()
	if trophy_config.get("enabled", false):
		trophy_package = TrophyTools.build(trophy_config)
		if trophy_package.has("error"):
			return _fail("Troféus: " + str(trophy_package.error))
	if debug:
		add_message(EXPORT_MESSAGE_INFO, "PS Vita", "Release export selected: this runtime has no debug template or remote debugger.")
	path = path.get_basename() + ".vpk" if path.get_extension().to_lower() in ["vpk", "zip", "pck"] else path + ".vpk"
	var separate: bool = preset.get("data/separate_folder")
	var folder: String = preset.get("data/folder_name")
	var destination := ProjectSettings.globalize_path(path)
	var data_destination := destination.get_base_dir().path_join(folder)
	var owner := str(preset.get("application/title_id"))
	if separate and (FileAccess.file_exists(data_destination) or (DirAccess.dir_exists_absolute(data_destination) and FileAccess.get_file_as_string(data_destination.path_join(".vita-export-owner")) != owner)):
		return _fail("A pasta de destino já existe e não pertence a este preset: " + data_destination)
	var template := ZIPReader.new()
	if template.open(_template_path(preset)) != OK:
		return _fail("Cannot open the runtime template.")
	var files := template.get_files()
	for key in ASSETS:
		if not ASSETS[key][0] in files:
			template.close()
			return _fail("Template is missing " + str(ASSETS[key][0]))
	for required in ["eboot.bin", "libgodot.so", "gxm2d.cfg", "vita-template.json", "sce_sys/livearea/contents/template.xml"]:
		if not required in files:
			template.close()
			return _fail("Template is missing " + required)
	var manifest = JSON.parse_string(template.read_file("vita-template.json").get_string_from_utf8())
	if not manifest is Dictionary or manifest.get("format") != 1 or manifest.get("godot") != "4.8-dev7":
		template.close()
		return _fail("Unsupported Vita template version.")
	if separate and not manifest.get("external_game_data", false):
		template.close()
		return _fail("Atualize o template para usar dados separados, ou desative Data / Separate Folder.")
	if preset.get("textures/gpu_format") not in manifest.get("texture_formats", ["RGBA8888"]):
		template.close()
		return _fail("Este template não suporta o formato selecionado. Atualize o template ou selecione RGBA8888.")
	for binary in ["eboot.bin", "libgodot.so"]:
		var hashing := HashingContext.new()
		hashing.start(HashingContext.HASH_SHA256)
		hashing.update(template.read_file(binary))
		if hashing.finish().hex_encode() != manifest.get("sha256", {}).get(binary, ""):
			template.close()
			return _fail("Template integrity check failed: " + binary)
	var temp := ProjectSettings.globalize_path("res://.godot/vita-export-%s.zip" % Time.get_ticks_usec())
	# The generic save dialog defaults to Debug; this platform ships Release only.
	var result := save_zip(preset, false, temp)
	if result.get("result", FAILED) != OK or not result.get("so_files", []).is_empty():
		template.close()
		DirAccess.remove_absolute(temp)
		return _fail("Resource export failed or project contains unsupported native extensions.")
	var game := ZIPReader.new()
	if game.open(temp) != OK:
		template.close()
		DirAccess.remove_absolute(temp)
		return _fail("Cannot open exported resources.")
	var staging := destination + ".partial-%s" % Time.get_ticks_usec()
	var data_staging := data_destination + ".partial-%s" % Time.get_ticks_usec()
	var zip := ZIPPacker.new()
	var error := zip.open(staging)
	if error == OK:
		for name in files:
			if not _safe_path(name) or name.begins_with("game_data/"):
				error = ERR_INVALID_DATA
				break
			if name.ends_with("/") or name in ["sce_sys/param.sfo", "gxm_texture_format.cfg", "game_data_folder.cfg", "capunlocker.cfg"]:
				continue
			var replacement := ""
			for key in ASSETS:
				if ASSETS[key][0] == name:
					replacement = _asset_path(preset, key)
			var bytes := template.read_file(name) if replacement.is_empty() else FileAccess.get_file_as_bytes(replacement)
			if name == "sce_sys/livearea/contents/template.xml":
				bytes = make_livearea_xml(preset.get("livearea/style"))
			error = _put(zip, name, bytes)
			if error != OK:
				break
		if error == OK:
			error = _put(zip, "gxm_texture_format.cfg", (str(preset.get("textures/gpu_format")) + "\n").to_utf8_buffer())
		if error == OK:
			error = _put(zip, "capunlocker.cfg", (str(preset.get("performance/capunlocker")) + "\n").to_utf8_buffer())
		if error == OK:
			error = _put(zip, "sce_sys/param.sfo", make_sfo(preset.get("application/title"), preset.get("application/title_id"), preset.get("application/version")))
		if error == OK and separate:
			error = _put(zip, "game_data_folder.cfg", folder.to_utf8_buffer())
			if error == OK:
				error = _write_data(data_staging.path_join(".vita-export-owner"), owner.to_utf8_buffer())
		if error == OK and not trophy_package.is_empty():
			error = _put(zip, trophy_package.vpk_path, trophy_package.trp)
		if error == OK:
			for name in game.get_files():
				if not _safe_path(name):
					error = ERR_INVALID_DATA
					break
				if name.ends_with("/") or name.begins_with("addons/psvita/") or name.begins_with("vita_trophies/"):
					continue
				error = _write_data(data_staging.path_join("game_data").path_join(name), game.read_file(name)) if separate else _put(zip, "game_data/" + name, game.read_file(name))
				if error != OK:
					break
		if error == OK and separate:
			error = DirAccess.make_dir_recursive_absolute(data_staging.path_join("logs"))
		var close_error := zip.close()
		if error == OK:
			error = close_error
	game.close()
	template.close()
	DirAccess.remove_absolute(temp)
	var previous_data := data_destination + ".previous-%s" % Time.get_ticks_usec()
	var backed_up := false
	var published_data := false
	var previous_vpk := staging + ".previous-vpk"
	var backed_up_vpk := false
	if error == OK and separate:
		if DirAccess.dir_exists_absolute(data_destination):
			error = DirAccess.rename_absolute(data_destination, previous_data)
			backed_up = error == OK
		if error == OK:
			error = DirAccess.rename_absolute(data_staging, data_destination)
			published_data = error == OK
	# Windows DirAccess.rename removes an existing target before moving the source.
	# Move the old VPK aside first so a failed publication can be rolled back.
	if error == OK and FileAccess.file_exists(destination):
		error = DirAccess.rename_absolute(destination, previous_vpk)
		backed_up_vpk = error == OK
	if error == OK:
		error = DirAccess.rename_absolute(staging, destination)
	if error != OK:
		var restored := true
		if published_data:
			restored = DirAccess.rename_absolute(data_destination, data_staging) == OK
		if backed_up:
			restored = (DirAccess.rename_absolute(previous_data, data_destination) == OK) and restored
		if backed_up_vpk:
			restored = (DirAccess.rename_absolute(previous_vpk, destination) == OK) and restored
		DirAccess.remove_absolute(staging)
		if not restored:
			return _fail("Falha ao restaurar a saída. Preserve os backups: " + previous_data + " e " + previous_vpk, error)
		return _fail("VPK export failed; previous output was preserved.", error)
	if backed_up_vpk:
		DirAccess.remove_absolute(previous_vpk)
	add_message(EXPORT_MESSAGE_INFO, "PS Vita", "VPK: " + destination)
	if separate:
		add_message(EXPORT_MESSAGE_INFO, "PS Vita", "Copie a pasta " + data_destination + " para ux0:data/ (destino: ux0:data/" + folder + ").")
	if backed_up:
		add_message(EXPORT_MESSAGE_INFO, "PS Vita", "Dados anteriores preservados em " + previous_data)
	return OK

func _export_zip(preset: EditorExportPreset, _debug: bool, path: String, _flags: int) -> int:
	return _export_project(preset, false, path, _flags)

func _export_pack(preset: EditorExportPreset, _debug: bool, path: String, _flags: int) -> int:
	return _export_project(preset, false, path, _flags)

func _write_data(path: String, bytes: PackedByteArray) -> int:
	var error := DirAccess.make_dir_recursive_absolute(path.get_base_dir())
	if error != OK:
		return error
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		return FileAccess.get_open_error()
	file.store_buffer(bytes)
	file.flush()
	error = file.get_error()
	file.close()
	return error
