@tool
extends VBoxContainer

const Tools = preload("trophy_tools.gd")
var model := {}
var selected := -1
var syncing := false
var busy := false
var enabled: CheckBox
var comm_id: LineEdit
var title_edit: LineEdit
var detail_edit: LineEdit
var app_id: LineEdit
var language: LineEdit
var api_key: LineEdit
var list: ItemList
var key_edit: LineEdit
var name_edit: LineEdit
var description_edit: TextEdit
var grade: OptionButton
var hidden_box: CheckBox
var icon_label: Label
var status: Label
var picker: FileDialog
var picker_action := ""
var network: HTTPRequest

func _ready() -> void:
	custom_minimum_size = Vector2(700, 320)
	var tabs := TabContainer.new()
	tabs.size_flags_vertical = Control.SIZE_EXPAND_FILL
	add_child(tabs)
	var config := VBoxContainer.new()
	config.name = "Conjunto"
	tabs.add_child(config)
	enabled = CheckBox.new()
	enabled.text = "Incluir troféus no VPK"
	config.add_child(enabled)
	comm_id = _field(config, "Communication ID", "GDOT40001_00")
	title_edit = _field(config, "Título", "Meu jogo")
	detail_edit = _field(config, "Descrição", "Troféus do projeto")
	var set_icon := Button.new()
	set_icon.text = "Escolher ícone do conjunto (320 × 176)"
	set_icon.pressed.connect(func(): _pick("set_icon"))
	config.add_child(set_icon)
	var note := Label.new()
	note.text = "PNG-8 automático. O TRP fica no VPK, mesmo com dados separados.\nNo Vita: NoTrpDrm + integração sceNpTrophy no jogo/runtime.\nEsta aba gera os arquivos; não desbloqueia troféus durante o jogo."
	config.add_child(note)
	var steam := VBoxContainer.new()
	steam.name = "Importar Steam"
	tabs.add_child(steam)
	app_id = _field(steam, "App ID", "Número do jogo na Steam")
	language = _field(steam, "Idioma", "brazilian / english")
	api_key = _field(steam, "Chave Web API Steam", "Usada apenas nesta sessão; não é salva")
	api_key.secret = true
	var import_buttons := HBoxContainer.new()
	steam.add_child(import_buttons)
	_button(import_buttons, "Importar da Steam", _import_online)
	_button(import_buttons, "Importar JSON Steam…", func(): _pick("json"))
	_button(import_buttons, "Baixar ícones Steam", _download_icons)
	var help := Label.new()
	help.text = "JSON: resposta GetSchemaForGame/v2. Importa nomes, descrições e conquistas ocultas.\nNovos troféus começam em Bronze. Reimportar mantém IDs e suas edições.\nA chave é enviada somente à API oficial Steam por HTTPS."
	steam.add_child(help)
	var edit := HBoxContainer.new()
	edit.name = "Troféus"
	tabs.add_child(edit)
	var left := VBoxContainer.new()
	left.custom_minimum_size.x = 240
	edit.add_child(left)
	list = ItemList.new()
	list.size_flags_vertical = Control.SIZE_EXPAND_FILL
	list.item_selected.connect(_select)
	left.add_child(list)
	_button(left, "Adicionar troféu", _add)
	_button(left, "Remover último troféu", _remove_last)
	var right := VBoxContainer.new()
	right.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	edit.add_child(right)
	key_edit = _field(right, "Chave no jogo / Steam", "ACH_WIN")
	name_edit = _field(right, "Nome", "Nome do troféu")
	description_edit = TextEdit.new()
	description_edit.placeholder_text = "Descrição"
	description_edit.custom_minimum_size.y = 60
	right.add_child(description_edit)
	var row := HBoxContainer.new()
	right.add_child(row)
	grade = OptionButton.new()
	for label in ["Bronze", "Prata", "Ouro", "Platina"]:
		grade.add_item(label)
	row.add_child(grade)
	hidden_box = CheckBox.new()
	hidden_box.text = "Oculto"
	row.add_child(hidden_box)
	_button(row, "Escolher ícone…", func(): _pick("icon"))
	icon_label = Label.new()
	icon_label.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	right.add_child(icon_label)
	for field in [key_edit, name_edit]:
		field.text_changed.connect(func(_text): _store_selected())
	description_edit.text_changed.connect(_store_selected)
	grade.item_selected.connect(func(_id): _store_selected())
	hidden_box.toggled.connect(func(_value): _store_selected())
	var actions := HBoxContainer.new()
	add_child(actions)
	_button(actions, "Salvar configuração", _save)
	_button(actions, "Gerar arquivos base + TRP", _generate)
	_button(actions, "Abrir pasta gerada", func(): OS.shell_open(ProjectSettings.globalize_path(Tools.ROOT)))
	status = Label.new()
	status.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	add_child(status)
	picker = FileDialog.new()
	picker.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	picker.access = FileDialog.ACCESS_FILESYSTEM
	picker.file_selected.connect(_picked)
	add_child(picker)
	network = HTTPRequest.new()
	network.timeout = 20
	network.body_size_limit = 8 * 1024 * 1024
	# Keep the API key from ever following a redirect to a different host.
	network.max_redirects = 0
	add_child(network)
	model = Tools.read_config()
	if model.has("error"):
		status.text = model.error
		return
	_load_fields()
	status.text = "Configuração do projeto aberto. Salve antes de exportar."

