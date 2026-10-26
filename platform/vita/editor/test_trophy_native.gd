@tool
extends SceneTree
var correct_title_count := 0
func _initialize() -> void:
	_run.call_deferred()
func _find(node: Node) -> int:
	var found := 0
	if node is Button:
		assert(not node.text.contains("TrofÃ"))
		if node.text == "Troféus PS Vita":
			correct_title_count += 1
	var script = node.get_script()
	if script != null and str(script.resource_path).ends_with("/trophy_panel.gd"):
		found += 1
	for child in node.get_children():
		found += _find(child)
	return found
func _run() -> void:
	assert(_find(root) == 1)
	assert(correct_title_count == 1)
	print("PASS: exactly one native trophy panel registered for the open project; UTF-8 title is correct")
	quit()
