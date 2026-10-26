extends SceneTree

func _initialize() -> void:
	var exporter = load("res://addons/psvita/platform.gd")
	assert(load("res://addons/psvita/defaults_inspector.gd") != null)
	for key in exporter.ASSETS:
		var path: String = exporter.default_asset_path(key)
		assert(exporter.validate_asset(path, exporter.ASSETS[key][1]).is_empty())
		var uid := ResourceUID.create_id()
		ResourceUID.add_id(uid, path)
		assert(exporter.validate_asset(ResourceUID.id_to_text(uid), exporter.ASSETS[key][1]).is_empty())
		ResourceUID.remove_id(uid)
		assert(not exporter.validate_asset(path, Vector2i(1, 1)).is_empty())
		var data := FileAccess.get_file_as_bytes(path)
		for format in [Vector2i(8, 6), Vector2i(8, 2), Vector2i(1, 3), Vector2i(16, 6)]:
			var fixture := data.duplicate()
			fixture[24] = format.x
			fixture[25] = format.y
			var temp := "res://.godot/png8-negative.png"
			var file := FileAccess.open(temp, FileAccess.WRITE)
			file.store_buffer(fixture)
			file.close()
			assert(exporter.validate_asset(temp, exporter.ASSETS[key][1]).contains("PNG-8"))
			DirAccess.remove_absolute(ProjectSettings.globalize_path(temp))
	print("PASS: all supplied PNG-8 defaults; wrong sizes, RGB/RGBA, 1-bit and 16-bit rejected")
	quit()