func _field(parent: Node, label: String, placeholder: String) -> LineEdit:
	var row := HBoxContainer.new()
	parent.add_child(row)
	var caption := Label.new()
	caption.text = label
	caption.custom_minimum_size.x = 180
	row.add_child(caption)
	var field := LineEdit.new()
	field.placeholder_text = placeholder
	field.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(field)
	return field

func _button(parent: Node, text: String, action: Callable) -> void:
	var button := Button.new()
	button.text = text
	button.pressed.connect(action)
	parent.add_child(button)

func _load_fields() -> void:
	enabled.button_pressed = model.get("enabled", false)
	comm_id.text = model.get("communication_id", "GDOT40001_00")
	title_edit.text = model.get("title", "")
	detail_edit.text = model.get("description", "")
	app_id.text = str(model.get("steam_app_id", ""))
	language.text = model.get("language", "brazilian")
	_refresh()

func _collect() -> void:
	_store_selected()
	model.enabled = enabled.button_pressed
	model.communication_id = comm_id.text.strip_edges()
	model.title = title_edit.text
	model.description = detail_edit.text
	model.steam_app_id = app_id.text.strip_edges()
	model.language = language.text.strip_edges()

func _refresh() -> void:
	list.clear()
	for item in model.get("trophies", []):
		list.add_item("%03d  [%s] %s" % [int(item.id), item.grade, item.name])
	selected = -1
	if list.item_count > 0:
		list.select(0)
		_select(0)

func _select(index: int) -> void:
	syncing = true
	selected = index
	var item: Dictionary = model.trophies[index]
	key_edit.text = item.key
	name_edit.text = item.name
	description_edit.text = item.get("description", "")
	grade.select(["B", "S", "G", "P"].find(item.grade))
	hidden_box.button_pressed = item.get("hidden", false)
	icon_label.text = item.get("icon", "") if not str(item.get("icon", "")).is_empty() else "Ícone provisório; escolha uma imagem ou baixe da Steam."
	syncing = false

func _store_selected() -> void:
	if syncing or selected < 0 or not model.has("trophies"):
		return
	var item: Dictionary = model.trophies[selected]
	item.key = key_edit.text
	item.name = name_edit.text
	item.description = description_edit.text
	item.grade = ["B", "S", "G", "P"][maxi(0, grade.selected)]
	item.hidden = hidden_box.button_pressed
	list.set_item_text(selected, "%03d  [%s] %s" % [int(item.id), item.grade, item.name])

func _add() -> void:
	if busy or model.has("error"):
		return
	_collect()
	var count: int = model.trophies.size()
	model.trophies.append({"id": count, "key": "ACH_%03d" % count, "name": "Novo troféu", "description": "", "grade": "B", "hidden": false, "icon": ""})
	_refresh()
	list.select(count)
	_select(count)

func _remove_last() -> void:
	if busy or model.get("trophies", []).is_empty():
		return
	model.trophies.pop_back()
	_refresh()
	status.text = "Último troféu removido do rascunho. Não altere IDs de um conjunto já publicado."

func _save() -> void:
	if busy or model.has("error"):
		return
	_collect()
	var error := Tools.save_config(model)
	status.text = "Configuração salva no projeto." if error == OK else error_string(error)

