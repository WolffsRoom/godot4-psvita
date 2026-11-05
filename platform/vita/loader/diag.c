/*
 * PS Vita early diagnostics.
 * A project folder identity is embedded in app0:game_data_folder.cfg by the
 * export platform for BOTH split and embedded VPKs. The log location is not
 * tied to the location of game.pck.
 *
 * The startup markers run in constructors, so use raw sceIo, static storage,
 * and no heap allocation. A missing legacy cfg uses the stable fallback.
 */
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include "diag.h"

static char data_root[100] = "ux0:data/godotvitagame";
static char logs_root[112] = "ux0:data/godotvitagame/logs";
static int paths_ready = 0;

static unsigned slen(const char *s) {
    unsigned n = 0;
    while (s[n]) ++n;
    return n;
}
static void append(char *dst, unsigned cap, const char *str) {
    unsigned at = slen(dst);
    unsigned i = 0;
    while (str[i] && at + 1 < cap) dst[at++] = str[i++];
    dst[at] = 0;
}
static int allowed(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '_';
}
void vita_init_log_paths(void) {
    if (paths_ready) return;
    paths_ready = 1;
    char folder[66] = { 0 };
    SceUID f = sceIoOpen("app0:game_data_folder.cfg", SCE_O_RDONLY, 0);
    if (f >= 0) {
        int n = sceIoRead(f, folder, 65);
        sceIoClose(f);
        int valid = n > 0 && n <= 64;
        for (int i = 0; i < n && valid; i++) valid = allowed(folder[i]);
        if (valid) {
            folder[n] = 0;
            data_root[0] = 0;
            append(data_root, sizeof(data_root), "ux0:data/");
            append(data_root, sizeof(data_root), folder);
        }
    }
    logs_root[0] = 0;
    append(logs_root, sizeof(logs_root), data_root);
    append(logs_root, sizeof(logs_root), "/logs");
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir(data_root, 0777);
    sceIoMkdir(logs_root, 0777);
}
const char *vita_log_dir(void) {
    vita_init_log_paths();
    return logs_root;
}
const char *vita_data_dir(void) {
    vita_init_log_paths();
    return data_root;
}
void diag(const char *s) {
    vita_init_log_paths();
    char path[160] = { 0 };
    append(path, sizeof(path), logs_root);
    append(path, sizeof(path), "/godot_diag.log");
    SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, s, slen(s));
        sceIoClose(fd);
    }
}
__attribute__((constructor(101))) static void diag_first(void) {
    vita_init_log_paths();
    char path[160] = { 0 };
    append(path, sizeof(path), logs_root);
    append(path, sizeof(path), "/godot_diag.log");
    sceIoRemove(path);
    diag("[ctor first] earliest constructor ran (Sce imports work)\n");
}
__attribute__((constructor(65535))) static void diag_last(void) {
    diag("[ctor last] all static constructors completed\n");
}
