@tool
extends SceneTree

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	var support := OS.get_executable_path().get_base_dir().path_join("vita")
	var tools = load(support.path_join("trophy_tools.gd"))
	assert(tools != null and tools.can_instantiate())
	var panel_script = load(support.path_join("trophy_panel.gd"))
	assert(panel_script != null and panel_script.can_instantiate())
	var panel = panel_script.new()
	root.add_child(panel)
	var model: Dictionary = tools.defaults()
	model.enabled = true
	model.title = "Teste & Troféus"
	model.communication_id = "TEST40001_00"
	var schema := {"game": {"gameName": "Steam fixture", "availableGameStats": {"achievements": [
		{"name": "ACH_FIRST", "displayName": "Primeiro <passo>", "description": "Conclua a fase & volte.", "hidden": 0, "icon": "https://cdn.akamai.steamstatic.com/test.png"},
		{"name": "ACH_SECRET", "displayName": "Segredo", "description": "Encontre o segredo.", "hidden": 1}
	]}}}
	model = tools.import_schema(schema, model)
	assert(not model.has("error"))
	assert(model.trophies.size() == 2 and model.trophies[1].hidden)
	model.trophies[1].grade = "G"
	model.trophies[0].name = "Minha edição"
	var imported: Dictionary = tools.import_schema(schema, model)
	assert(imported.trophies.size() == 2 and imported.trophies[0].name == "Minha edição" and imported.trophies[1].grade == "G")
	var invalid := model.duplicate(true)
	invalid.communication_id = "../outside"
	assert(not tools.validate(invalid).is_empty())
	invalid = model.duplicate(true)
	invalid.trophies[1].id = 0
	assert(not tools.validate(invalid).is_empty())
	invalid = model.duplicate(true)
	invalid.trophies[1].key = invalid.trophies[0].key
	assert(not tools.validate(invalid).is_empty())
	assert(tools.import_schema({}, model).has("error"))
	assert(tools.generate(model).is_empty())
	var parsed: Dictionary = tools.read_config()
	assert(parsed.trophies.size() == 2 and parsed.enabled)
	var built: Dictionary = tools.build(parsed)
	assert(not built.has("error") and built.mapping.ACH_SECRET == 1)
	assert(built.vpk_path == "sce_sys/trophy/TEST40001_00/TROPHY.TRP")
	assert(built.trp.slice(0, 4) == PackedByteArray([0xdc, 0xa2, 0x4d, 0]))
	var png: PackedByteArray = built.files["TROP000.PNG"]
	assert(png[24] == 8 and png[25] == 3)
	var image := Image.new()
	assert(image.load_png_from_buffer(png) == OK and image.get_size() == Vector2i(240, 240))
	# Confirm transparency also survives the indexed encoder.
	image.fill(Color(1, 0, 0, 0))
	png = tools.png8(image, Vector2i(8, 8))
	assert(image.load_png_from_buffer(png) == OK and image.get_pixel(0, 0).a == 0)
	panel.queue_free()
	print("PASS: trophy UI, Steam schema/merge, validation, PNG8, generated SFM/TRP and mapping")
	quit()
