#include "fog_fluid_sim.h"

#include "core/object/class_db.h"
#include "core/os/os.h"
#include "scene/3d/physics/animatable_body_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"
#include "servers/physics_3d/physics_server_3d_types.h"

static const float NO_GROUND = -1e4f;
static const float NO_BODY = 1e9f;

static _FORCE_INLINE_ float smooth(float p_from, float p_to, float p_x) {
	if (p_to <= p_from) {
		return p_x < p_from ? 0.0f : 1.0f;
	}
	float t = CLAMP((p_x - p_from) / (p_to - p_from), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

static _FORCE_INLINE_ float mixf(float p_a, float p_b, float p_t) {
	return p_a + (p_b - p_a) * p_t;
}

float FogFluidSim::p(const StringName &p_name, float p_default) const {
	const float *value = params.getptr(p_name);
	return value ? *value : p_default;
}

void FogFluidSim::set_params(const Dictionary &p_params) {
	for (const KeyValue<Variant, Variant> &kv : p_params) {
		params[StringName(kv.key)] = float(kv.value);
	}
}

void FogFluidSim::setup(int p_resolution, float p_size) {
	res = CLAMP(p_resolution, 8, 256);
	size = MAX(p_size, 0.01f);
	const uint32_t count = uint32_t(res * res);
	carved.resize(count);
	vel_x.resize(count);
	vel_y.resize(count);
	next_carved.resize(count);
	next_vx.resize(count);
	next_vy.resize(count);
	heights.resize(count);
	mask_bottom.resize(count);
	depth.resize(count);
	top.resize(count);
	draped.resize(count);
	drape_dirty.resize(count);
	influence.resize(count);
	draped_for = -1.0f;
	for (uint32_t i = 0; i < count; i++) {
		carved[i] = vel_x[i] = vel_y[i] = 0.0f;
		heights[i] = NO_GROUND;
		mask_bottom[i] = NO_BODY;
		depth[i] = top[i] = 0.0f;
	}
}

void FogFluidSim::set_noise(const PackedByteArray &p_luminance, int p_width, int p_height) {
	ERR_FAIL_COND(p_width <= 0 || p_height <= 0 || p_luminance.size() < p_width * p_height);
	noise_w = p_width;
	noise_h = p_height;
	noise.resize(p_width * p_height);
	memcpy(noise.ptr(), p_luminance.ptr(), p_width * p_height);
}

// Bilinear, repeating (the shader's noise sampler uses repeat_enable).
float FogFluidSim::sample_noise(float p_u, float p_v) const {
	if (!noise_w) {
		return 0.5f;
	}
	float fx = p_u * noise_w - 0.5f;
	float fy = p_v * noise_h - 0.5f;
	float flx = Math::floor(fx);
	float fly = Math::floor(fy);
	float tx = fx - flx;
	float ty = fy - fly;
	int x0 = int(flx) % noise_w;
	int y0 = int(fly) % noise_h;
	if (x0 < 0) {
		x0 += noise_w;
	}
	if (y0 < 0) {
		y0 += noise_h;
	}
	int x1 = x0 + 1 == noise_w ? 0 : x0 + 1;
	int y1 = y0 + 1 == noise_h ? 0 : y0 + 1;
	const uint8_t *n = noise.ptr();
	float a = mixf(n[y0 * noise_w + x0], n[y0 * noise_w + x1], tx);
	float b = mixf(n[y1 * noise_w + x0], n[y1 * noise_w + x1], tx);
	return mixf(a, b, ty) * (1.0f / 255.0f);
}

// Bilinear with clamped edges; `p_outside` is returned outside 0..1, which is
// how the simulation reads "untouched fog" beyond its window.
float FogFluidSim::sample(const LocalVector<float> &p_field, float p_u, float p_v, float p_outside) const {
	if (p_u < 0.0f || p_v < 0.0f || p_u > 1.0f || p_v > 1.0f) {
		return p_outside;
	}
	float fx = CLAMP(p_u * res - 0.5f, 0.0f, float(res - 1));
	float fy = CLAMP(p_v * res - 0.5f, 0.0f, float(res - 1));
	int x0 = int(fx);
	int y0 = int(fy);
	int x1 = MIN(x0 + 1, res - 1);
	int y1 = MIN(y0 + 1, res - 1);
	float tx = fx - x0;
	float ty = fy - y0;
	const float *f = p_field.ptr();
	float a = mixf(f[y0 * res + x0], f[y0 * res + x1], tx);
	float b = mixf(f[y1 * res + x0], f[y1 * res + x1], tx);
	return mixf(a, b, ty);
}

float FogFluidSim::sample_height(float p_u, float p_v) const {
	return sample(heights, CLAMP(p_u, 0.0f, 1.0f), CLAMP(p_v, 0.0f, 1.0f), NO_GROUND);
}

// The three simulation fields at one point, sharing the bilinear weights.
void FogFluidSim::sample3(float p_u, float p_v, float &r_carved, float &r_vx, float &r_vy) const {
	if (p_u < 0.0f || p_v < 0.0f || p_u > 1.0f || p_v > 1.0f) {
		r_carved = r_vx = r_vy = 0.0f;
		return;
	}
	float fx = CLAMP(p_u * res - 0.5f, 0.0f, float(res - 1));
	float fy = CLAMP(p_v * res - 0.5f, 0.0f, float(res - 1));
	int x0 = int(fx);
	int y0 = int(fy);
	int x1 = MIN(x0 + 1, res - 1);
	int y1 = MIN(y0 + 1, res - 1);
	float tx = fx - x0;
	float ty = fy - y0;
	const float w00 = (1.0f - tx) * (1.0f - ty), w10 = tx * (1.0f - ty), w01 = (1.0f - tx) * ty, w11 = tx * ty;
	const int i00 = y0 * res + x0, i10 = y0 * res + x1, i01 = y1 * res + x0, i11 = y1 * res + x1;
	const float *c = carved.ptr();
	const float *gx = vel_x.ptr();
	const float *gy = vel_y.ptr();
	r_carved = c[i00] * w00 + c[i10] * w10 + c[i01] * w01 + c[i11] * w11;
	r_vx = gx[i00] * w00 + gx[i10] * w10 + gx[i01] * w01 + gx[i11] * w11;
	r_vy = gy[i00] * w00 + gy[i10] * w10 + gy[i01] * w01 + gy[i11] * w11;
}

void FogFluidSim::mark_influence(float p_u0, float p_v0, float p_u1, float p_v1) {
	int x0 = MAX(int(Math::floor(p_u0 * res)) - 1, 0);
	int y0 = MAX(int(Math::floor(p_v0 * res)) - 1, 0);
	int x1 = MIN(int(Math::floor(p_u1 * res)) + 1, res - 1);
	int y1 = MIN(int(Math::floor(p_v1 * res)) + 1, res - 1);
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			influence[y * res + x] = 1;
		}
	}
}

