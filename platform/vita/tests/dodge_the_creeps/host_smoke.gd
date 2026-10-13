extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var packed := load("res://main.tscn") as PackedScene
	if packed == null:
		printerr("[dodge-smoke] FAIL main scene resource")
		quit(2)
		return
	var game := packed.instantiate()
	root.add_child(game)
	await process_frame
	game.new_game()
	await create_timer(3.1).timeout
	var start_pos: Vector2 = game.get_node("Player").position
	Input.action_press("move_right")
	await create_timer(0.25).timeout
	Input.action_release("move_right")
	var end_pos: Vector2 = game.get_node("Player").position
	var mobs := get_nodes_in_group("mobs").size()
	var score: int = game.score
	var sound_ok := game.get_node("Music").stream != null and game.get_node("DeathSound").stream != null
	printerr("[dodge-smoke] score=", score, " mobs=", mobs, " start=", start_pos,
		" end=", end_pos, " audio_assets=", sound_ok)
	if score <= 0 or mobs <= 0 or end_pos.x <= start_pos.x or not sound_ok:
		printerr("[dodge-smoke] FAIL gameplay")
		quit(3)
		return
	printerr("[dodge-smoke] PASS")
	quit(0)
