#ifndef __DIAG_H__
#define __DIAG_H__
void vita_init_log_paths(void);
const char *vita_log_dir(void);
const char *vita_data_dir(void);
void diag(const char *s);
#endif