// A changed terrain texel changes the ledge spill of everything within reach.
void FogFluidSim::mark_drape_dirty(int p_x, int p_y) {
	const int reach = draped_for > 0.0f ? int(Math::ceil(draped_for / (size / res))) + 1 : 0;
	for (int y = MAX(p_y - reach, 0); y <= MIN(p_y + reach, res - 1); y++) {
		for (int x = MAX(p_x - reach, 0); x <= MIN(p_x + reach, res - 1); x++) {
			drape_dirty[y * res + x] = 1;
		}
	}
}

float FogFluidSim::get_height(int p_x, int p_y) const {
	if (!res) {
		return NO_GROUND;
	}
	return heights[CLAMP(p_y, 0, res - 1) * res + CLAMP(p_x, 0, res - 1)];
}

// How far the lowest body surface over this point sits below the fog top, or
// -1 where no body covers it.
float FogFluidSim::dip_at(float p_u, float p_v, float p_fog_height) const {
	int x = int(Math::floor(p_u * res));
	int y = int(Math::floor(p_v * res));
	if (x < 0 || y < 0 || x >= res || y >= res) {
		return -1.0f;
	}
	float bottom = mask_bottom[y * res + x];
	if (bottom >= NO_BODY * 0.5f) {
		return -1.0f;
	}
	return sample_height(p_u, p_v) + p_fog_height - bottom;
}

bool FogFluidSim::is_moving(Object *p_collider) const {
	if (!Object::cast_to<PhysicsBody3D>(p_collider)) {
		return false;
	}
	return !Object::cast_to<StaticBody3D>(p_collider) || Object::cast_to<AnimatableBody3D>(p_collider);
}

// Height of the first static surface below `p_from`, or NO_GROUND.
float FogFluidSim::cast_down(PhysicsDirectSpaceState3D *p_space, const Vector3 &p_from, float p_bottom) const {
	PS3DT::RayParameters ray;
	ray.from = p_from;
	ray.to = Vector3(p_from.x, p_bottom, p_from.z);
	ray.collision_mask = terrain_mask;
	for (int i = 0; i < 4; i++) {
		PS3DT::RayResult hit;
		if (!p_space->intersect_ray(ray, hit)) {
			break;
		}
		// Skip whatever moves (player, ragdoll, platforms).
		if (is_moving(hit.collider)) {
			ray.exclude.insert(hit.rid);
			continue;
		}
		return hit.position.y;
	}
	return NO_GROUND;
}

void FogFluidSim::bake_texel(PhysicsDirectSpaceState3D *p_space, int p_x, int p_y, float p_node_y, float p_max_surface_height, float p_max_depth) {
	const float texel = size / res;
	const float half = res * 0.5f;
	const float wx = (cell.x + p_x + 0.5f - half) * texel;
	const float wz = (cell.y + p_y + 0.5f - half) * texel;
	const Vector3 cap(wx, p_node_y + p_max_surface_height, wz);
	const float bottom = p_node_y - p_max_depth;
	float h = cast_down(p_space, Vector3(wx, p_node_y + p_max_depth, wz), bottom);
	if (h > cap.y) {
		// Something above the cap: an underside right above the cap point is
		// an overhang and the fog goes on the floor under it; otherwise the
		// cap point is buried in the terrain and the top surface is the ground.
		PS3DT::RayParameters up;
		up.from = cap;
		up.to = Vector3(wx, h + 0.01f, wz);
		up.collision_mask = terrain_mask;
		up.hit_from_inside = true;
		PS3DT::RayResult ceiling;
		if (p_space->intersect_ray(up, ceiling) && !is_moving(ceiling.collider)) {
			h = cast_down(p_space, cap, bottom);
		}
	}
	if (heights[p_y * res + p_x] != h) {
		heights[p_y * res + p_x] = h;
		mark_drape_dirty(p_x, p_y);
	}
}

void FogFluidSim::bake_rows(PhysicsDirectSpaceState3D *p_space, int p_first_row, int p_rows, float p_node_y, float p_max_surface_height, float p_max_depth) {
	ERR_FAIL_NULL(p_space);
	if (!res) {
		return;
	}
	uint64_t started = OS::get_singleton()->get_ticks_usec();
	for (int r = 0; r < p_rows; r++) {
		int y = (p_first_row + r) % res;
		for (int x = 0; x < res; x++) {
			bake_texel(p_space, x, y, p_node_y, p_max_surface_height, p_max_depth);
		}
	}
	last_bake_usec = OS::get_singleton()->get_ticks_usec() - started;
}

