#pragma once

namespace GXMCameraLOD {
// The engine has already rejected objects outside the camera frustum. This
// second stage removes geometry too small to matter on the Vita's 960x544
// display while keeping nearby objects and large room structure.
// Objects drawn in the previous frame use slightly wider limits, so an object
// sitting on a threshold does not alternate between visible and culled.
static inline bool should_cull(float center_distance, float radius, bool was_drawn = false) {
    if (center_distance < 0.001f) return false;
    const float edge_distance = center_distance - radius;
    const float slack = was_drawn ? 1.15f : 1.0f;
    if (edge_distance > 40.0f * slack) return true;
    // ~0.012 rad is a few pixels on the Vita screen; larger props stayed
    // visibly on screen when R13 culled them at 0.065 (objects popping out).
    const float apparent_radius = radius / center_distance;
    return edge_distance > 22.0f * slack && apparent_radius < 0.012f / slack;
}
}
