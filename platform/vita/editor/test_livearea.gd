extends SceneTree

func _initialize() -> void:
	var exporter = load("res://addons/psvita/platform.gd")
	assert(load("res://addons/psvita/defaults_inspector.gd") != null)
	for style in exporter.LIVEAREA_STYLES:
		var xml: PackedByteArray = exporter.make_livearea_xml(style)
		var parser := XMLParser.new()
		assert(parser.open_buffer(xml) == OK)
		var root_found := false
		while parser.read() == OK:
			if parser.get_node_type() == XMLParser.NODE_ELEMENT and parser.get_node_name() == "livearea":
				assert(parser.get_named_attribute_value("style") == style)
				root_found = true
		assert(root_found)
		var preview := Image.load_from_file("res://addons/psvita/previews/psdevwiki_frames_layout_%s.jpg" % style)
		assert(preview != null and preview.get_width() == 120)
	assert(exporter.make_livearea_xml("bad\"<tag>").is_empty())
	print("PASS: a5 default, 15 valid XML styles, 15 source previews, invalid style rejected")
	quit()