// Moves the window by whole texels: terrain and simulation keep their overlap,
// only the new terrain strips are raycast and new fog cells start at rest.
void FogFluidSim::shift(PhysicsDirectSpaceState3D *p_space, const Vector2i &p_delta, float p_node_y, float p_max_surface_height, float p_max_depth) {
	ERR_FAIL_NULL(p_space);
	if (!res || p_delta == Vector2i()) {
		return;
	}
	cell += p_delta;
	draped_for = -1.0f; // The whole window moved.
	const uint32_t count = uint32_t(res * res);
	// `depth` is rebuilt on every build(), so it doubles as scratch here.
	LocalVector<float> &old_heights = depth;
	memcpy(old_heights.ptr(), heights.ptr(), count * sizeof(float));
	for (uint32_t i = 0; i < count; i++) {
		next_carved[i] = next_vx[i] = next_vy[i] = 0.0f;
		heights[i] = NO_GROUND;
	}
	for (int y = 0; y < res; y++) {
		int sy = y + p_delta.y;
		if (sy < 0 || sy >= res) {
			continue;
		}
		for (int x = 0; x < res; x++) {
			int sx = x + p_delta.x;
			if (sx < 0 || sx >= res) {
				continue;
			}
			heights[y * res + x] = old_heights[sy * res + sx];
			next_carved[y * res + x] = carved[sy * res + sx];
			next_vx[y * res + x] = vel_x[sy * res + sx];
			next_vy[y * res + x] = vel_y[sy * res + sx];
		}
	}
	SWAP(carved, next_carved);
	SWAP(vel_x, next_vx);
	SWAP(vel_y, next_vy);
	for (int y = 0; y < res; y++) {
		int sy = y + p_delta.y;
		for (int x = 0; x < res; x++) {
			int sx = x + p_delta.x;
			if (sx < 0 || sx >= res || sy < 0 || sy >= res) {
				bake_texel(p_space, x, y, p_node_y, p_max_surface_height, p_max_depth);
			}
		}
	}
}

void FogFluidSim::clear_bodies() {
	for (uint32_t i = 0; i < mask_bottom.size(); i++) {
		mask_bottom[i] = NO_BODY;
	}
}

// Stands in for the mask camera: the body's footprint (box or ellipse of its
// world bounds) records the height of its lowest surface.
void FogFluidSim::add_body(const AABB &p_world_bounds, bool p_round) {
	if (!res) {
		return;
	}
	const float texel = size / res;
	const float half = res * 0.5f;
	const Vector3 c = p_world_bounds.get_center();
	const float hx = MAX(p_world_bounds.size.x * 0.5f, texel * 0.5f);
	const float hz = MAX(p_world_bounds.size.z * 0.5f, texel * 0.5f);
	const float cx = c.x / texel - cell.x + half;
	const float cz = c.z / texel - cell.y + half;
	const float rx = hx / texel;
	const float rz = hz / texel;
	int x0 = MAX(int(Math::floor(cx - rx)), 0);
	int x1 = MIN(int(Math::ceil(cx + rx)), res - 1);
	int y0 = MAX(int(Math::floor(cz - rz)), 0);
	int y1 = MIN(int(Math::ceil(cz + rz)), res - 1);
	const float bottom = p_world_bounds.position.y;
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			float dx = (x + 0.5f - cx) / rx;
			float dz = (y + 0.5f - cz) / rz;
			bool inside = p_round ? dx * dx + dz * dz <= 1.0f : (Math::abs(dx) <= 1.0f && Math::abs(dz) <= 1.0f);
			if (inside && bottom < mask_bottom[y * res + x]) {
				mask_bottom[y * res + x] = bottom;
			}
		}
	}
}

