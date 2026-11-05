def can_build(env, platform):
    return env.editor_build and platform in ("windows", "linuxbsd", "macos")


def configure(env):
    env.module_add_dependencies("vita_editor", ["gdscript"])
