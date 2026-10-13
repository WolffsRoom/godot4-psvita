extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	Engine.time_scale = 3.0
	var game := (load("res://main.tscn") as PackedScene).instantiate()
	root.add_child(game)
	await process_frame
	game.new_game()
	game.get_node("Player/CollisionShape2D").set_deferred("disabled", true)
	var frames: Array[int] = []
	for i in 12:
		await create_timer(1.0).timeout
		frames.append(game.get_node("Player/AnimatedSprite2D").frame)
		if game.score >= 10:
			break
	var mobs := get_nodes_in_group("mobs").size()
	printerr("[score10-smoke] score=", game.score, " mobs=", mobs,
		" player_animation=", game.get_node("Player/AnimatedSprite2D").animation,
		" frames=", frames)
	var passed: bool = game.score >= 10 and mobs > 0
	printerr("[score10-smoke] RESULT=", "PASS" if passed else "FAIL")
	quit(0 if passed else 3)