// One pass of fog_trail.gdshader over the grid. Units as in the shader: the
// state is carved fog (0 rest, 1 cleared, < 0 piled) and velocity in uv/s.
void FogFluidSim::step(float p_delta, const PackedVector4Array &p_brushes_from, const PackedVector4Array &p_brushes_to, int p_brush_count, const PackedVector4Array &p_bursts, int p_burst_count) {
	if (!res) {
		return;
	}
	uint64_t started = OS::get_singleton()->get_ticks_usec();
	const float dt = CLAMP(p_delta, 0.0f, 0.1f);
	const float refill_speed = p("refill_speed", 0.6f);
	const float velocity_damping = p("velocity_damping", 0.8f);
	const float wave_speed = p("wave_speed", 0.06f);
	const float diffusion = p("diffusion", 0.2f);
	const float vorticity = p("vorticity", 0.04f);
	const float fog_height = p("fog_height", 0.5f);
	const float mask_softness = p("mask_softness", 0.25f);
	const float footprint_spread = p("footprint_spread", 0.01f);
	const float carve_speed = p("carve_speed", 6.0f);
	const float inject_rate = p("inject_rate", 6.0f);
	const float drag = p("drag", 0.7f);
	const float push_speed = p("push_speed", 0.05f);
	const float push_reach = p("push_reach", 0.04f);
	const float burst_speed = p("burst_speed", 0.12f);
	const float burst_width = MAX(p("burst_width", 0.02f), 1e-5f);

	const float px = 1.0f / res;
	const float inv2 = res * 0.5f; // 1 / (2 px)
	const float refill_k = Math::exp(-refill_speed * dt);
	const float damping_k = Math::exp(-velocity_damping * dt);
	const float carve_k = 1.0f - Math::exp(-carve_speed * dt);
	const float inject_k = 1.0f - Math::exp(-inject_rate * dt);
	const float burst_k = 1.0f - Math::exp(-12.0f * dt);
	const int brush_count = MIN(p_brush_count, MIN(int(p_brushes_from.size()), int(p_brushes_to.size())));
	const int burst_count = MIN(p_burst_count, int(p_bursts.size()));
	const Vector4 *brushes_from = p_brushes_from.ptr();
	const Vector4 *brushes_to = p_brushes_to.ptr();
	const Vector4 *bursts = p_bursts.ptr();

	// Only cells near a body can be carved by the mask; the ring search for
	// the dilated footprint is skipped everywhere else.
	// Cells out of reach of every body, brush and burst and with a calm
	// neighbourhood stay exactly as they are and are skipped.
	memset(influence.ptr(), 0, influence.size());
	bool any_body = false;
	{
		const int spread = int(Math::ceil(footprint_spread * res)) + 1;
		const float *mb = mask_bottom.ptr();
		uint8_t *inf = influence.ptr();
		for (int y = 0; y < res; y++) {
			for (int x = 0; x < res; x++) {
				if (mb[y * res + x] < NO_BODY * 0.5f) {
					any_body = true;
					for (int yy = MAX(y - spread, 0); yy <= MIN(y + spread, res - 1); yy++) {
						memset(inf + yy * res + MAX(x - spread, 0), 1, MIN(x + spread, res - 1) - MAX(x - spread, 0) + 1);
					}
				}
			}
		}
	}
	for (int i = 0; i < brush_count; i++) {
		const float outer = brushes_from[i].z * 2.0f + push_reach;
		mark_influence(MIN(brushes_from[i].x, brushes_to[i].x) - outer, MIN(brushes_from[i].y, brushes_to[i].y) - outer,
				MAX(brushes_from[i].x, brushes_to[i].x) + outer, MAX(brushes_from[i].y, brushes_to[i].y) + outer);
	}
	for (int i = 0; i < burst_count; i++) {
		const float outer = bursts[i].z + 4.0f * burst_width;
		mark_influence(bursts[i].x - outer, bursts[i].y - outer, bursts[i].x + outer, bursts[i].y + outer);
	}
	const uint8_t *touched = influence.ptr();
	static const float RING[8][2] = {
		{ 1.0f, 0.0f }, { 0.70710678f, 0.70710678f }, { 0.0f, 1.0f }, { -0.70710678f, 0.70710678f },
		{ -1.0f, 0.0f }, { -0.70710678f, -0.70710678f }, { 0.0f, -1.0f }, { 0.70710678f, -0.70710678f }
	};

	const float *c = carved.ptr();
	const float *gx = vel_x.ptr();
	const float *gy = vel_y.ptr();
#define AT(m_field, m_x, m_y) (((m_x) < 0 || (m_y) < 0 || (m_x) >= res || (m_y) >= res) ? 0.0f : m_field[(m_y) * res + (m_x)])

	for (int y = 0; y < res; y++) {
		for (int x = 0; x < res; x++) {
			const float u = (x + 0.5f) * px;
			const float v = (y + 0.5f) * px;
			const float velx = gx[y * res + x];
			const float vely = gy[y * res + x];

			const float nl_r = AT(c, x - 1, y), nr_r = AT(c, x + 1, y), nd_r = AT(c, x, y - 1), nu_r = AT(c, x, y + 1);
			const float nl_g = AT(gx, x - 1, y), nr_g = AT(gx, x + 1, y), nd_g = AT(gx, x, y - 1), nu_g = AT(gx, x, y + 1);
			const float nl_b = AT(gy, x - 1, y), nr_b = AT(gy, x + 1, y), nd_b = AT(gy, x, y - 1), nu_b = AT(gy, x, y + 1);
			const bool hot = touched[y * res + x];
			if (!hot && velx == 0.0f && vely == 0.0f && c[y * res + x] == 0.0f &&
					nl_r == 0.0f && nr_r == 0.0f && nd_r == 0.0f && nu_r == 0.0f &&
					nl_g == 0.0f && nr_g == 0.0f && nd_g == 0.0f && nu_g == 0.0f &&
					nl_b == 0.0f && nr_b == 0.0f && nd_b == 0.0f && nu_b == 0.0f) {
				next_carved[y * res + x] = 0.0f;
				next_vx[y * res + x] = 0.0f;
				next_vy[y * res + x] = 0.0f;
				continue;
			}
			const float div = (nr_g - nl_g) * inv2 + (nu_b - nd_b) * inv2;
			const float grad_x = (nr_r - nl_r) * inv2;
			const float grad_y = (nu_r - nd_r) * inv2;
			const float curl = (nr_b - nl_b) * inv2 - (nu_g - nd_g) * inv2;

			// Curl at the four neighbours, for the vorticity confinement.
			float conf_x = 0.0f, conf_y = 0.0f;
			if (vorticity != 0.0f && curl != 0.0f) {
				const float r2x = AT(gx, x + 2, y), r2y = AT(gy, x + 2, y);
				const float l2x = AT(gx, x - 2, y), l2y = AT(gy, x - 2, y);
				const float u2x = AT(gx, x, y + 2), u2y = AT(gy, x, y + 2);
				const float d2x = AT(gx, x, y - 2), d2y = AT(gy, x, y - 2);
				const float rux = AT(gx, x + 1, y + 1), ruy = AT(gy, x + 1, y + 1);
				const float rdx = AT(gx, x + 1, y - 1), rdy = AT(gy, x + 1, y - 1);
				const float lux = AT(gx, x - 1, y + 1), luy = AT(gy, x - 1, y + 1);
				const float ldx = AT(gx, x - 1, y - 1), ldy = AT(gy, x - 1, y - 1);
				(void)r2x; (void)l2x; (void)u2y; (void)d2y;
				const float curl_r = (r2y - vely) * inv2 - (rux - rdx) * inv2;
				const float curl_l = (vely - l2y) * inv2 - (lux - ldx) * inv2;
				const float curl_u = (ruy - luy) * inv2 - (u2x - velx) * inv2;
				const float curl_d = (rdy - ldy) * inv2 - (velx - d2x) * inv2;
				const float gcx = Math::abs(curl_r) - Math::abs(curl_l);
				const float gcy = Math::abs(curl_u) - Math::abs(curl_d);
				const float len = MAX(Math::sqrt(gcx * gcx + gcy * gcy), 1e-5f);
				conf_x = vorticity * (gcy / len) * curl;
				conf_y = vorticity * (-gcx / len) * curl;
			}

			// Semi-Lagrangian advection plus a little blur.
			const float su = u - velx * dt;
			const float sv = v - vely * dt;
			float s_r, s_g, s_b;
			if (velx == 0.0f && vely == 0.0f) {
				s_r = mixf(c[y * res + x], (nl_r + nr_r + nd_r + nu_r) * 0.25f, diffusion);
				s_g = mixf(velx, (nl_g + nr_g + nd_g + nu_g) * 0.25f, diffusion);
				s_b = mixf(vely, (nl_b + nr_b + nd_b + nu_b) * 0.25f, diffusion);
			} else {
				float here_r, here_g, here_b, r1, g1, b1, r2, g2, b2, r3, g3, b3, r4, g4, b4;
				sample3(su, sv, here_r, here_g, here_b);
				sample3(su + px, sv, r1, g1, b1);
				sample3(su - px, sv, r2, g2, b2);
				sample3(su, sv + px, r3, g3, b3);
				sample3(su, sv - px, r4, g4, b4);
				const float ar = (r1 + r2 + r3 + r4) * 0.25f;
				const float ag = (g1 + g2 + g3 + g4) * 0.25f;
				const float ab = (b1 + b2 + b3 + b4) * 0.25f;
				s_r = mixf(here_r, ar, diffusion);
				s_g = mixf(here_g, ag, diffusion);
				s_b = mixf(here_b, ab, diffusion);
			}

			// Continuity, slow relax to rest height, then pressure.
			const float h = 1.0f - s_r;
			float cv = 1.0f - MAX(h - h * div * dt, 0.0f);
			cv *= refill_k;
			float vx = (s_g + (wave_speed * wave_speed * grad_x + conf_x) * dt) * damping_k;
			float vy = (s_b + (wave_speed * wave_speed * grad_y + conf_y) * dt) * damping_k;

			if (any_body && hot) {
				float dip = dip_at(u, v, fog_height);
				for (int i = 0; i < 8; i++) {
					dip = MAX(dip, dip_at(u + RING[i][0] * footprint_spread, v + RING[i][1] * footprint_spread, fog_height) - mask_softness * 0.5f);
				}
				if (dip > 0.0f) {
					cv = mixf(cv, 1.0f, smooth(0.0f, mask_softness, dip) * carve_k);
				}
			}

			for (int i = 0; hot && i < brush_count; i++) {
				const Vector4 &f = brushes_from[i];
				const Vector4 &to = brushes_to[i];
				const float abx = to.x - f.x;
				const float aby = to.y - f.y;
				const float t = CLAMP(((u - f.x) * abx + (v - f.y) * aby) / MAX(abx * abx + aby * aby, 1e-8f), 0.0f, 1.0f);
				const float ox = u - (f.x + abx * t);
				const float oy = v - (f.y + aby * t);
				const float dist = Math::sqrt(ox * ox + oy * oy);
				const float outer = f.z * 2.0f + push_reach;
				if (dist >= outer) {
					continue;
				}
				float m = (1.0f - smooth(f.z, outer, dist)) * f.w;
				const float body_speed = Math::sqrt(to.z * to.z + to.w * to.w);
				m *= smooth(0.0f, wave_speed, body_speed);
				if (m <= 0.0f) {
					continue;
				}
				const float inv = 1.0f / MAX(dist, 1e-5f);
				const float push = MIN(body_speed, push_speed);
				const float k = m * inject_k;
				vx = mixf(vx, to.z * drag + ox * inv * push, k);
				vy = mixf(vy, to.w * drag + oy * inv * push, k);
			}

			for (int i = 0; hot && i < burst_count; i++) {
				const Vector4 &b = bursts[i];
				const float ox = u - b.x;
				const float oy = v - b.y;
				const float d = Math::sqrt(ox * ox + oy * oy);
				const float q = (d - b.z) / burst_width;
				if (q < 4.0f) {
					const float ring = q > -4.0f ? Math::exp(-q * q) : 0.0f;
					const float k = ring * b.w * burst_k;
					const float inv = 1.0f / MAX(d, 1e-5f);
					vx = mixf(vx, ox * inv * burst_speed, k);
					vy = mixf(vy, oy * inv * burst_speed, k);
					cv = mixf(cv, 1.0f, (1.0f - smooth(b.z * 0.4f, b.z, d)) * b.w * burst_k);
				}
			}

			const float speed = Math::sqrt(vx * vx + vy * vy);
			const float limit = MIN(1.0f, 0.5f / MAX(speed, 1e-5f));
			vx *= limit;
			vy *= limit;
			// Let tiny residues settle so calm areas take the cheap path.
			if (Math::abs(vx) < 1e-6f) {
				vx = 0.0f;
			}
			if (Math::abs(vy) < 1e-6f) {
				vy = 0.0f;
			}
			next_carved[y * res + x] = CLAMP(cv, -1.0f, 1.0f);
			next_vx[y * res + x] = vx;
			next_vy[y * res + x] = vy;
		}
	}
#undef AT
	SWAP(carved, next_carved);
	SWAP(vel_x, next_vx);
	SWAP(vel_y, next_vy);
	last_step_usec = OS::get_singleton()->get_ticks_usec() - started;
}

