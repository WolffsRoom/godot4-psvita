#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int vita_gxm_init(void);
void vita_gxm_begin(uint32_t color);
void vita_gxm_quad(const float *xy, uint32_t color);
void *vita_gxm_texture_create(int width, int height, const uint8_t *rgba);
void vita_gxm_texture_free(void *texture);
void vita_gxm_texture_quad(void *texture, const float *xy, const float *uv, uint32_t color, int linear);
void vita_gxm_clip(int enabled, int x, int y, int width, int height);
void vita_gxm_end(int present);
void vita_gxm_finish(void);
#ifdef __cplusplus
}
#endif
