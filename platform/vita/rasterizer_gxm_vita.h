#pragma once
#include "servers/rendering/dummy/rasterizer_dummy.h"

// First milestone: a single screen viewport and untextured canvas rectangles.
// Unimplemented services retain the engine's dummy implementations.
class RasterizerGXMVita : public RasterizerDummy {
public:
    static RendererCompositor *create() { return memnew(RasterizerGXMVita); }
    static void make_current() { _create_func = create; low_end = true; }
    void end_frame(bool p_present) override;
    // Shows the project's boot splash while the scripts and main scene load
    // (previously a black screen for ~30 s on Tenebrarium).
    void set_boot_image_with_stretch(const Ref<Image> &p_image, const Color &p_color, RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true) override;
    RasterizerGXMVita();
    ~RasterizerGXMVita();
};
