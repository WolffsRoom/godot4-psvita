#pragma once
#include "servers/rendering/dummy/rasterizer_dummy.h"

// First milestone: a single screen viewport and untextured canvas rectangles.
// Unimplemented services retain the engine's dummy implementations.
class RasterizerGXMVita : public RasterizerDummy {
public:
    static RendererCompositor *create() { return memnew(RasterizerGXMVita); }
    static void make_current() { _create_func = create; low_end = true; }
    void end_frame(bool p_present) override;
    RasterizerGXMVita();
    ~RasterizerGXMVita();
};
