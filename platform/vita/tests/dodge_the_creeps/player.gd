extends Area2D

signal hit

@export var speed = 400 # How fast the player will move (pixels/sec).
var screen_size # Size of the game window.
# The original GPUParticles2D trail is unsupported by the native GXM canvas.
# Render five short-lived translucent sprites instead (texture-quad path).
var vita_trail_cooldown := 0.0
var vita_trail_instances: Array[Dictionary] = []
const VITA_TRAIL_INTERVAL := 0.085
const VITA_TRAIL_LIFETIME := 0.38
const VITA_TRAIL_MAX := 6

func _spawn_vita_trail() -> void:
	if vita_trail_instances.size() >= VITA_TRAIL_MAX:
		return
	var anim: AnimatedSprite2D = $AnimatedSprite2D
	if anim.sprite_frames == null or anim.sprite_frames.get_frame_count(anim.animation) < 1:
		return
	var ghost := Sprite2D.new()
	ghost.texture = anim.sprite_frames.get_frame_texture(anim.animation, anim.frame)
	ghost.flip_h = anim.flip_h
	ghost.flip_v = anim.flip_v
	ghost.z_as_relative = false
	ghost.z_index = 1
	ghost.modulate = Color(0.7, 0.85, 1.0, 0.38)
	get_parent().add_child(ghost)
	ghost.global_transform = anim.global_transform
	vita_trail_instances.append({"node": ghost, "remaining": VITA_TRAIL_LIFETIME})

func _update_vita_trail(delta: float, moving: bool) -> void:
	for i in range(vita_trail_instances.size() - 1, -1, -1):
		var entry: Dictionary = vita_trail_instances[i]
		entry["remaining"] = float(entry["remaining"]) - delta
		var ghost: Sprite2D = entry["node"]
		if float(entry["remaining"]) <= 0.0:
			ghost.queue_free()
			vita_trail_instances.remove_at(i)
		else:
			ghost.modulate.a = 0.38 * float(entry["remaining"]) / VITA_TRAIL_LIFETIME
	vita_trail_cooldown -= delta
	if moving and visible and vita_trail_cooldown <= 0.0:
		_spawn_vita_trail()
		vita_trail_cooldown = VITA_TRAIL_INTERVAL

func _clear_vita_trail() -> void:
	for entry in vita_trail_instances:
		var ghost: Sprite2D = entry["node"]
		if is_instance_valid(ghost):
			ghost.queue_free()
	vita_trail_instances.clear()
	vita_trail_cooldown = 0.0


func _ready():
	screen_size = get_viewport_rect().size
	hide()


func _process(delta):
	var velocity = Vector2.ZERO # The player's movement vector.
	if Input.is_action_pressed(&"move_right"):
		velocity.x += 1
	if Input.is_action_pressed(&"move_left"):
		velocity.x -= 1
	if Input.is_action_pressed(&"move_down"):
		velocity.y += 1
	if Input.is_action_pressed(&"move_up"):
		velocity.y -= 1

	if velocity.length() > 0:
		velocity = velocity.normalized() * speed
		$AnimatedSprite2D.play()
	else:
		$AnimatedSprite2D.stop()

	position += velocity * delta
	position = position.clamp(Vector2.ZERO, screen_size)

	if velocity.x != 0:
		rotation = 0
		$AnimatedSprite2D.animation = &"right"
		$AnimatedSprite2D.flip_v = false
		$AnimatedSprite2D.flip_h = velocity.x < 0
	elif velocity.y != 0:
		$AnimatedSprite2D.animation = &"up"
		rotation = PI if velocity.y > 0 else 0

	_update_vita_trail(delta, velocity.length_squared() > 0.0)


func start(pos):
	_clear_vita_trail()
	position = pos
	rotation = 0
	show()
	$CollisionShape2D.disabled = false


func _on_body_entered(_body):
	_clear_vita_trail()
	hide() # Player disappears after being hit.
	hit.emit()
	# Must be deferred as we can't change physics properties on a physics callback.
	$CollisionShape2D.set_deferred(&"disabled", true)
