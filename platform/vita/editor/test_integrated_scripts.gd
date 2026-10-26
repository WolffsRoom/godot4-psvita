extends SceneTree

func _initialize() -> void:
	var root := OS.get_cmdline_user_args()[0]
	var platform = load(root.path_join("platform.gd"))
	var inspector = load(root.path_join("defaults_inspector.gd"))
	assert(platform != null and platform.can_instantiate())
	assert(inspector != null and inspector.can_instantiate())
	assert(platform.make_livearea_xml("a5").get_string_from_utf8().contains('style="a5"'))
	print("PASS: integrated support scripts load from outside the project")
	quit()