// Thickness of the fog over the ground: noise hills, the simulation (carved
// or piled fog and the lifted rim of trails), coverage and the sphere edge.
float FogFluidSim::fog_depth(int p_x, int p_y, const Vector2 &p_world_xz, float p_ground, float p_time, const Vector3 &p_player) const {
	const float fog_height = dp.fog_height, streak = dp.streak, hill_scale = dp.hill_scale;
	const float wind_x = dp.wind_x, wind_y = dp.wind_y, morph_speed = dp.morph_speed;
	const float coverage_scale = dp.coverage_scale, drift_x = dp.drift_x, drift_y = dp.drift_y;
	const float coverage_cutoff = dp.coverage_cutoff, coverage_softness = dp.coverage_softness;
	const float wake_lift = dp.wake_lift, radius = dp.radius, edge_softness = dp.edge_softness;

	// Outside the sphere there is nothing to shape.
	const float dist = Vector3(p_world_xz.x, p_ground, p_world_xz.y).distance_to(p_player);
	if (dist >= radius) {
		return 0.0f;
	}
	// Grid points sit on the texel centres, so the shader's bilinear reads
	// are plain array reads here.
	const int i = p_y * res + p_x;
	const float sim_r = carved[i];
	const float flow_x = -vel_x[i] * size * streak;
	const float flow_y = -vel_y[i] * size * streak;
	const float *cv = carved.ptr();
	const float grad = Math::abs(cv[p_y * res + MIN(p_x + 2, res - 1)] - cv[p_y * res + MAX(p_x - 2, 0)]) +
			Math::abs(cv[MIN(p_y + 2, res - 1) * res + p_x] - cv[MAX(p_y - 2, 0) * res + p_x]);

	const float px = (p_world_xz.x + flow_x) * hill_scale;
	const float py = (p_world_xz.y + flow_y) * hill_scale;
	const float n = sample_noise(px + p_time * wind_x, py + p_time * wind_y) * 0.6f +
			sample_noise(px * 1.7f - wind_y * p_time * morph_speed * 10.0f, py * 1.7f + wind_x * p_time * morph_speed * 10.0f) * 0.4f;
	const float hills = smooth(0.2f, 0.8f, n);
	float coverage = sample_noise(p_world_xz.x * coverage_scale + p_time * drift_x, p_world_xz.y * coverage_scale + p_time * drift_y);
	coverage = smooth(coverage_cutoff, coverage_cutoff + coverage_softness, coverage);

	const float edge = 1.0f - smooth(radius - edge_softness, radius, dist);
	const float amount = CLAMP(1.0f - sim_r, 0.0f, 2.0f) * edge * coverage;
	return fog_height * amount * (mixf(0.4f, 1.2f, hills) + grad * wake_lift);
}

