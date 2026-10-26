@tool
extends RefCounted

const ROOT = "res://vita_trophies"
const CONFIG = ROOT + "/trophies.json"

static func defaults() -> Dictionary:
	var communication_id := "GDOT40001_00"
	var presets := ConfigFile.new()
	if presets.load("res://export_presets.cfg") == OK:
		for section in presets.get_sections():
			if presets.get_value(section, "platform", "") == "PS Vita":
				communication_id = str(presets.get_value(section + ".options", "application/title_id", "GDOT40001")) + "_00"
				break
	return {"format": 1, "enabled": false, "communication_id": communication_id, "title": str(ProjectSettings.get_setting("application/config/name", "Meu jogo")), "description": "Troféus do projeto", "steam_app_id": "", "language": "brazilian", "set_icon": "", "trophies": []}

static func _xml_text_valid(text: String) -> bool:
	for i in range(text.length()):
		var code := text.unicode_at(i)
		if (code < 32 and code not in [9, 10, 13]) or code in [0xfffe, 0xffff]:
			return false
	return true

static func read_config() -> Dictionary:
	if not FileAccess.file_exists(CONFIG):
		return defaults()
	var parsed = JSON.parse_string(FileAccess.get_file_as_string(CONFIG))
	return parsed if parsed is Dictionary else {"enabled": true, "error": "Configuração de troféus inválida."}

static func validate(model: Dictionary) -> String:
	if model.has("error"):
		return str(model.error)
	var re := RegEx.new()
	re.compile("\\A[A-Z0-9]{9}_00\\z")
	if re.search(str(model.get("communication_id", ""))) == null:
		return "Communication ID: use 9 letras maiúsculas/dígitos seguidos de _00."
	if str(model.get("title", "")).strip_edges().is_empty():
		return "Informe o título do conjunto."
	for field in ["title", "description"]:
		if not _xml_text_valid(str(model.get(field, ""))):
			return "Remova caracteres de controle do título/descrição."
	var entries = model.get("trophies", [])
	if not entries is Array or entries.is_empty() or entries.size() > 128:
		return "O conjunto precisa de 1 a 128 troféus."
	var ids := {}
	var keys := {}
	var platinum := 0
	for item in entries:
		if not item is Dictionary:
			return "Definição de troféu inválida."
		var id := int(item.get("id", -1))
		var key := str(item.get("key", ""))
		if id < 0 or id >= entries.size() or ids.has(id):
			return "IDs devem ser únicos e consecutivos, começando em zero."
		if key.is_empty() or keys.has(key):
			return "Cada troféu precisa de uma chave exclusiva."
		if str(item.get("name", "")).strip_edges().is_empty():
			return "Preencha o nome de todos os troféus."
		if not _xml_text_valid(str(item.name)) or not _xml_text_valid(str(item.get("description", ""))):
			return "Nome/descrição de troféu contém caracteres inválidos para XML."
		if item.get("grade", "") not in ["B", "S", "G", "P"]:
			return "Escolha Bronze, Prata, Ouro ou Platina."
		platinum += 1 if item.grade == "P" else 0
		ids[id] = true
		keys[key] = true
	if platinum > 1:
		return "Use no máximo uma platina por conjunto."
	return ""

# Steam GetSchemaForGame/v2 JSON; reimports preserve IDs and custom edits.
static func import_schema(schema: Variant, current: Dictionary) -> Dictionary:
	if not schema is Dictionary or not schema.get("game") is Dictionary:
		return {"error": "JSON esperado: game.availableGameStats.achievements (GetSchemaForGame/v2)."}
	var stats = schema.game.get("availableGameStats", {})
	if not stats is Dictionary or not stats.get("achievements") is Array or stats.achievements.is_empty():
		return {"error": "A Steam não retornou conquistas para este jogo."}
	var result := current.duplicate(true)
	var entries: Array = result.get("trophies", [])
	var existing := {}
	for item in entries:
		existing[str(item.key)] = true
	for achievement in stats.achievements:
		if not achievement is Dictionary or str(achievement.get("name", "")).is_empty():
			return {"error": "Conquista Steam sem nome interno."}
		var key := str(achievement.name)
		if existing.has(key):
			continue
		entries.append({"id": entries.size(), "key": key, "name": str(achievement.get("displayName", key)), "description": str(achievement.get("description", "")), "grade": "B", "hidden": int(achievement.get("hidden", 0)) != 0, "icon": "", "steam_icon": str(achievement.get("icon", ""))})
		existing[key] = true
	result.trophies = entries
	if result.title.is_empty():
		result.title = str(schema.game.get("gameName", "Meu jogo"))
	var error := validate(result)
	return {"error": error} if not error.is_empty() else result

