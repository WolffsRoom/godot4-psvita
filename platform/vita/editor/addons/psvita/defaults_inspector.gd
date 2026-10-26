@tool
extends EditorInspectorPlugin

const VitaPlatform = preload("platform.gd")
const DEFAULT_DIR = "res://addons/psvita/defaults"

func _can_handle(object: Object) -> bool:
	return object is EditorExportPreset and object.has("custom_template/release") and object.has("assets/icon_128x128")

func _parse_begin(object: Object) -> void:
	var prepare := Button.new()
	prepare.text = "Preparar configurações para PS Vita"
	prepare.tooltip_text = "Configura Compatibility, 960 × 544 e dois workers apenas para a exportação Vita."
	prepare.pressed.connect(func():
		var settings := {
			"rendering/renderer/rendering_method.Vita": "gl_compatibility",
			"rendering/renderer/rendering_method.mobile.Vita": "gl_compatibility",
			"display/window/size/viewport_width.Vita": 960,
			"display/window/size/viewport_height.Vita": 544,
			"threading/worker_pool/max_threads.Vita": 2,
		}
		for key in settings:
			ProjectSettings.set_setting(key, settings[key])
		if ProjectSettings.save() != OK:
			push_error("Não foi possível salvar as configurações Vita.")
		object.notify_property_list_changed())
	add_custom_control(prepare)
	var button := Button.new()
	button.text = "Aplicar padrão PS Vita"
	button.tooltip_text = "Preenche Release, Icon, Background e Startup com o template e as imagens padrão incluídas."
	button.pressed.connect(_apply.bind(object, button))
	add_custom_control(button)
	var reference := LinkButton.new()
	reference.text = "Ver modelos e prévias no PSDevWiki"
	reference.uri = "https://www.psdevwiki.com/vita/Template.xml"
	add_custom_control(reference)
	var previews := VBoxContainer.new()
	var toggle := Button.new()
	toggle.text = "Mostrar prévias dos modelos"
	previews.add_child(toggle)
	var scroll := ScrollContainer.new()
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	scroll.custom_minimum_size.y = 100 * EditorInterface.get_editor_scale()
	scroll.visible = false
	previews.add_child(scroll)
	var row := HBoxContainer.new()
	scroll.add_child(row)
	toggle.pressed.connect(func(): scroll.visible = not scroll.visible)
	for style in VitaPlatform.LIVEAREA_STYLES:
		var choice := Button.new()
		choice.text = style
		choice.tooltip_text = "Selecionar " + style + " — referência PSDevWiki"
		var image := Image.load_from_file("res://addons/psvita/previews/psdevwiki_frames_layout_%s.jpg" % style)
		if image != null:
			choice.icon = ImageTexture.create_from_image(image)
		choice.pressed.connect(func():
			object.set("livearea/style", style)
			object.notify_property_list_changed())
		row.add_child(choice)
	add_custom_control(previews)

static func apply_defaults(preset: EditorExportPreset) -> String:
	# Select the bundled files; never extract over user-supplied defaults.
	if not FileAccess.file_exists(VitaPlatform.TEMPLATE):
		return "Template padrão não encontrado: " + VitaPlatform.TEMPLATE
	for key in VitaPlatform.ASSETS:
		var error := VitaPlatform.validate_asset(VitaPlatform.default_asset_path(key), VitaPlatform.ASSETS[key][1])
		if not error.is_empty():
			return error
	preset.set("custom_template/release", VitaPlatform.TEMPLATE)
	preset.set("livearea/style", "a5")
	for key in VitaPlatform.ASSETS:
		preset.set(key, VitaPlatform.default_asset_path(key))
	preset.notify_property_list_changed()
	return ""

func _apply(preset: EditorExportPreset, button: Button) -> void:
	var error := apply_defaults(preset)
	if error.is_empty():
		EditorInterface.get_resource_filesystem().scan()
		print("PS Vita: padrão aplicado a Release, Icon, Background e Startup.")
	else:
		button.text = "Falha ao aplicar padrão — veja a saída"
		push_error(error)
