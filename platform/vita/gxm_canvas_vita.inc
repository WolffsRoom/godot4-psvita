// Native GXM canvas renderer. Included by rasterizer_gxm_vita.cpp after
// GXMTextureStorage. Every primitive is lowered to the two bridge calls the
// loader already exports (flat and textured quads), so this file needs no
// new loader symbols. Triangles are sent as quads with a repeated vertex.

#include "gxm_shader_translator_vita.inc"

class GXMCanvas : public RasterizerCanvasDummy {
    GXMTextureStorage *storage;
    GXMMaterialStorage *materials = nullptr;
    HashSet<RID> diagnosed_shaders;
    HashSet<RID> unshaded_rect_diagnostics;
    HashMap<RID, int> compiled_shaders;
    HashMap<RID, String> shaders_awaiting_compile;
    int active_shader = 0;
    void *active_screen = nullptr;
    bool drawing_screen_target = false;
    bool native_screen_shader_ready = false;
    VitaGXMCanvasUniform active_uniforms[48];
    unsigned active_uniform_count = 0;
    VitaGXMCanvasSampler active_samplers[4];
    unsigned active_sampler_count = 0;
    unsigned int logged_rects = 0;

    struct PolygonData {
        Vector<int> indices;
        Vector<Vector2> points;
        Vector<Color> colors;
        Vector<Vector2> uvs;
    };
    HashMap<PolygonID, PolygonData> polygons;
    PolygonID next_polygon = 1;

    // Per-call state shared by the helpers below.
    float scale_x = 1, scale_y = 1, offset_x = 0, offset_y = 0;
    bool snap = false;
    // CPU time spent walking and lowering canvas items, logged every 300 calls.
    uint64_t perf_canvas_us = 0;
    unsigned perf_items = 0, perf_calls = 0;

    Vector2 to_screen(const Transform2D &xf, const Vector2 &p, bool p_snap) const {
        Vector2 s = xf.xform(p);
        if (p_snap) s = s.floor();
        return Vector2(offset_x + s.x * scale_x, offset_y + s.y * scale_y);
    }

    struct Tex {
        void *gpu = nullptr;
        Size2 size;
        bool linear = false;
    };

    // Consecutive commands mostly use the same texture (tiles, glyphs, UI
    // atlas). Each full resolve costs several RID hash lookups, so the last
    // result is reused within one canvas_render_items call.
    RID cached_rid;
    int cached_filter = -1;
    bool cached_ok = false;
    Tex cached_tex;

    bool resolve_texture(RID texture, const Item *item, RSE::CanvasItemTextureFilter default_filter, Tex &r_tex) {
        if (!texture.is_valid()) return true; // Untextured: flat color.
        const int filter_key = int(item->texture_filter) * 16 + int(default_filter);
        if (texture == cached_rid && filter_key == cached_filter) {
            r_tex = cached_tex;
            return cached_ok;
        }
        cached_rid = texture;
        cached_filter = filter_key;
        cached_ok = resolve_texture_uncached(texture, item, default_filter, cached_tex);
        r_tex = cached_tex;
        return cached_ok;
    }

    bool resolve_texture_uncached(RID texture, const Item *item, RSE::CanvasItemTextureFilter default_filter, Tex &r_tex) {
        r_tex = Tex();
        r_tex.size = storage->texture_size_with_proxy(texture);
        if (r_tex.size.x <= 0 || r_tex.size.y <= 0) return false;
        r_tex.gpu = storage->get_gpu(texture);
        if (!r_tex.gpu) return false;
        auto filter = item->texture_filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? default_filter : item->texture_filter;
        filter = storage->texture_filter(texture, filter);
        r_tex.linear = filter != RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST && filter != RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS;
        return true;
    }

    // Screen-space quad (4 corners in winding order) with normalized UVs.
    void emit_quad(const Tex &tex, const Vector2 *s, const Vector2 *uv, const Color &color) {
        float xy[8], st[8];
        for (int i = 0; i < 4; i++) {
            xy[2 * i] = s[i].x; xy[2 * i + 1] = s[i].y;
            st[2 * i] = uv ? uv[i].x : 0.0f; st[2 * i + 1] = uv ? uv[i].y : 0.0f;
        }
        if (active_shader > 0) {
            if (vita_gxm_canvas_shader_draw(active_shader, tex.gpu, active_screen, xy, st,
                    gxm_color(color), active_uniforms, active_uniform_count,
                    active_samplers, active_sampler_count)) {
                return;
            }
        }
        if (tex.gpu) {
            vita_gxm_texture_quad(tex.gpu, xy, st, gxm_color(color), tex.linear);
        } else {
            vita_gxm_quad(xy, gxm_color(color));
        }
    }

    void emit_triangle(const Tex &tex, const Vector2 *s, const Vector2 *uv, const Color &color) {
        const Vector2 q[4] = { s[0], s[1], s[2], s[2] };
        if (uv) {
            const Vector2 quv[4] = { uv[0], uv[1], uv[2], uv[2] };
            emit_quad(tex, q, quv, color);
        } else {
            emit_quad(tex, q, nullptr, color);
        }
    }