static func save_config(model: Dictionary) -> Error:
	var error := DirAccess.make_dir_recursive_absolute(ROOT)
	if error != OK:
		return error
	# Keep source artwork, config and generated TRP out of the game-data export.
	error = write_file(ROOT + "/.gdignore", PackedByteArray())
	if error != OK:
		return error
	return write_file(CONFIG, JSON.stringify(model, "\t").to_utf8_buffer())

static func write_file(path: String, bytes: PackedByteArray) -> Error:
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

static func _be(value: int, size: int) -> PackedByteArray:
	var bytes := PackedByteArray()
	for shift in range(size - 1, -1, -1):
		bytes.append((value >> (shift * 8)) & 255)
	return bytes

static func _chunk(kind: String, payload: PackedByteArray) -> PackedByteArray:
	var data := kind.to_ascii_buffer()
	data.append_array(payload)
	var crc: int = 0xffffffff
	for byte in data:
		crc ^= byte
		for bit in range(8):
			crc = (crc >> 1) ^ (0xedb88320 if crc & 1 else 0)
	var out := _be(payload.size(), 4)
	out.append_array(data)
	out.append_array(_be(crc ^ 0xffffffff, 4))
	return out

# PNG-8, 216 RGB colours + transparent entry. No external Python/image tools.
static func png8(image: Image, size: Vector2i) -> PackedByteArray:
	image = image.duplicate()
	image.resize(size.x, size.y, Image.INTERPOLATE_LANCZOS)
	image.convert(Image.FORMAT_RGBA8)
	var rgba := image.get_data()
	var palette := PackedByteArray([0, 0, 0])
	for r in range(6):
		for g in range(6):
			for b in range(6):
				palette.append_array(PackedByteArray([r * 51, g * 51, b * 51]))
	var pixels := PackedByteArray()
	for y in range(size.y):
		pixels.append(0)
		for x in range(size.x):
			var i := (y * size.x + x) * 4
			var index := 0
			if rgba[i + 3] >= 128:
				index = 1 + mini(5, int((rgba[i] + 25) / 51)) * 36 + mini(5, int((rgba[i + 1] + 25) / 51)) * 6 + mini(5, int((rgba[i + 2] + 25) / 51))
			pixels.append(index)
	var header := _be(size.x, 4)
	header.append_array(_be(size.y, 4))
	header.append_array(PackedByteArray([8, 3, 0, 0, 0]))
	var png := PackedByteArray([137, 80, 78, 71, 13, 10, 26, 10])
	png.append_array(_chunk("IHDR", header))
	png.append_array(_chunk("PLTE", palette))
	png.append_array(_chunk("tRNS", PackedByteArray([0])))
	png.append_array(_chunk("IDAT", pixels.compress(FileAccess.COMPRESSION_DEFLATE)))
	png.append_array(_chunk("IEND", PackedByteArray()))
	return png

static func _icon(path: String, size: Vector2i, grade: String) -> PackedByteArray:
	var image := Image.new()
	if not path.is_empty():
		if image.load(path) != OK:
			return PackedByteArray()
	else:
		image = Image.create(size.x, size.y, false, Image.FORMAT_RGBA8)
		image.fill({"B": Color("b88746"), "S": Color("aab4c0"), "G": Color("dbb52c"), "P": Color("75b8d5")}.get(grade, Color("426b90")))
		image.fill_rect(Rect2i(size.x / 3, size.y / 4, size.x / 3, size.y / 3), Color.WHITE)
		image.fill_rect(Rect2i(size.x * 5 / 12, size.y / 2, size.x / 6, size.y / 4), Color.WHITE)
		image.fill_rect(Rect2i(size.x / 3, size.y * 3 / 4, size.x / 3, size.y / 12), Color.WHITE)
	return png8(image, size)

static func pack_trp(files: Dictionary) -> PackedByteArray:
	var count := files.size()
	var table := PackedByteArray()
	table.resize(64 + count * 64)
	var blob := PackedByteArray()
	var i := 0
	for name in files:
		while blob.size() % 16:
			blob.append(0)
		var entry := 64 + i * 64
		var name_bytes: PackedByteArray = str(name).to_ascii_buffer()
		for j in range(name_bytes.size()):
			table[entry + j] = name_bytes[j]
		var offset := _be(table.size() + blob.size(), 8)
		var length := _be(files[name].size(), 8)
		for j in range(8):
			table[entry + 32 + j] = offset[j]
			table[entry + 40 + j] = length[j]
		blob.append_array(files[name])
		i += 1
	var header := PackedByteArray([0xdc, 0xa2, 0x4d, 0])
	header.append_array(_be(2, 4))
	header.append_array(_be(table.size() + blob.size(), 8))
	header.append_array(_be(count, 4))
	header.append_array(_be(64, 4))
	for j in range(header.size()):
		table[j] = header[j]
	table.append_array(blob)
	var hash := HashingContext.new()
	hash.start(HashingContext.HASH_SHA1)
	hash.update(table)
	var digest := hash.finish()
	for j in range(20):
		table[28 + j] = digest[j]
	return table

