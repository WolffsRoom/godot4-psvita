#include "register_types.h"

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "scene/resources/image_texture.h"
#include "scene/gui/box_container.h"
#include "editor/export/editor_export_platform_extension.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/plugins/editor_plugin.h"

// Built-in editor plugin: registered for every project, with support files
// resolved beside the executable rather than inside the user's project.
class VitaExportPlatform : public EditorExportPlatformExtension {
	GDCLASS(VitaExportPlatform, EditorExportPlatformExtension);
protected:
	static void _bind_methods() {}
public:
	Ref<EditorExportPreset> create_preset() override {
		Ref<EditorExportPreset> preset = EditorExportPlatformExtension::create_preset();
		preset->set_script_export_mode(EditorExportPreset::MODE_SCRIPT_TEXT);
		return preset;
	}
};

class VitaExportEditorPlugin : public EditorPlugin {
	GDCLASS(VitaExportEditorPlugin, EditorPlugin);
	Ref<VitaExportPlatform> platform;
	Ref<EditorInspectorPlugin> inspector;
	VBoxContainer *trophy_panel = nullptr;

protected:
	static void _bind_methods() {}

public:
	VitaExportEditorPlugin() {
		const String root = OS::get_singleton()->get_executable_path().get_base_dir().path_join("vita");
		Ref<Script> platform_script = ResourceLoader::load(root.path_join("platform.gd"));
		ERR_FAIL_COND_MSG(platform_script.is_null() || !platform_script->can_instantiate(), "Missing or invalid Vita support files beside the editor executable.");
		platform.instantiate();
		platform->set_script(platform_script);
		add_export_platform(platform);
		Ref<Script> inspector_script = ResourceLoader::load(root.path_join("defaults_inspector.gd"));
		ERR_FAIL_COND_MSG(inspector_script.is_null() || !inspector_script->can_instantiate(), "Cannot load the Vita export inspector.");
		inspector.instantiate();
		inspector->set_script(inspector_script);
		add_inspector_plugin(inspector);
		Ref<Script> trophy_script = ResourceLoader::load(root.path_join("trophy_panel.gd"));
		if (trophy_script.is_valid() && trophy_script->can_instantiate()) {
			trophy_panel = memnew(VBoxContainer);
			trophy_panel->set_script(trophy_script);
			add_control_to_bottom_panel(trophy_panel, String::utf8("Trof\xc3\xa9us PS Vita"));
		}
	}

	~VitaExportEditorPlugin() {
		if (trophy_panel) {
			remove_control_from_bottom_panel(trophy_panel);
			memdelete(trophy_panel);
		}
		if (inspector.is_valid()) {
			remove_inspector_plugin(inspector);
		}
		if (platform.is_valid()) {
			remove_export_platform(platform);
		}
	}
};

void initialize_vita_editor_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_INTERNAL_CLASS(VitaExportPlatform);
		GDREGISTER_INTERNAL_CLASS(VitaExportEditorPlugin);
		EditorPlugins::add_by_type<VitaExportEditorPlugin>();
	}
}

void uninitialize_vita_editor_module(ModuleInitializationLevel p_level) {}
