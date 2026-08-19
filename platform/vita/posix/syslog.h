/* Vita stub: VitaSDK newlib has no <syslog.h>. drivers/unix/syslog_logger just
 * discards messages here (Godot also logs through its own console path). */
#pragma once

#include <stdarg.h>

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

static inline void openlog(const char *, int, int) {}
static inline void closelog(void) {}
static inline void syslog(int, const char *, ...) {}
static inline void vsyslog(int, const char *, va_list) {}
