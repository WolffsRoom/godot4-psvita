@tool
extends EditorPlugin

const VitaPlatform = preload("platform.gd")
const DefaultsInspector = preload("defaults_inspector.gd")
var platform: EditorExportPlatformExtension
var defaults_inspector: EditorInspectorPlugin
var trophy_panel: Control

func _enter_tree() -> void:
	if ClassDB.class_exists("VitaExportEditorPlugin"):
		return # The integrated editor already registers the platform globally.
	platform = VitaPlatform.new()
	add_export_platform(platform)
	defaults_inspector = DefaultsInspector.new()
	add_inspector_plugin(defaults_inspector)
	trophy_panel = preload("trophy_panel.gd").new()
	add_control_to_bottom_panel(trophy_panel, "Troféus PS Vita")

func _exit_tree() -> void:
	if trophy_panel != null:
		remove_control_from_bottom_panel(trophy_panel)
		trophy_panel.queue_free()
	trophy_panel = null
	if defaults_inspector != null:
		remove_inspector_plugin(defaults_inspector)
	defaults_inspector = null
	if platform != null:
		remove_export_platform(platform)
	platform = null
