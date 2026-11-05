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
/* Runtime-compiled native GXM canvas shaders (Cg -> GXP via vitaShaRK).
 * Uniforms use Godot ShaderMaterial names and are uploaded per draw. */
typedef struct VitaGXMCanvasUniform {
    char name[48];
    float value[4];
    int components;
} VitaGXMCanvasUniform;
typedef struct VitaGXMCanvasSampler {
    void *texture;
} VitaGXMCanvasSampler;
int vita_gxm_canvas_compiler_idle(void);
int vita_gxm_canvas_shader_compiler_enabled(void);
int vita_gxm_canvas_shader_create(const char *cg_source);
int vita_gxm_canvas_shader_draw(int shader_handle, void *texture, void *screen_texture,
        const float *xy, const float *uv, uint32_t color,
        const VitaGXMCanvasUniform *uniforms, unsigned uniform_count,
        const VitaGXMCanvasSampler *samplers, unsigned sampler_count);
/* Screen sampling uses separate GXM render targets (never read-write aliasing). */
int vita_gxm_canvas_screen_begin(uint32_t color);
void *vita_gxm_canvas_screen_snapshot(void);
void vita_gxm_blend_add(int enabled);
void vita_gxm_quad(const float *xy, uint32_t color);
void *vita_gxm_texture_create(int width, int height, const uint8_t *rgba);
void vita_gxm_texture_free(void *texture);
/* CPU readback of a texture made by vita_gxm_texture_create, as RGBA8.
 * When supported, the renderer drops its RGBA8 copy after upload and reads
 * the pixels back only if something asks for the image. RGBA4444 textures
 * come back at 4-bit precision. Returns 1 on success. */
int vita_gxm_texture_readback_supported(void);
int vita_gxm_texture_read(void *texture, uint8_t *rgba, int width, int height);
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