    // Local-space axis-aligned rectangle with a pixel-space source region.
    void emit_rect(const Tex &tex, const Transform2D &xf, const Rect2 &dst, const Rect2 &src, const Color &color) {
        if (dst.size.x == 0 || dst.size.y == 0) return;
        const Vector2 p[4] = { dst.position, dst.position + Vector2(dst.size.x, 0), dst.position + dst.size, dst.position + Vector2(0, dst.size.y) };
        Vector2 s[4];
        for (int i = 0; i < 4; i++) s[i] = to_screen(xf, p[i], snap);
        if (!tex.gpu) {
            if (active_shader > 0) {
                // ColorRect/PanelContainer: UV must span the local quad.
                // Passing null used to set every UV to zero, making all
                // screen samplers read a single pixel.
                const Vector2 full_uv[4] = { Vector2(0, 0), Vector2(1, 0),
                        Vector2(1, 1), Vector2(0, 1) };
                emit_quad(tex, s, full_uv, color);
            } else {
                emit_quad(tex, s, nullptr, color);
            }
            return;
        }
        const Vector2 a = src.position / tex.size, b = (src.position + src.size) / tex.size;
        const Vector2 uv[4] = { a, Vector2(b.x, a.y), b, Vector2(a.x, b.y) };
        emit_quad(tex, s, uv, color);
    }

    // Lines are one canvas pixel wide (scaled with the viewport).
    void emit_line(const Tex &tex, const Transform2D &xf, const Vector2 &a, const Vector2 &b, const Color &color) {
        Vector2 sa = to_screen(xf, a, false), sb = to_screen(xf, b, false);
        Vector2 d = sb - sa;
        float len = d.length();
        if (len < 0.001f) {
            d = Vector2(1, 0);
        } else {
            d /= len;
        }
        const float half = MAX(scale_x, 1.0f) * 0.5f;
        Vector2 n = Vector2(-d.y, d.x) * half;
        const Vector2 s[4] = { sa + n, sb + n, sb - n, sa - n };
        emit_quad(tex, s, nullptr, color);
    }

    static Color average(const Color *c, int n) {
        Color r(0, 0, 0, 0);
        for (int i = 0; i < n; i++) r += c[i];
        return r / float(n);
    }

