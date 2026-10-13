extends RigidBody2D

func _ready():
	var mob_types = Array($AnimatedSprite2D.sprite_frames.get_animation_names())
	$AnimatedSprite2D.animation = mob_types.pick_random()
	$AnimatedSprite2D.play()


func _on_VisibilityNotifier2D_screen_exited():
	queue_free()

# Vita GXM does not drive 2D on-screen notifiers yet.  This supplements
# the original screen_exited event and bounds active physics bodies.
func _physics_process(_delta: float) -> void:
	var margin := 160.0
	var rect := get_viewport_rect()
	if global_position.x < -margin or global_position.y < -margin:
		queue_free()
	elif global_position.x > rect.size.x + margin or global_position.y > rect.size.y + margin:
		queue_free()