// The surface of ground_fog.gdshader per grid point. Opacity follows the
// shader's Compatibility path (no depth texture): the view ray crosses the
// whole layer. Lighting is baked into the colour.
Array FogFluidSim::build(float p_time, const Vector3 &p_origin, const Vector3 &p_player, const Vector3 &p_camera, const Vector3 &p_light_dir, const Color &p_light, const Color &p_ambient) {
	Array result;
	if (!res) {
		return result;
	}
	uint64_t started = OS::get_singleton()->get_ticks_usec();
	dp.fog_height = p("fog_height", 0.5f);
	dp.streak = p("streak", 0.6f);
	dp.hill_scale = p("hill_scale", 0.06f);
	dp.wind_x = p("wind_x", 0.03f);
	dp.wind_y = p("wind_y", 0.012f);
	dp.morph_speed = p("morph_speed", 0.15f);
	dp.coverage_scale = p("coverage_scale", 0.015f);
	dp.drift_x = p("coverage_drift_x", 0.006f);
	dp.drift_y = p("coverage_drift_y", 0.004f);
	dp.coverage_cutoff = p("coverage_cutoff", 0.35f);
	dp.coverage_softness = p("coverage_softness", 0.3f);
	dp.wake_lift = p("wake_lift", 0.4f);
	dp.radius = p("radius", 8.0f);
	dp.edge_softness = p("edge_softness", 3.0f);
	const float density = p("density", 1.6f);
	const float lighting = p("lighting", 0.8f);
	const float ground_softness = MAX(p("ground_softness", 0.3f), 1e-4f);
	const float ledge_drape = p("ledge_drape", 0.8f);
	const float presence = p("presence", 1.0f);
	const float camera_fade = p("camera_fade", 1.5f);
	const float detail_strength = p("detail_strength", 0.5f);
	const float detail_scale = p("detail_scale", 0.8f);
	const float detail_cycle = p("detail_cycle", 1.2f);
	const Color fog_color(p("fog_color_r", 0.82f), p("fog_color_g", 0.81f), p("fog_color_b", 0.76f), p("fog_color_a", 1.0f));
	const float px = 1.0f / res;
	const float texel = size / res;
	const float half = res * 0.5f;

	for (int y = 0; y < res; y++) {
		for (int x = 0; x < res; x++) {
			const int i = y * res + x;
			const float ground = heights[i];
			if (ground <= NO_GROUND * 0.5f) {
				depth[i] = 0.0f;
				top[i] = NO_GROUND;
				continue;
			}
			const Vector2 world((cell.x + x + 0.5f - half) * texel, (cell.y + y + 0.5f - half) * texel);
			depth[i] = fog_depth(x, y, world, ground, p_time, p_player);
			top[i] = ground + depth[i];
		}
	}

	PackedByteArray vertex_bytes;
	vertex_bytes.resize(res * res * 3 * sizeof(float));
	float *vertices = (float *)vertex_bytes.ptrw();
	PackedByteArray pixels;
	pixels.resize(res * res * 4);
	uint8_t *rgba = pixels.ptrw();

	const bool drape_all = draped_for != ledge_drape;
	draped_for = ledge_drape;
	const float still_wisps = sample_noise(0.0f, 0.0f);
	const bool wisps_flow = detail_scale > 0.0f && detail_cycle > 0.0f;
	const float phase = wisps_flow ? p_time / detail_cycle : 0.0f;
	const float p0 = phase - Math::floor(phase);
	const float p1 = (phase + 0.5f) - Math::floor(phase + 0.5f);
	const Vector3 light_dir = p_light_dir.normalized();
	static const float DIRS[8][2] = {
		{ 1.0f, 0.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f },
		{ 0.707f, 0.707f }, { -0.707f, 0.707f }, { 0.707f, -0.707f }, { -0.707f, -0.707f }
	};

	for (int y = 0; y < res; y++) {
		for (int x = 0; x < res; x++) {
			const int i = y * res + x;
			const float u = (x + 0.5f) * px;
			const float v = (y + 0.5f) * px;
			const float wx = (cell.x + x + 0.5f - half) * texel;
			const float wz = (cell.y + y + 0.5f - half) * texel;
			float *vertex = vertices + i * 3;
			uint8_t *pixel = rgba + i * 4;
			vertex[0] = wx - p_origin.x;
			vertex[2] = wz - p_origin.z;
			pixel[0] = uint8_t(CLAMP(fog_color.r, 0.0f, 1.0f) * 255.0f);
			pixel[1] = uint8_t(CLAMP(fog_color.g, 0.0f, 1.0f) * 255.0f);
			pixel[2] = uint8_t(CLAMP(fog_color.b, 0.0f, 1.0f) * 255.0f);
			pixel[3] = 0;
			float ground = heights[i];
			if (ground <= NO_GROUND * 0.5f) {
				// No ground here: keep the vertex level with a neighbour so the
				// invisible triangles do not stretch down to the sunk height.
				float level = p_player.y;
				const int xn = x > 0 ? x - 1 : x + 1;
				const int yn = y > 0 ? y - 1 : y + 1;
				if (top[y * res + xn] > NO_GROUND * 0.5f) {
					level = top[y * res + xn];
				} else if (top[yn * res + x] > NO_GROUND * 0.5f) {
					level = top[yn * res + x];
				}
				vertex[1] = level - p_origin.y;
				continue;
			}
			// Fog spreads past ledges and slopes down them instead of stepping.
			// Only redone where the terrain under it changed.
			if (drape_all || drape_dirty[i]) {
				drape_dirty[i] = 0;
				float value = ground;
				if (ledge_drape > 0.0f) {
					float blur = 0.0f;
					bool valid = true;
					for (int d = 0; d < 8 && valid; d++) {
						float hgt = sample_height(u + DIRS[d][0] * ledge_drape / size, v + DIRS[d][1] * ledge_drape / size);
						valid = hgt > NO_GROUND * 0.5f;
						blur += hgt;
					}
					if (valid) {
						value = MAX(ground, blur * 0.125f);
					}
				}
				this->draped[i] = value;
			}
			const float draped = this->draped[i];
			vertex[1] = draped - p_origin.y + 0.02f + depth[i];

			// Smooth layer depth and normal from the neighbours one texel away.
			const int xl = MAX(x - 1, 0), xr = MIN(x + 1, res - 1), yd = MAX(y - 1, 0), yu = MIN(y + 1, res - 1);
			const int il = y * res + xl, ir = y * res + xr, id = yd * res + x, iu = yu * res + x;
			const float layer = (depth[il] + depth[ir] + depth[id] + depth[iu]) * 0.25f;
			if (layer <= 0.0f) {
				continue;
			}
			float hx = (top[ir] > NO_GROUND * 0.5f && top[il] > NO_GROUND * 0.5f) ? top[ir] - top[il] : 0.0f;
			float hz = (top[iu] > NO_GROUND * 0.5f && top[id] > NO_GROUND * 0.5f) ? top[iu] - top[id] : 0.0f;
			hx = CLAMP(hx, -4.0f * texel, 4.0f * texel);
			hz = CLAMP(hz, -4.0f * texel, 4.0f * texel);
			const Vector3 normal = Vector3(-hx, 2.0f * texel, -hz).normalized();

			const Vector3 position(wx, draped + 0.02f + depth[i], wz);
			const Vector3 to_fragment = position - p_camera;
			const float frag_dist = MAX(to_fragment.length(), 1e-4f);
			const float descent = MAX(-to_fragment.y / frag_dist, 0.02f);
			const float slab = MAX(layer, 0.001f);
			float thickness = slab / descent;

			// Wisps dragged along the simulated flow.
			float wisps;
			if (wisps_flow) {
				const float vxm = vel_x[i] * size;
				const float vym = vel_y[i] * size;
				const float dx = wx * detail_scale;
				const float dy = wz * detail_scale;
				const float n0 = sample_noise(dx - vxm * p0 * detail_cycle * detail_scale, dy - vym * p0 * detail_cycle * detail_scale);
				const float n1 = sample_noise(dx + 0.37f - vxm * p1 * detail_cycle * detail_scale, dy + 0.71f - vym * p1 * detail_cycle * detail_scale);
				wisps = mixf(n0, n1, Math::abs(1.0f - 2.0f * p0));
			} else {
				wisps = still_wisps;
			}
			thickness *= mixf(1.0f, smooth(0.15f, 0.85f, wisps) * 2.0f, detail_strength);

			float alpha = (1.0f - Math::exp(-thickness * density)) * smooth(0.0f, ground_softness, layer) * fog_color.a * presence;
			if (camera_fade > 0.0f) {
				alpha *= smooth(0.0f, camera_fade, frag_dist);
			}
			// diffuse_lambert_wrap with roughness 1: (N.L + 1) / 4.
			const float wrap = MAX(normal.dot(light_dir) + 1.0f, 0.0f) * 0.25f;
			const float lit_r = p_ambient.r + p_light.r * wrap;
			const float lit_g = p_ambient.g + p_light.g * wrap;
			const float lit_b = p_ambient.b + p_light.b * wrap;
			pixel[0] = uint8_t(CLAMP(fog_color.r * (lighting * lit_r + (1.0f - lighting)), 0.0f, 1.0f) * 255.0f);
			pixel[1] = uint8_t(CLAMP(fog_color.g * (lighting * lit_g + (1.0f - lighting)), 0.0f, 1.0f) * 255.0f);
			pixel[2] = uint8_t(CLAMP(fog_color.b * (lighting * lit_b + (1.0f - lighting)), 0.0f, 1.0f) * 255.0f);
			pixel[3] = uint8_t(CLAMP(alpha, 0.0f, 1.0f) * 255.0f);
		}
	}
	last_build_usec = OS::get_singleton()->get_ticks_usec() - started;
	result.push_back(vertex_bytes);
	result.push_back(pixels);
	return result;
}

