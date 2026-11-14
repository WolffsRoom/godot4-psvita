#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"
#include "core/variant/typed_array.h"

class PhysicsDirectSpaceState3D;

// CPU ground-fog fluid for renderers without programmable shaders (native GXM
// on PS Vita). It ports the "Cheap Ground Fog" shaders to a grid:
//   - step(): the shallow-water pass of fog_trail.gdshader (carved fog and
//     velocity, advection, pressure, vorticity confinement, body carving,
//     brushes and landing bursts);
//   - build(): the surface of ground_fog.gdshader evaluated per grid point,
//     giving a displaced vertex buffer and an RGBA8 colour/opacity texture.
// The body mask camera is replaced by footprints rasterised from each body's
// world bounds, and the terrain height window is raycast here instead of in
// script.
class FogFluidSim : public RefCounted {
	GDCLASS(FogFluidSim, RefCounted);

	int res = 0; // Cells per side; grid points are cell centres.
	float size = 16.0f; // World size (m) of the window.
	Vector2i cell; // Window centre in texels.

	LocalVector<float> carved, vel_x, vel_y; // Simulation state.
	LocalVector<float> next_carved, next_vx, next_vy;
	LocalVector<float> heights; // Terrain Y per cell, -1e4 where there is none.
	LocalVector<float> mask_bottom; // Lowest body surface per cell, or +inf.
	LocalVector<float> depth, top; // Fog thickness and fog top per cell.
	LocalVector<float> draped; // Terrain with the ledge spill applied.
	LocalVector<uint8_t> drape_dirty; // Cells whose `draped` must be redone.
	LocalVector<uint8_t> influence; // Cells a body, brush or burst can touch this step.
	float draped_for = -1.0f; // ledge_drape the cache was built with.
	LocalVector<uint8_t> noise; // Tiling 8-bit noise.
	int noise_w = 0, noise_h = 0;

	HashMap<StringName, float> params;
	uint32_t terrain_mask = 1;
	uint64_t last_step_usec = 0, last_build_usec = 0, last_bake_usec = 0;

	// Surface parameters, read once per build() instead of once per cell.
	struct DepthParams {
		float fog_height, streak, hill_scale, wind_x, wind_y, morph_speed;
		float coverage_scale, drift_x, drift_y, coverage_cutoff, coverage_softness;
		float wake_lift, radius, edge_softness;
	} dp;

	float p(const StringName &p_name, float p_default) const;
	float sample_noise(float p_u, float p_v) const;
	float sample(const LocalVector<float> &p_field, float p_u, float p_v, float p_outside) const;
	float sample_height(float p_u, float p_v) const;
	void sample3(float p_u, float p_v, float &r_carved, float &r_vx, float &r_vy) const;
	void mark_influence(float p_u0, float p_v0, float p_u1, float p_v1);
	void mark_drape_dirty(int p_x, int p_y);
	float dip_at(float p_u, float p_v, float p_fog_height) const;
	bool is_moving(Object *p_collider) const;
	float cast_down(PhysicsDirectSpaceState3D *p_space, const Vector3 &p_from, float p_bottom) const;
	void bake_texel(PhysicsDirectSpaceState3D *p_space, int p_x, int p_y, float p_node_y, float p_max_surface_height, float p_max_depth);
	float fog_depth(int p_x, int p_y, const Vector2 &p_world_xz, float p_ground, float p_time, const Vector3 &p_player) const;

protected:
	static void _bind_methods();

public:
	void setup(int p_resolution, float p_size);
	int get_resolution() const { return res; }
	void set_param(const StringName &p_name, float p_value) { params[p_name] = p_value; }
	void set_params(const Dictionary &p_params);
	void set_terrain_mask(uint32_t p_mask) { terrain_mask = p_mask; }
	void set_noise(const PackedByteArray &p_luminance, int p_width, int p_height);
	void set_cell(const Vector2i &p_cell) { cell = p_cell; }
	Vector2i get_cell() const { return cell; }

	// Terrain window: full bake, a few refreshed rows, or a scroll by whole
	// texels that only raycasts the new strips.
	void bake_rows(PhysicsDirectSpaceState3D *p_space, int p_first_row, int p_rows, float p_node_y, float p_max_surface_height, float p_max_depth);
	void shift(PhysicsDirectSpaceState3D *p_space, const Vector2i &p_delta, float p_node_y, float p_max_surface_height, float p_max_depth);
	float get_height(int p_x, int p_y) const;

	void clear_bodies();
	void add_body(const AABB &p_world_bounds, bool p_round);

	void step(float p_delta, const PackedVector4Array &p_brushes_from, const PackedVector4Array &p_brushes_to, int p_brush_count, const PackedVector4Array &p_bursts, int p_burst_count);
	// Returns [vertex bytes (res*res xyz floats, mesh local), RGBA8 bytes (res*res)].
	Array build(float p_time, const Vector3 &p_origin, const Vector3 &p_player, const Vector3 &p_camera, const Vector3 &p_light_dir, const Color &p_light, const Color &p_ambient);
	PackedInt32Array make_indices() const;
	PackedVector2Array make_uvs() const;
	Dictionary get_timings() const;
};