    void draw_rect_command(const Item *item, const Item::CommandRect *rect, const Transform2D &xf, const Color &modulate,
            RSE::CanvasItemTextureFilter default_filter) {
        if (rect->flags & (CANVAS_RECT_MSDF | CANVAS_RECT_LCD)) {
            WARN_PRINT_ONCE("GXM: MSDF and LCD rectangles are not implemented.");
            return;
        }
        Color color = rect->modulate * item->final_modulate * modulate;
        Tex tex;
        if (!resolve_texture(rect->texture, item, default_filter, tex)) return;
        if (!tex.gpu) {
            // ColorRect + ShaderMaterial without a compiled native program
            // is NOT equivalent to its Godot GLSL fragment output. Painting
            // its raw opaque color covers the scene (e.g. procedural alpha
            // fog, animated overlays). Do not invent an opaque replacement.
            // Keep textured sprites unaffected, and restore normal drawing
            // as soon as the GXP program becomes valid.
            if (active_shader <= 0 && materials) {
                const RID material = item->material.is_valid() ? item->material :
                        (item->material_owner ? item->material_owner->material : RID());
                const RID shader = materials->shader_for_material(material);
                if (shader.is_valid()) {
                    if (!unshaded_rect_diagnostics.has(shader)) {
                        unshaded_rect_diagnostics.insert(shader);
                        fprintf(stderr, "[gxm2d-shader] no-GXP procedural/untextured rectangle suppressed: path=%s size=%.0fx%.0f\n",
                                materials->shader_path(shader).utf8().get_data(),
                                rect->rect.size.x, rect->rect.size.y);
                        fflush(stderr);
                    }
                    return;
                }
            }
            emit_rect(tex, xf, rect->rect, Rect2(), color);
            return;
        }
        Rect2 src = (rect->flags & CANVAS_RECT_REGION) ? rect->source : Rect2(Vector2(), tex.size);
        if (item->material.is_valid() && storage->is_render_target_texture(rect->texture)) {
            // A viewport shown through a material is the magnifier lens: its
            // shader keeps the inscribed circle, emulated here as a fan.
            draw_ellipse(tex, xf, rect->rect, src, color);
            return;
        }
        if (rect->flags & CANVAS_RECT_TILE) {
            draw_tiled(tex, xf, rect->rect, src, color);
            return;
        }
        Vector2 points[4] = { rect->rect.position, rect->rect.position + Vector2(rect->rect.size.x, 0),
                rect->rect.position + rect->rect.size, rect->rect.position + Vector2(0, rect->rect.size.y) };
        Vector2 s[4], uv[4];
        const Vector2 corners[4] = { Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1) };
        for (int i = 0; i < 4; i++) {
            s[i] = to_screen(xf, points[i], snap);
            Vector2 c = corners[i];
            if (rect->flags & CANVAS_RECT_TRANSPOSE) SWAP(c.x, c.y);
            if (rect->flags & CANVAS_RECT_FLIP_H) c.x = 1.0f - c.x;
            if (rect->flags & CANVAS_RECT_FLIP_V) c.y = 1.0f - c.y;
            uv[i] = (src.position + c * src.size) / tex.size;
        }
        emit_quad(tex, s, uv, color);
        if (logged_rects++ < 3) {
            fprintf(stderr, "[gxm2d] rect submitted xy=(%.1f,%.1f) rgba=%08lx\n", s[0].x, s[0].y, (unsigned long)gxm_color(color));
            fflush(stderr);
        }
    }

    void draw_ellipse(const Tex &tex, const Transform2D &xf, const Rect2 &dst, const Rect2 &src, const Color &color) {
        static const int SEGMENTS = 40;
        const Vector2 center = dst.get_center(), radius = dst.size * 0.5f;
        const Vector2 uv_center = (src.position + src.size * 0.5f) / tex.size, uv_radius = src.size * 0.5f / tex.size;
        Vector2 prev_s, prev_uv;
        for (int k = 0; k <= SEGMENTS; k++) {
            const float a = Math::TAU * k / SEGMENTS;
            const Vector2 dir(Math::cos(a), Math::sin(a));
            const Vector2 p_s = to_screen(xf, center + dir * radius, false), p_uv = uv_center + dir * uv_radius;
            if (k > 0) {
                const Vector2 tri[3] = { to_screen(xf, center, false), prev_s, p_s };
                const Vector2 uv[3] = { uv_center, prev_uv, p_uv };
                emit_triangle(tex, tri, uv, color);
            }
            prev_s = p_s;
            prev_uv = p_uv;
        }
    }

    // Texture repeat without sampler wrap: split the source region at
    // texture-cell boundaries. Capped so a huge repeat cannot stall a frame.
    void draw_tiled(const Tex &tex, const Transform2D &xf, const Rect2 &dst, const Rect2 &src, const Color &color) {
        if (src.size.x <= 0 || src.size.y <= 0) return;
        const Vector2 ratio = dst.size / src.size;
        int pieces = 0;
        for (float y = src.position.y; y < src.position.y + src.size.y - 0.001f;) {
            float cell_y = Math::floor(y / tex.size.y) * tex.size.y;
            float y_end = MIN(cell_y + tex.size.y, src.position.y + src.size.y);
            for (float x = src.position.x; x < src.position.x + src.size.x - 0.001f;) {
                float cell_x = Math::floor(x / tex.size.x) * tex.size.x;
                float x_end = MIN(cell_x + tex.size.x, src.position.x + src.size.x);
                Rect2 piece(Vector2(x - cell_x, y - cell_y), Vector2(x_end - x, y_end - y));
                Rect2 out(dst.position + (Vector2(x, y) - src.position) * ratio, piece.size * ratio);
                emit_rect(tex, xf, out, piece, color);
                if (++pieces >= 1024) return;
                x = x_end;
            }
            y = y_end;
        }
    }

    void draw_ninepatch(const Item *item, const Item::CommandNinePatch *np, const Transform2D &xf, const Color &modulate,
            RSE::CanvasItemTextureFilter default_filter) {
        Color color = np->color * item->final_modulate * modulate;
        Tex tex;
        if (!resolve_texture(np->texture, item, default_filter, tex)) return;
        const Rect2 &r = np->rect;
        Rect2 src = np->source;
        if (tex.gpu && (src.size.x == 0 || src.size.y == 0)) src = Rect2(Vector2(), tex.size);
        // Margins are in source pixels and keep that size on screen (SIDE_LEFT, TOP, RIGHT, BOTTOM).
        float ml = np->margin[SIDE_LEFT], mt = np->margin[SIDE_TOP], mr = np->margin[SIDE_RIGHT], mb = np->margin[SIDE_BOTTOM];
        // Shrink margins proportionally when the rectangle is smaller than them.
        if (ml + mr > r.size.x && ml + mr > 0) { float k = r.size.x / (ml + mr); ml *= k; mr *= k; }
        if (mt + mb > r.size.y && mt + mb > 0) { float k = r.size.y / (mt + mb); mt *= k; mb *= k; }
        const float dx[4] = { r.position.x, r.position.x + ml, r.position.x + r.size.x - mr, r.position.x + r.size.x };
        const float dy[4] = { r.position.y, r.position.y + mt, r.position.y + r.size.y - mb, r.position.y + r.size.y };
        const float sml = np->margin[SIDE_LEFT], smt = np->margin[SIDE_TOP], smr = np->margin[SIDE_RIGHT], smb = np->margin[SIDE_BOTTOM];
        const float sx[4] = { src.position.x, src.position.x + sml, src.position.x + src.size.x - smr, src.position.x + src.size.x };
        const float sy[4] = { src.position.y, src.position.y + smt, src.position.y + src.size.y - smb, src.position.y + src.size.y };
        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < 3; i++) {
                if (i == 1 && j == 1 && !np->draw_center) continue;
                Rect2 d(dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]);
                if (d.size.x <= 0 || d.size.y <= 0) continue;
                Rect2 s(sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j]);
                if (tex.gpu && (s.size.x <= 0 || s.size.y <= 0)) continue;
                if (!tex.gpu) {
                    emit_rect(tex, xf, d, s, color);
                    continue;
                }
                // The middle column/row follows the axis mode; corners never tile.
                emit_patch_cell(np->texture, tex, xf, d, s, color, i == 1 ? np->axis_x : RSE::NINE_PATCH_STRETCH,
                        j == 1 ? np->axis_y : RSE::NINE_PATCH_STRETCH);
            }
        }
    }

    // Tiled nine-patch cells used to cost one quad per repeat (a 600 px panel
    // over a 6 px pattern is ~100 quads per row). The cell's source region is
    // copied once into a "sheet" texture holding it repeated kx * ky times, so
    // each sheet-sized piece of the cell is a single quad.
    struct TileSheet {
        void *gpu = nullptr;
        Size2 size;
        Vector2i repeats;
    };
    HashMap<String, TileSheet> tile_sheets;
    static const int SHEET_TARGET = 128;

    const TileSheet *tile_sheet(RID texture, const Rect2 &src, bool tile_x, bool tile_y) {
        const Rect2i region(src);
        if (region.size.x < 1 || region.size.y < 1) return nullptr;
        const String key = itos(texture.get_id()) + ":" + itos(region.position.x) + "," + itos(region.position.y) + "," +
                itos(region.size.x) + "," + itos(region.size.y) + ":" + itos(tile_x) + itos(tile_y);
        const TileSheet *cached = tile_sheets.getptr(key);
        if (cached) return cached->gpu ? cached : nullptr;
        if (tile_sheets.size() >= 256) {
            for (KeyValue<String, TileSheet> &entry : tile_sheets) {
                vita_gxm_texture_free(entry.value.gpu);
            }
            tile_sheets.clear();
        }
        TileSheet sheet;
        Ref<Image> image = storage->texture_2d_get(texture);
        if (image.is_valid() && Rect2i(Vector2i(), image->get_size()).encloses(region)) {
            Ref<Image> cell = image->get_region(region);
            cell->convert(Image::FORMAT_RGBA8);
            sheet.repeats = Vector2i(tile_x ? MAX(1, SHEET_TARGET / region.size.x) : 1, tile_y ? MAX(1, SHEET_TARGET / region.size.y) : 1);
            const Vector2i sheet_size = region.size * sheet.repeats;
            Ref<Image> out = Image::create_empty(sheet_size.x, sheet_size.y, false, Image::FORMAT_RGBA8);
            for (int y = 0; y < sheet.repeats.y; y++) {
                for (int x = 0; x < sheet.repeats.x; x++) {
                    out->blit_rect(cell, Rect2i(Vector2i(), region.size), Vector2i(x, y) * region.size);
                }
            }
            sheet.gpu = vita_gxm_texture_create(sheet_size.x, sheet_size.y, out->get_data().ptr());
            sheet.size = Size2(sheet_size);
        }
        tile_sheets.insert(key, sheet); // A failed sheet is remembered as null.
        cached = tile_sheets.getptr(key);
        return cached->gpu ? cached : nullptr;
    }

    // Splits one axis of a tiled cell into pieces of at most `per_piece` tiles.
    // Returns the piece count; d_len/s_len are destination and source lengths.
    static int tile_pieces(float dst, float src, RSE::NinePatchAxisMode mode, int per_piece, float *d_off, float *d_len, float *s_len, int max_pieces) {
        if (mode == RSE::NINE_PATCH_STRETCH || src <= 0.0f || dst <= src) {
            d_off[0] = 0; d_len[0] = dst; s_len[0] = src;
            return 1;
        }
        // TILE cuts the last tile; TILE_FIT scales whole tiles to fill exactly.
        float tiles = dst / src, scale = 1.0f;
        if (mode == RSE::NINE_PATCH_TILE_FIT) {
            tiles = MAX(1.0f, Math::round(tiles));
            scale = dst / (tiles * src);
        }
        int n = 0;
        float offset = 0;
        for (float remaining = tiles; remaining > 0.001f && n < max_pieces; n++) {
            const float t = MIN((float)per_piece, remaining);
            d_off[n] = offset; d_len[n] = t * src * scale; s_len[n] = t * src;
            offset += d_len[n];
            remaining -= t;
        }
        return n;
    }

    void emit_patch_cell(RID texture, const Tex &tex, const Transform2D &xf, const Rect2 &d, const Rect2 &s, const Color &color,
            RSE::NinePatchAxisMode mode_x, RSE::NinePatchAxisMode mode_y) {
        static const int MAX_PIECES = 256;
        const bool tile_x = mode_x != RSE::NINE_PATCH_STRETCH && d.size.x > s.size.x;
        const bool tile_y = mode_y != RSE::NINE_PATCH_STRETCH && d.size.y > s.size.y;
        Tex draw = tex;
        Vector2i per_piece(1, 1);
        Vector2 origin = s.position;
        if (tile_x || tile_y) {
            const TileSheet *sheet = tile_sheet(texture, s, tile_x, tile_y);
            if (sheet) {
                draw.gpu = sheet->gpu;
                draw.size = sheet->size;
                per_piece = sheet->repeats;
                origin = Vector2();
            }
        }
        float xo[MAX_PIECES], xl[MAX_PIECES], xs[MAX_PIECES], yo[MAX_PIECES], yl[MAX_PIECES], ys[MAX_PIECES];
        const int nx = tile_pieces(d.size.x, s.size.x, tile_x ? mode_x : RSE::NINE_PATCH_STRETCH, per_piece.x, xo, xl, xs, MAX_PIECES);
        int ny = tile_pieces(d.size.y, s.size.y, tile_y ? mode_y : RSE::NINE_PATCH_STRETCH, per_piece.y, yo, yl, ys, MAX_PIECES);
        if (nx * ny > 4096) {
            // Keep the vertex pool safe: stretch the second axis instead.
            ny = tile_pieces(d.size.y, s.size.y, RSE::NINE_PATCH_STRETCH, 1, yo, yl, ys, MAX_PIECES);
        }
        for (int y = 0; y < ny; y++) {
            for (int x = 0; x < nx; x++) {
                emit_rect(draw, xf, Rect2(d.position.x + xo[x], d.position.y + yo[y], xl[x], yl[y]),
                        Rect2(origin, Vector2(xs[x], ys[y])), color);
            }
        }
    }

    void draw_vertices(const Tex &tex, const Transform2D &xf, RSE::PrimitiveType primitive, const Vector2 *points, const Color *colors,
            bool per_vertex_color, const Color &base, const Vector2 *uvs, const int *indices, int count) {
        auto idx = [&](int i) { return indices ? indices[i] : i; };
        auto col = [&](int i) { return per_vertex_color ? colors[idx(i)] * base : base; };
        switch (primitive) {
            case RSE::PRIMITIVE_TRIANGLES:
            case RSE::PRIMITIVE_TRIANGLE_STRIP: {
                const bool strip = primitive == RSE::PRIMITIVE_TRIANGLE_STRIP;
                const int tris = strip ? count - 2 : count / 3;
                for (int t = 0; t < tris; t++) {
                    int k[3] = { strip ? t : t * 3, strip ? t + 1 : t * 3 + 1, strip ? t + 2 : t * 3 + 2 };
                    Vector2 s[3], uv[3];
                    Color c[3];
                    for (int v = 0; v < 3; v++) {
                        s[v] = to_screen(xf, points[idx(k[v])], false);
                        if (uvs) uv[v] = uvs[idx(k[v])];
                        c[v] = col(k[v]);
                    }
                    Color avg = average(c, 3);
                    if (avg.a <= 0.002f) continue;
                    emit_triangle(tex, s, uvs ? uv : nullptr, avg);
                }
            } break;
            case RSE::PRIMITIVE_LINES:
                for (int i = 0; i + 1 < count; i += 2) {
                    emit_line(Tex(), xf, points[idx(i)], points[idx(i + 1)], (col(i) + col(i + 1)) * 0.5f);
                }
                break;
            case RSE::PRIMITIVE_LINE_STRIP:
                for (int i = 0; i + 1 < count; i++) {
                    emit_line(Tex(), xf, points[idx(i)], points[idx(i + 1)], (col(i) + col(i + 1)) * 0.5f);
                }
                break;
            case RSE::PRIMITIVE_POINTS:
                for (int i = 0; i < count; i++) {
                    Vector2 p = points[idx(i)];
                    emit_rect(Tex(), xf, Rect2(p - Vector2(0.5f, 0.5f), Vector2(1, 1)), Rect2(), col(i));
                }
                break;
            default:
                break;
        }
    }

    void draw_polygon(const Item *item, const Item::CommandPolygon *cmd, const Transform2D &xf, const Color &modulate,
            RSE::CanvasItemTextureFilter default_filter) {
        const PolygonData *p = polygons.getptr(cmd->polygon.polygon_id);
        if (!p || p->points.is_empty()) return;
        Tex tex;
        if (!resolve_texture(cmd->texture, item, default_filter, tex)) return;
        const Color base = item->final_modulate * modulate;
        const int point_count = p->points.size();
        const bool per_vertex = p->colors.size() == point_count;
        const Color single = (!per_vertex && p->colors.size() >= 1) ? p->colors[0] * base : base;
        const bool has_uv = tex.gpu && p->uvs.size() == point_count;
        const int count = p->indices.is_empty() ? point_count : p->indices.size();
        draw_vertices(tex, xf, cmd->primitive, p->points.ptr(), per_vertex ? p->colors.ptr() : nullptr, per_vertex,
                per_vertex ? base : single, has_uv ? p->uvs.ptr() : nullptr, p->indices.is_empty() ? nullptr : p->indices.ptr(), count);
    }

    void draw_primitive(const Item *item, const Item::CommandPrimitive *cmd, const Transform2D &xf, const Color &modulate,
            RSE::CanvasItemTextureFilter default_filter) {
        Tex tex;
        if (!resolve_texture(cmd->texture, item, default_filter, tex)) return;
        const Color base = item->final_modulate * modulate;
        switch (cmd->point_count) {
            case 1:
                draw_vertices(tex, xf, RSE::PRIMITIVE_POINTS, cmd->points, cmd->colors, true, base, nullptr, nullptr, 1);
                break;
            case 2:
                draw_vertices(tex, xf, RSE::PRIMITIVE_LINES, cmd->points, cmd->colors, true, base, nullptr, nullptr, 2);
                break;
            case 3:
                draw_vertices(tex, xf, RSE::PRIMITIVE_TRIANGLES, cmd->points, cmd->colors, true, base, tex.gpu ? cmd->uvs : nullptr, nullptr, 3);
                break;
            case 4: {
                static const int quad[6] = { 0, 1, 2, 0, 2, 3 };
                draw_vertices(tex, xf, RSE::PRIMITIVE_TRIANGLES, cmd->points, cmd->colors, true, base, tex.gpu ? cmd->uvs : nullptr, quad, 6);
            } break;
            default:
                break;
        }
    }

