#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int vita_gxm_init(void);
void vita_gxm_begin(uint32_t color);
int vita_gxm_triangles3d(void *texture, const float *xyz, const float *uv, unsigned count, const float *mvp, uint32_t color);
void vita_gxm_scene3d_end(void);
/* GPU-resident static geometry: vertices are copied once into a mapped arena
 * and drawn every frame without CPU vertex conversion. */
void *vita_gxm_mesh_create(const float *xyz, const float *uv, unsigned count);
int vita_gxm_mesh_draw(void *mesh, void *texture, const float *mvp, uint32_t color);
int vita_gxm_mesh_update(void *mesh, const float *xyz, const float *uv, unsigned count); // In-place rewrite
void vita_gxm_mesh_reset(void);
unsigned vita_gxm_mesh_arena_used(void);
unsigned vita_gxm_last_gpu_wait_us(void);
void vita_gxm_set_vsync(int enabled);
void vita_gxm_depth_write(int enabled);
int vita_gxm_vertex_lighting_available(void);
void *vita_gxm_light_mesh_create(const float *xyz, unsigned count);
void vita_gxm_light_mesh_set_colors(void *mesh, const uint32_t *colors);
int vita_gxm_light_mesh_draw(void *mesh, const float *mvp);
void vita_gxm_blend_add(int enabled);
void vita_gxm_quad(const float *xy, uint32_t color);
void *vita_gxm_texture_create(int width, int height, const uint8_t *rgba);
void vita_gxm_texture_free(void *texture);
void vita_gxm_texture_quad(void *texture, const float *xy, const float *uv, uint32_t color, int linear);
void vita_gxm_clip(int enabled, int x, int y, int width, int height);
/* Offscreen render targets (SubViewport): create, then begin/end around the
 * target's draws before the main scene of the frame. */
void *vita_gxm_target_create(int width, int height);
int vita_gxm_target_begin(void *target, uint32_t clear_color);
void vita_gxm_target_end(void);
/* Tilt-shift. begin_offscreen replaces vita_gxm_begin for the 3D
 * pass; resolve composites it onto the display before the UI draws. */
int vita_gxm_begin_offscreen(uint32_t color);
void vita_gxm_tiltshift_resolve(float top, float bottom, float strength);
void vita_gxm_end(int present);
void vita_gxm_finish(void);
#ifdef __cplusplus
}
#endif
