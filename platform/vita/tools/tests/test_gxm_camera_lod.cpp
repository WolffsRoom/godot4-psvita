#include "../../gxm_camera_lod.h"
#include <cassert>
#include <cstdio>

int main() {
    // Player-facing detail remains while it is near the camera.
    assert(!GXMCameraLOD::should_cull(8.0f, 0.15f));
    // A prop that is still several pixels wide at room distance stays.
    assert(!GXMCameraLOD::should_cull(22.0f, 0.40f));
    // Tiny props that cover only a pixel or two far away are culled.
    assert(GXMCameraLOD::should_cull(30.0f, 0.30f));
    // Large walls and room structure remain at the same distance.
    assert(!GXMCameraLOD::should_cull(22.0f, 3.0f));
    // Geometry beyond the Vita scene range is culled regardless of size.
    assert(GXMCameraLOD::should_cull(50.0f, 4.0f));
    // Camera inside a large volume must retain it.
    assert(!GXMCameraLOD::should_cull(1.0f, 8.0f));
    // Hysteresis: objects just past a limit stay while they were drawn.
    assert(GXMCameraLOD::should_cull(46.0f, 4.0f));
    assert(!GXMCameraLOD::should_cull(46.0f, 4.0f, true));
    assert(GXMCameraLOD::should_cull(30.0f, 0.34f));
    assert(!GXMCameraLOD::should_cull(30.0f, 0.34f, true));
    assert(GXMCameraLOD::should_cull(60.0f, 4.0f, true));
    std::puts("PASS GXM camera LOD: near detail, apparent size, large structure, far range and hysteresis");
}
