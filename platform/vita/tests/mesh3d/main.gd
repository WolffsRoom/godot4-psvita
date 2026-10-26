extends Node3D

var spinner: Node3D
var elapsed := 0.0

func make_box(at: Vector3, size: Vector3, color: Color, textured := false) -> MeshInstance3D:
	var instance := MeshInstance3D.new()
	var mesh := BoxMesh.new()
	mesh.size = size
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	material.albedo_color = color
	if textured:
		var image := Image.create(32, 32, false, Image.FORMAT_RGBA8)
		for y in 32:
			for x in 32:
				image.set_pixel(x, y, Color.WHITE if (x / 8 + y / 8) % 2 == 0 else Color(0.15, 0.35, 0.9))
		material.albedo_texture = ImageTexture.create_from_image(image)
	mesh.material = material
	instance.mesh = mesh
	instance.position = at
	add_child(instance)
	return instance

func _ready() -> void:
	var camera := Camera3D.new()
	add_child(camera)
	camera.position = Vector3(0, 2.5, 7)
	camera.look_at(Vector3(0, 0.5, 0))
	camera.current = true
	make_box(Vector3(0, -0.7, 0), Vector3(7, 0.2, 5), Color(0.2, 0.25, 0.3))
	spinner = make_box(Vector3(-1.4, 0.4, 0), Vector3(1.5, 1.5, 1.5), Color.WHITE, true)
	# Front red post must occlude rear green box irrespective of draw order.
	make_box(Vector3(1.4, 0.3, 1), Vector3(0.3, 1.8, 0.3), Color(1, 0.15, 0.1))
	make_box(Vector3(1.4, 0.3, -0.7), Vector3(1.6, 1.6, 1.6), Color(0.1, 0.8, 0.2))
	if ResourceLoader.exists("res://overburden/Fuel_Can_1.glb"):
		var model: Node3D = load("res://overburden/Fuel_Can_1.glb").instantiate()
		add_child(model)
		model.position = Vector3(0, 0.1, 0)
		model.scale = Vector3.ONE * 2.0
		print("[gxm3d-probe] Overburden fuel can loaded")
	print("[gxm3d-probe] READY: checker cube, red front post, green rear box, floor")

func _process(delta: float) -> void:
	elapsed += delta
	spinner.rotation = Vector3(elapsed * 0.3, elapsed * 0.6, 0)
	if Input.is_joy_button_pressed(0, JOY_BUTTON_START):
		get_tree().quit()
