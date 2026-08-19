/*
 * Null-safe dlopen/dlsym/dlclose/dlerror.
 *
 * Godot calls these at startup (GDExtension / dynamic library support). The Vita
 * has no real runtime dynamic loading in this model, and VitaSDK's libdl dlsym
 * dereferences a null handle and crashes. These stubs fail gracefully instead,
 * so GDExtension init just reports "unavailable" and the engine continues.
 * (This replaces -ldl, which is removed from the loader link.)
 */

void *dlopen(const char *filename, int flags) {
	(void)filename;
	(void)flags;
	return 0; // No handle: dynamic loading unsupported on Vita.
}

int dlclose(void *handle) {
	(void)handle;
	return 0;
}

void *dlsym(void *handle, const char *symbol) {
	(void)handle;
	(void)symbol;
	return 0; // Null-safe: never dereference the handle.
}

char *dlerror(void) {
	return "dynamic loading is not supported on Vita";
}