PackedInt32Array FogFluidSim::make_indices() const {
	PackedInt32Array indices;
	if (res < 2) {
		return indices;
	}
	indices.resize((res - 1) * (res - 1) * 6);
	int32_t *w = indices.ptrw();
	for (int y = 0; y < res - 1; y++) {
		for (int x = 0; x < res - 1; x++) {
			const int i = y * res + x;
			*w++ = i;
			*w++ = i + 1;
			*w++ = i + res;
			*w++ = i + 1;
			*w++ = i + res + 1;
			*w++ = i + res;
		}
	}
	return indices;
}

PackedVector2Array FogFluidSim::make_uvs() const {
	PackedVector2Array uvs;
	uvs.resize(res * res);
	Vector2 *w = uvs.ptrw();
	for (int y = 0; y < res; y++) {
		for (int x = 0; x < res; x++) {
			w[y * res + x] = Vector2((x + 0.5f) / res, (y + 0.5f) / res);
		}
	}
	return uvs;
}

Dictionary FogFluidSim::get_timings() const {
	Dictionary d;
	d["step_usec"] = last_step_usec;
	d["build_usec"] = last_build_usec;
	d["bake_usec"] = last_bake_usec;
	return d;
}

void FogFluidSim::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "resolution", "size"), &FogFluidSim::setup);
	ClassDB::bind_method(D_METHOD("get_resolution"), &FogFluidSim::get_resolution);
	ClassDB::bind_method(D_METHOD("set_param", "name", "value"), &FogFluidSim::set_param);
	ClassDB::bind_method(D_METHOD("set_params", "params"), &FogFluidSim::set_params);
	ClassDB::bind_method(D_METHOD("set_terrain_mask", "mask"), &FogFluidSim::set_terrain_mask);
	ClassDB::bind_method(D_METHOD("set_noise", "luminance", "width", "height"), &FogFluidSim::set_noise);
	ClassDB::bind_method(D_METHOD("set_cell", "cell"), &FogFluidSim::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &FogFluidSim::get_cell);
	ClassDB::bind_method(D_METHOD("bake_rows", "space", "first_row", "rows", "node_y", "max_surface_height", "max_depth"), &FogFluidSim::bake_rows);
	ClassDB::bind_method(D_METHOD("shift", "space", "delta", "node_y", "max_surface_height", "max_depth"), &FogFluidSim::shift);
	ClassDB::bind_method(D_METHOD("get_height", "x", "y"), &FogFluidSim::get_height);
	ClassDB::bind_method(D_METHOD("clear_bodies"), &FogFluidSim::clear_bodies);
	ClassDB::bind_method(D_METHOD("add_body", "world_bounds", "round"), &FogFluidSim::add_body);
	ClassDB::bind_method(D_METHOD("step", "delta", "brushes_from", "brushes_to", "brush_count", "bursts", "burst_count"), &FogFluidSim::step);
	ClassDB::bind_method(D_METHOD("build", "time", "origin", "player", "camera", "light_dir", "light", "ambient"), &FogFluidSim::build);
	ClassDB::bind_method(D_METHOD("make_indices"), &FogFluidSim::make_indices);
	ClassDB::bind_method(D_METHOD("make_uvs"), &FogFluidSim::make_uvs);
	ClassDB::bind_method(D_METHOD("get_timings"), &FogFluidSim::get_timings);
}
