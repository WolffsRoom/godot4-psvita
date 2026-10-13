extends Node2D

var elapsed := 0.0
var phase := -1
var animated: Sprite2D
var rotating: Sprite2D
var dynamic: Sprite2D
var status: Label
var dynamic_texture: ImageTexture
var held_textures: Array[Texture2D] = []

func caption(text: String, at: Vector2, font_size: int = 22) -> Label:
	var label := Label.new()
	label.text = text
	label.position = at
	label.add_theme_font_size_override("font_size", font_size)
	add_child(label)
	return label

func panel(at: Vector2, dimensions: Vector2, color: Color, parent: Node = null) -> ColorRect:
	if parent == null:
		parent = self
	var rect := ColorRect.new()
	rect.position = at
	rect.size = dimensions
	rect.color = color
	parent.add_child(rect)
	return rect

func sprite(tex: Texture2D, at: Vector2, parent: Node = null) -> Sprite2D:
	if parent == null:
		parent = self
	var node := Sprite2D.new()
	node.texture = tex
	node.position = at
	node.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	parent.add_child(node)
	return node

func _ready() -> void:
	caption("GXM - TEXTURES / ALPHA / ATLAS / LABEL", Vector2(24, 12), 28)
	var image := Image.create(128, 64, false, Image.FORMAT_RGBA8)
	image.fill(Color(0, 0, 0, 0))
	var colors := [Color(1, 0.2, 0.2), Color(0.2, 1, 0.3), Color(0.2, 0.5, 1), Color(1, 0.8, 0.2)]
	for y in range(64):
		for x in range(128):
			var cell := int(x / 32)
			var u := x % 32
			if u < 3 or u > 28 or y < 4 or y > 59:
				continue
			var color: Color = colors[cell]
			if u < 13 and y < 12:
				color = Color.WHITE
			if y > 32:
				color.a = 0.5
			image.set_pixel(x, y, color)
	var atlas := ImageTexture.create_from_image(image)
	held_textures.append(atlas)
	for i in range(4):
		panel(Vector2(24 + i * 136, 90), Vector2(120, 180), Color(0.15, 0.15, 0.22))
		var node := sprite(atlas, Vector2(84 + i * 136, 175))
		node.region_enabled = true
		node.region_rect = Rect2(i * 32, 0, 32, 64)
		node.scale = Vector2(3, 2)
		node.flip_h = i == 1
		node.flip_v = i == 2
		caption(["NORMAL", "FLIP H", "FLIP V", "LINEAR"][i], Vector2(24 + i * 136, 62), 18)
		if i == 3:
			node.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	caption("CLIP + MOVEMENT", Vector2(605, 62), 20)
	var clipper := panel(Vector2(605, 90), Vector2(320, 180), Color(0.15, 0.2, 0.25))
	clipper.clip_contents = true
	animated = sprite(atlas, Vector2(160, 90), clipper)
	animated.region_enabled = true
	animated.region_rect = Rect2(0, 0, 32, 64)
	animated.scale = Vector2(4, 3)
	animated.modulate = Color(1, 1, 1, 0.65)
	caption("ROTATION + ATLAS TEXTURE", Vector2(24, 292), 20)
	var sub := AtlasTexture.new()
	sub.atlas = atlas
	sub.region = Rect2(64, 0, 32, 64)
	held_textures.append(sub)
	rotating = sprite(sub, Vector2(155, 410))
	rotating.scale = Vector2(2, 2)
	caption("UPDATE / REPLACE", Vector2(340, 292), 20)
	var initial := Image.create(13, 9, false, Image.FORMAT_RGBA8)
	initial.fill(Color.WHITE)
	dynamic_texture = ImageTexture.create_from_image(initial)
	dynamic = sprite(dynamic_texture, Vector2(455, 410))
	dynamic.scale = Vector2(7, 7)
	status = caption("READY", Vector2(605, 320), 22)
	# On-device check: the native fallback must produce real glyph metrics.
	var fallback_font := status.get_theme_font("font")
	if fallback_font != null:
		printerr("[canvas-features] font_height=", fallback_font.get_height(22),
			" glyph_width=", fallback_font.get_string_size("Vita Aa 0123", HORIZONTAL_ALIGNMENT_LEFT, -1, 22).x)
	else:
		printerr("[canvas-features] ERROR: no fallback font")
	caption("Bitmap font: Aa 0123 !?", Vector2(605, 370), 24)
	caption("Observe transparency over panels", Vector2(24, 505), 20)
	printerr("[canvas-features] READY; atlas 128x64; bitmap labels; NPOT texture 13x9")

func _process(delta: float) -> void:
	elapsed += delta
	if animated == null:
		return
	animated.position.x = 160 + sin(elapsed) * 180
	rotating.rotation = elapsed * 0.6
	var next_phase := int(elapsed / 3.0) % 4
	if next_phase == phase:
		return
	phase = next_phase
	animated.region_rect.position.x = phase * 32
	var img := Image.create(13, 9, false, Image.FORMAT_RGBA8)
	img.fill([Color.RED, Color.GREEN, Color.BLUE, Color.YELLOW][phase])
	img.set_pixel(0, 0, Color.WHITE)
	if phase % 2 == 0:
		dynamic_texture.update(img)
	else:
		dynamic_texture.set_image(img)
	status.text = "PHASE %d\nFPS %d" % [phase, Engine.get_frames_per_second()]
	printerr("[canvas-features] phase=", phase, " fps=", Engine.get_frames_per_second(), " update_or_replace=", phase % 2)