public:
    explicit GXMCanvas(GXMTextureStorage *p_storage) : storage(p_storage) {}
    void set_material_storage(GXMMaterialStorage *p_materials) { materials = p_materials; }
    void process_pending_shader() {
        // Compile ONLY at the start of a frame, with NO active GXM scene.
        // The former implementation called SceShaccCg while rendering an
        // item, inside sceGxmBeginScene, and entered a kernel compiler abort.
        if (!vita_gxm_canvas_shader_compiler_enabled() || shaders_awaiting_compile.is_empty()) return;
        // The loader independently enforces this guard. Keep the shader
        // queued if another viewport has an active target scene.
        if (!vita_gxm_canvas_compiler_idle()) return;
        RID shader;
        String cg;
        for (const KeyValue<RID, String> &pair : shaders_awaiting_compile) {
            shader = pair.key;
            cg = pair.value;
            break;
        }
        if (!shader.is_valid()) return;
        shaders_awaiting_compile.erase(shader);
        fprintf(stderr, "[gxm2d-shader] compile scheduling shader=%llu cg_bytes=%d outside_scene=1\n",
                (unsigned long long)shader.get_id(), cg.utf8().length());
        const CharString source_for_log = cg.utf8();
        fprintf(stderr, "[gxm2d-shader] Cg source BEGIN\n%s[gxm2d-shader] Cg source END\n",
                source_for_log.get_data());
        fflush(stderr);
        const CharString utf8 = cg.utf8();
        int program = vita_gxm_canvas_shader_create(utf8.get_data());
        compiled_shaders[shader] = program;
        if (program > 0 && materials &&
                materials->shader_get_code(shader).contains("hint_screen_texture")) {
            native_screen_shader_ready = true;
        }
        fprintf(stderr, "[gxm2d-shader] compile result shader=%llu native_program=%d\n",
                (unsigned long long)shader.get_id(), program);
        fflush(stderr);
    }
    void prepare_material(const Item *item) {
        active_shader = 0;
        active_screen = nullptr;
        active_uniform_count = 0;
        active_sampler_count = 0;
        if (!materials || !vita_gxm_canvas_shader_compiler_enabled()) return;
        const RID material = item->material.is_valid() ? item->material :
                (item->material_owner ? item->material_owner->material : RID());
        if (!material.is_valid()) return;
        RID shader = materials->shader_for_material(material);
        if (!shader.is_valid()) return;
        const int *previous = compiled_shaders.getptr(shader);
        if (previous) {
            active_shader = *previous;
        } else {
            String reason;
            const String source = materials->shader_get_code(shader);
            const String cg = GXMCanvasShaderTranslator::to_cg(source, &reason);
            if (!cg.is_empty()) {
                // Request an out-of-scene compile next frame. Never invoke
                // SceShaccCg on the render-submit call stack.
                shaders_awaiting_compile.insert(shader, cg);
            }
            compiled_shaders.insert(shader, 0);
            fprintf(stderr, "[gxm2d-shader] path=%s code_bytes=%d queued=%d reason=%s\n",
                    materials->shader_path(shader).utf8().get_data(),
                    source.utf8().length(), !cg.is_empty(), reason.utf8().get_data());
            fflush(stderr);
        }
        if (active_shader <= 0) return;
        if (materials->shader_get_code(shader).contains("hint_screen_texture")) {
            if (!drawing_screen_target) {
                // Defer this screen shader until its capture target is enabled.
                active_shader = 0;
                return;
            }
            active_screen = vita_gxm_canvas_screen_snapshot();
            if (!active_screen) {
                active_shader = 0;
                return;
            }
        }
        const HashMap<StringName, Variant> *params = materials->material_parameters(material);
        const PackedStringArray sampler_names = GXMCanvasShaderTranslator::additional_sampler_names(
                materials->shader_get_code(shader));
        for (int i = 0; i < sampler_names.size() && i < 4; ++i) {
            const StringName sampler_name = sampler_names[i];
            RID texture;
            if (params) {
                const Variant *candidate = params->getptr(sampler_name);
                if (candidate && candidate->get_type() == Variant::RID) texture = *candidate;
            }
            if (!texture.is_valid()) texture = materials->shader_get_default_texture_parameter(shader, sampler_name, 0);
            active_samplers[active_sampler_count++].texture = texture.is_valid() ? storage->get_gpu(texture) : nullptr;
        }
        if (params) {
            for (const KeyValue<StringName, Variant> &param : *params) {
                if (active_uniform_count >= 46) break;
                VitaGXMCanvasUniform &u = active_uniforms[active_uniform_count];
                memset(&u, 0, sizeof(u));
                const CharString name = String(param.key).utf8();
                strncpy(u.name, name.get_data(), sizeof(u.name) - 1);
                switch (param.value.get_type()) {
                    case Variant::FLOAT:
                    case Variant::INT:
                        u.value[0] = float(param.value); u.components = 1; break;
                    case Variant::BOOL:
                        u.value[0] = bool(param.value) ? 1.0f : 0.0f; u.components = 1; break;
                    case Variant::COLOR: {
                        const Color c = param.value;
                        u.value[0] = c.r; u.value[1] = c.g;
                        u.value[2] = c.b; u.value[3] = c.a; u.components = 4;
                    } break;
                    case Variant::VECTOR2: {
                        const Vector2 v = param.value;
                        u.value[0] = v.x; u.value[1] = v.y; u.components = 2;
                    } break;
                    case Variant::VECTOR3: {
                        const Vector3 v = param.value;
                        u.value[0] = v.x; u.value[1] = v.y; u.value[2] = v.z; u.components = 3;
                    } break;
                    case Variant::VECTOR4: {
                        const Vector4 v = param.value;
                        u.value[0] = v.x; u.value[1] = v.y;
                        u.value[2] = v.z; u.value[3] = v.w; u.components = 4;
                    } break;
                    default: break;
                }
                if (u.components) active_uniform_count++;
            }
        }
        // Godot canvas builtins are updated once per draw, including while
        // the scene tree is paused (shader TIME is wall-clock elapsed).
        VitaGXMCanvasUniform &time = active_uniforms[active_uniform_count++];
        memset(&time, 0, sizeof(time));
        strcpy(time.name, "TIME"); time.value[0] = OS::get_singleton()->get_ticks_usec() / 1000000.0f;
        time.components = 1;
        VitaGXMCanvasUniform &pixel = active_uniforms[active_uniform_count++];
        memset(&pixel, 0, sizeof(pixel));
        strcpy(pixel.name, "SCREEN_PIXEL_SIZE");
        pixel.value[0] = 1.f / 960.f; pixel.value[1] = 1.f / 544.f;
        pixel.components = 2;
    }

    PolygonID request_polygon(const Vector<int> &p_indices, const Vector<Point2> &p_points, const Vector<Color> &p_colors,
            const Vector<Point2> &p_uvs = Vector<Point2>(), const Vector<int> &p_bones = Vector<int>(),
            const Vector<float> &p_weights = Vector<float>(), int p_count = -1) override {
        PolygonData data;
        data.indices = p_indices;
        if (p_count >= 0 && p_count < data.indices.size()) data.indices.resize(p_count);
        // Drop out-of-range indices once here instead of checking every frame.
        for (int i = 0; i < data.indices.size(); i++) {
            if (data.indices[i] < 0 || data.indices[i] >= p_points.size()) {
                data.indices.clear();
                break;
            }
        }
        data.points = p_points;
        data.colors = p_colors;
        data.uvs = p_uvs;
        PolygonID id = next_polygon++;
        polygons.insert(id, data);
        return id;
    }
    void free_polygon(PolygonID p_polygon) override { polygons.erase(p_polygon); }

    void canvas_render_items(RID target, Item *items, const Color &modulate, Light *, Light *,
            const Transform2D &, RSE::CanvasItemTextureFilter default_filter, RSE::CanvasItemTextureRepeat,
            bool p_snap, bool &sdf_used, RenderingServerTypes::RenderInfo * = nullptr) override {
        sdf_used = false;
        // Vita has a single physical scanout, but the main Godot scene is not
        // guaranteed to own the first RenderingServer render target. A strict
        // RID/960x544 guard used to discard *all* menu items (quads=0, cmds=0).
        // Render any canvas target with items directly to the native screen.
        // This is the Vita-specific offscreen viewport fallback, not full
        // multi-viewport compositing.
        if (!items) return;
        const Size2i target_size = storage->render_target_get_size(target);
        scale_x = target_size.x > 0 ? 960.0f / (float)target_size.x : 1.0f;
        scale_y = target_size.y > 0 ? 544.0f / (float)target_size.y : 1.0f;
        offset_x = 0;
        offset_y = 0;
        snap = p_snap;
        cached_rid = RID(); // Textures may have been freed since the last call.
        // Any target other than the screen is a SubViewport: render it into
        // its own GXM texture (the viewport maps the 960x544 space onto it).
        const bool offscreen = !storage->is_screen(target);
        if (offscreen) {
            void *gpu = storage->target_gpu(target);
            if (!gpu) return;
            if (!vita_gxm_target_begin(gpu, gxm_color(storage->render_target_get_clear_request_color(target)))) {
                WARN_PRINT_ONCE("GXM: offscreen viewport skipped (main scene already started).");
                return;
            }
            storage->render_target_disable_clear_request(target);
        } else if (bool(ProjectSettings::get_singleton()->get_setting("rendering/vita/keep_aspect", false))) {
            scale_x = scale_y = MIN(scale_x, scale_y);
            offset_x = (960 - target_size.x * scale_x) * 0.5f;
            offset_y = (544 - target_size.y * scale_y) * 0.5f;
        }
        static unsigned canvas_calls = 0;
        if (canvas_calls++ < 12) {
            fprintf(stderr, "[gxm2d] canvas target=%llu screen=%d viewport=%dx%d native=960x544 items=1 scale=(%.3f,%.3f)\n",
                    (unsigned long long)target.get_id(), storage->is_screen(target),
                    target_size.x, target_size.y, scale_x, scale_y);
            fflush(stderr);
        }
        drawing_screen_target = false;
        if (!offscreen) {
            process_pending_shader();
            const uint32_t clear = gxm_color(storage->render_target_get_clear_request_color(target));
            // Allocate the expensive screen capture targets only after at least
            // one screen-reading program has compiled successfully.
            if (materials && native_screen_shader_ready &&
                    vita_gxm_canvas_shader_compiler_enabled()) {
                drawing_screen_target = vita_gxm_canvas_screen_begin(clear) != 0;
            }
            if (!drawing_screen_target) vita_gxm_begin(clear);
            storage->render_target_disable_clear_request(target);
        }
        const uint64_t started_us = OS::get_singleton()->get_ticks_usec();
        const double now = double(started_us) / 1000000.0;
        for (Item *item = items; item; item = item->next) {
            prepare_material(item);
            // A screen-reading ColorRect often covers the entire scene.
            // Without a compiled fragment shader its raw white draw command
            // hides the UI underneath (Split Memories opening-menu regression).
            // Keep it transparent until native GXP has compiled. This is an
            // engine-level fail-safe, not a replacement for the original shader.
            if (materials) {
                const RID material = item->material.is_valid() ? item->material :
                        (item->material_owner ? item->material_owner->material : RID());
                const RID shader = materials->shader_for_material(material);
                if (shader.is_valid() && active_shader <= 0 &&
                        materials->shader_get_code(shader).contains("hint_screen_texture")) {
                    if (!diagnosed_shaders.has(shader)) {
                        diagnosed_shaders.insert(shader);
                        fprintf(stderr, "[gxm2d-shader] transparency fallback: screen sampler unavailable path=%s\n",
                                materials->shader_path(shader).utf8().get_data());
                        fflush(stderr);
                    }
                    continue;
                }
            }
            if (item->material.is_valid() || (item->material_owner && item->material_owner->material.is_valid()) || item->canvas_group_owner) {
                // Shader/material effects are unsupported, but discarding the
                // whole item makes normal sprites invisible. Draw the unshaded
                // command as a fallback; transparencies still use vertex tint.
                if (materials) {
                    const RID material = item->material.is_valid() ? item->material :
                            (item->material_owner ? item->material_owner->material : RID());
                    const RID shader = materials->shader_for_material(material);
                    if (shader.is_valid() && !diagnosed_shaders.has(shader)) {
                        diagnosed_shaders.insert(shader);
                        const String source = materials->shader_get_code(shader);
                        const bool screen = source.contains("hint_screen_texture");
                        const bool timed = source.contains("TIME");
                        fprintf(stderr, "[gxm2d-shader] path=%s bytes=%d screen=%d time=%d status=not-compiled\n",
                                materials->shader_path(shader).utf8().get_data(),
                                source.utf8().length(), screen, timed);
                        fflush(stderr);
                    }
                }
                WARN_PRINT_ONCE("GXM: drawing canvas item without unsupported material/group effects.");
            }
            Rect2i clip = item->final_clip_owner ? Rect2i(item->final_clip_owner->final_clip_rect) : Rect2i();
            if (item->final_clip_owner && !clip.has_area()) continue;
            auto apply_clip = [&](bool enabled) {
                vita_gxm_clip(enabled,
                        (int)(offset_x + clip.position.x * scale_x), (int)(offset_y + clip.position.y * scale_y),
                        (int)(clip.size.x * scale_x), (int)(clip.size.y * scale_y));
            };
            apply_clip(item->final_clip_owner != nullptr);
            Transform2D extra;
            bool skipping = false;
            for (Item::Command *command = item->commands; command; command = command->next) {
                if (command->type == Item::Command::TYPE_TRANSFORM) {
                    extra = static_cast<Item::CommandTransform *>(command)->xform;
                    continue;
                }
                if (command->type == Item::Command::TYPE_ANIMATION_SLICE) {
                    // Animated tiles: only the frame whose slice holds the current time is drawn.
                    const Item::CommandAnimationSlice *as = static_cast<Item::CommandAnimationSlice *>(command);
                    if (as->animation_length > 0) {
                        double local = Math::fposmod(now - as->offset, as->animation_length);
                        skipping = !(local >= as->slice_begin && local < as->slice_end);
                    } else {
                        skipping = false;
                    }
                    continue;
                }
                if (skipping) continue;
                if (command->type == Item::Command::TYPE_CLIP_IGNORE) {
                    const bool ignore = static_cast<Item::CommandClipIgnore *>(command)->ignore;
                    apply_clip(!ignore && item->final_clip_owner != nullptr);
                    continue;
                }
                const Transform2D xf = item->final_transform * extra;
                switch (command->type) {
                    case Item::Command::TYPE_RECT:
                        draw_rect_command(item, static_cast<Item::CommandRect *>(command), xf, modulate, default_filter);
                        break;
                    case Item::Command::TYPE_NINEPATCH:
                        draw_ninepatch(item, static_cast<Item::CommandNinePatch *>(command), xf, modulate, default_filter);
                        break;
                    case Item::Command::TYPE_POLYGON:
                        draw_polygon(item, static_cast<Item::CommandPolygon *>(command), xf, modulate, default_filter);
                        break;
                    case Item::Command::TYPE_PRIMITIVE:
                        draw_primitive(item, static_cast<Item::CommandPrimitive *>(command), xf, modulate, default_filter);
                        break;
                    default:
                        WARN_PRINT_ONCE("GXM: mesh, multimesh and particle canvas commands are not implemented.");
                        break;
                }
            }
        }
        for (Item *item = items; item; item = item->next) perf_items++;
        perf_canvas_us += OS::get_singleton()->get_ticks_usec() - started_us;
        if (++perf_calls >= 300) {
            fprintf(stderr, "[gxm2d-canvas] calls=%u avg_canvas_us=%llu avg_items=%u\n", perf_calls,
                    (unsigned long long)(perf_canvas_us / perf_calls), perf_items / perf_calls);
            fflush(stderr);
            perf_calls = perf_items = 0;
            perf_canvas_us = 0;
        }
        if (offscreen) {
            vita_gxm_target_end();
        } else {
            vita_gxm_clip(0, 0, 0, 0, 0);
        }
    }
};