func _generate() -> void:
	if busy or model.has("error"):
		return
	_collect()
	var error := Tools.generate(model)
	status.text = "Arquivos gerados em vita_trophies/. O TRP será incluído ao exportar se a opção estiver marcada." if error.is_empty() else error

func _pick(action: String) -> void:
	if busy:
		return
	if action == "icon" and selected < 0:
		return
	picker_action = action
	picker.filters = PackedStringArray(["*.json ; Steam JSON"]) if action == "json" else PackedStringArray(["*.png,*.jpg,*.jpeg,*.webp ; Imagens"])
	picker.popup_centered_ratio(0.65)

func _picked(path: String) -> void:
	if picker_action == "json":
		_apply_schema(JSON.parse_string(FileAccess.get_file_as_string(path)))
		return
	var image := Image.load_from_file(path)
	if image == null:
		status.text = "Imagem inválida."
		return
	var is_set := picker_action == "set_icon"
	var target := Tools.ROOT + ("/icons/set.png" if is_set else "/icons/trophy_%03d.png" % int(model.trophies[selected].id))
	var error := Tools.write_file(target, Tools.png8(image, Vector2i(320, 176) if is_set else Vector2i(240, 240)))
	if error != OK:
		status.text = error_string(error)
		return
	if is_set:
		model.set_icon = target
	else:
		model.trophies[selected].icon = target
		icon_label.text = target
	status.text = "Ícone copiado para o projeto e convertido em PNG-8. Salve a configuração."

func _apply_schema(schema: Variant) -> void:
	_collect()
	var imported := Tools.import_schema(schema, model)
	if imported.has("error"):
		status.text = imported.error
		return
	model = imported
	_load_fields()
	status.text = "%d troféus no rascunho. Revise as categorias, baixe os ícones e salve." % model.trophies.size()

func _import_online() -> void:
	if busy:
		return
	if not app_id.text.is_valid_int() or int(app_id.text) <= 0 or api_key.text.strip_edges().is_empty():
		status.text = "Informe App ID numérico e chave Web API Steam, ou importe o JSON."
		return
	busy = true
	status.text = "Consultando a Steam…"
	var url := "https://api.steampowered.com/ISteamUserStats/GetSchemaForGame/v2/?appid=%s&l=%s&key=%s" % [app_id.text.uri_encode(), language.text.uri_encode(), api_key.text.strip_edges().uri_encode()]
	var error := network.request(url)
	if error != OK:
		busy = false
		status.text = "Não foi possível iniciar a consulta Steam."
		return
	var response: Array = await network.request_completed
	busy = false
	if response[0] != HTTPRequest.RESULT_SUCCESS or response[1] != 200:
		status.text = "Falha na Steam (HTTP %s). Verifique a chave e a conexão, ou importe o JSON." % response[1]
		return
	_apply_schema(JSON.parse_string(response[3].get_string_from_utf8()))

func _download_icons() -> void:
	if busy or model.has("error"):
		return
	busy = true
	var downloaded := 0
	var failed := 0
	for item in model.trophies:
		if not str(item.get("icon", "")).is_empty():
			continue
		var url := str(item.get("steam_icon", ""))
		var host := url.trim_prefix("https://").get_slice("/", 0).to_lower()
		var allowed := false
		for domain in ["steamstatic.com", "steamusercontent.com", "steamcdn-a.akamaihd.net"]:
			allowed = allowed or host == domain or host.ends_with("." + domain)
		if not url.begins_with("https://") or not allowed:
			failed += 1
			continue
		status.text = "Baixando ícone %03d…" % int(item.id)
		if network.request(url) != OK:
			failed += 1
			continue
		var response: Array = await network.request_completed
		var image := Image.new()
		if response[0] != HTTPRequest.RESULT_SUCCESS or response[1] != 200:
			failed += 1
			continue
		var raw: PackedByteArray = response[3]
		var error := image.load_png_from_buffer(raw)
		if error != OK:
			error = image.load_jpg_from_buffer(raw)
		if error != OK:
			failed += 1
			continue
		var target := Tools.ROOT + "/icons/trophy_%03d.png" % int(item.id)
		if Tools.write_file(target, Tools.png8(image, Vector2i(240, 240))) != OK:
			failed += 1
			continue
		item.icon = target
		downloaded += 1
	busy = false
	_refresh()
	status.text = "%d ícones baixados; %d indisponíveis. Salve a configuração." % [downloaded, failed]
