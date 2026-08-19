/*
 * TLS init-function stubs.
 *
 * libgodot.so left these `_ZTH*` symbols (the dynamic-init wrappers for a few
 * thread_local variables) undefined — a side effect of building the engine as
 * -fPIC -nostdlib. The variables in question are pointers/ints that are
 * zero-initialized, so a no-op init is safe for bring-up. The loader defines
 * them here; default_dynlib maps the .so's imports to these.
 *
 * TODO: fix properly on the .so side (ensure the TLS init wrappers are emitted)
 * once the engine boots.
 */
void _ZTHN12MessageQueue16thread_singletonE(void) {}
void _ZTHN14ResourceLoader12load_nestingE(void) {}
void _ZTHN16GDScriptLanguage11_call_stackE(void) {}
void _ZTHN16GDScriptLanguage16_call_stack_sizeE(void) {}
void _ZTHN16GDScriptLanguage21_debug_parse_err_lineE(void) {}
void _ZTHN4Node28current_process_thread_groupE(void) {}
