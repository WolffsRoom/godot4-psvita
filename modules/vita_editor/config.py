def can_build(env, platform):
    return env.editor_build and platform == "windows"


def configure(env):
    env.module_add_dependencies("vita_editor", ["gdscript"])