static func build(model: Dictionary) -> Dictionary:
	var error := validate(model)
	if not error.is_empty():
		return {"error": error}
	var prefix := "<!--Sce-Np-Trophy-Signature: " + "x".repeat(320) + "-->\n"
	prefix += '<trophyconf version="1.1" platform="psp2" policy="large">\n'
	prefix += " <npcommid>%s</npcommid>\n <trophyset-version>01.00</trophyset-version>\n <parental-level license-area=\"default\">0</parental-level>\n" % model.communication_id
	var conf := prefix
	var localized := prefix + " <title-name>%s</title-name>\n <title-detail>%s</title-detail>\n" % [str(model.title).xml_escape(), str(model.get("description", "")).xml_escape()]
	var platinum := -1
	for item in model.trophies:
		if item.grade == "P":
			platinum = int(item.id)
	var icons := {}
	var mapping := {}
	for item in model.trophies:
		var attrs := 'id="%03d" hidden="%s" ttype="%s" pid="%s"' % [int(item.id), "yes" if item.get("hidden", false) else "no", item.grade, "%03d" % platinum if platinum >= 0 and item.grade != "P" else "-1"]
		conf += " <trophy %s/>\n" % attrs
		localized += " <trophy %s><name>%s</name><detail>%s</detail></trophy>\n" % [attrs, str(item.name).xml_escape(), str(item.get("description", "")).xml_escape()]
		var png := _icon(str(item.get("icon", "")), Vector2i(240, 240), item.grade)
		if png.is_empty():
			return {"error": "Não foi possível ler o ícone do troféu %s." % item.id}
		icons["TROP%03d.PNG" % int(item.id)] = png
		mapping[str(item.key)] = int(item.id)
	var set_icon := _icon(str(model.get("set_icon", "")), Vector2i(320, 176), "")
	if set_icon.is_empty():
		return {"error": "Não foi possível ler o ícone do conjunto."}
	var files := {"TROPCONF.SFM": (conf + "</trophyconf>\n").to_utf8_buffer(), "TROP.SFM": (localized + "</trophyconf>\n").to_utf8_buffer(), "ICON0.PNG": set_icon}
	files.merge(icons)
	return {"files": files, "trp": pack_trp(files), "mapping": mapping, "vpk_path": "sce_sys/trophy/%s/TROPHY.TRP" % model.communication_id}

static func generate(model: Dictionary) -> String:
	var built := build(model)
	if built.has("error"):
		return str(built.error)
	var error := save_config(model)
	if error != OK:
		return error_string(error)
	for name in built.files:
		error = write_file(ROOT.path_join("source").path_join(name), built.files[name])
		if error != OK:
			return error_string(error)
	error = write_file(ROOT.path_join("vpk_inject").path_join(built.vpk_path), built.trp)
	if error != OK:
		return error_string(error)
	error = write_file(ROOT + "/achievement_to_trophy.json", JSON.stringify(built.mapping, "\t").to_utf8_buffer())
	if error != OK:
		return error_string(error)
	var guide: String = "# Troféus PS Vita\n\nDefinições editáveis: trophies.json.\nArte e XML gerados: source/.\nPacote: vpk_inject/" + built.vpk_path + ".\nMapa de chaves Steam para IDs Vita: achievement_to_trophy.json.\n\nO editor inclui o TRP no VPK quando Incluir troféus no VPK está marcado.\nOs arquivos fonte desta pasta não são incluídos nos dados do jogo.\nA API key Steam não é salva. Bronze é a categoria inicial; revise os tipos.\n\nEsses arquivos não desbloqueiam conquistas sozinhos. O jogo precisa chamar\nsceNpTrophy pelo runtime/ponte nativa, usando os IDs do mapa. A ponte de\ndesbloqueio ainda não está implementada neste runtime Godot. NoTrpDrm deve\nestar configurado no aparelho conforme seu README. Valide no Vita antes\nde publicar e não altere IDs de um conjunto já registrado.\n\nReferências:\nhttps://github.com/Rinnegatamante/NoTrpDrm\nhttps://partner.steamgames.com/doc/webapi/ISteamUserStats#GetSchemaForGame\n"
	error = write_file(ROOT + "/LEIA-ME.md", guide.to_utf8_buffer())
	if error != OK:
		return error_string(error)
	return ""

