#include "rasterizer_gxm_vita.h"
#include "gxm_bridge.h"
#include "servers/rendering/dummy/rasterizer_canvas_dummy.h"
#include "servers/rendering/dummy/storage/texture_storage.h"
#include <stdio.h>

static uint32_t gxm_color(const Color &p_color) {
    Color c = p_color.clamp();
    return uint32_t(c.r * 255.0f + 0.5f) | (uint32_t(c.g * 255.0f + 0.5f) << 8) |
            (uint32_t(c.b * 255.0f + 0.5f) << 16) | (uint32_t(c.a * 255.0f + 0.5f) << 24);
}

class GXMTextureStorage : public RendererDummy::TextureStorage {
    struct Target {
        Size2i size;
        Color clear = Color(0, 0, 0, 1);
        bool pending_clear = true;
    };
    mutable RID_Owner<Target> targets;
    RID screen;
    struct Texture {
        Ref<Image> image;
        Image::Format format = Image::FORMAT_MAX;
        Size2i size;
        void *gpu = nullptr;
    };
    struct CanvasTexture {
        RID diffuse;
        RSE::CanvasItemTextureFilter filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
    };
    HashMap<RID, Texture> textures;
    HashMap<RID, RID> proxies;
    HashMap<RID, CanvasTexture> canvas_textures;
public:
    void release_gpu() {
        for (KeyValue<RID, Texture> &entry : textures) {
            vita_gxm_texture_free(entry.value.gpu);
            entry.value.gpu = nullptr;
        }
    }
    RID resolve(RID rid) const {
        for (int i = 0; i < 16; i++) {
            const CanvasTexture *ct = canvas_textures.getptr(rid);
            if (ct) { rid = ct->diffuse; continue; }
            const RID *proxy = proxies.getptr(rid);
            if (proxy) { rid = *proxy; continue; }
            return rid;
        }
        return RID(); // Invalid/cyclic proxy chain.
    }
    void texture_2d_initialize(RID rid, const Ref<Image> &image) override {
        ERR_FAIL_COND(!owns_texture(rid) || image.is_null() || image->is_empty());
        Ref<Image> rgba = image->duplicate();
        if (rgba->is_compressed()) {
            ERR_FAIL_COND_MSG(rgba->decompress() != OK, "GXM: image decompression failed.");
        }
        rgba->convert(Image::FORMAT_RGBA8);
        Texture *old = textures.getptr(rid);
        if (old) vita_gxm_texture_free(old->gpu);
        Texture t;
        t.image = rgba;
        t.format = image->get_format();
        t.size = Size2i(image->get_width(), image->get_height());
        textures.insert(rid, t);
    }
    void texture_2d_update(RID rid, const Ref<Image> &image, int layer = 0) override {
        ERR_FAIL_COND(layer != 0);
        texture_2d_initialize(rid, image);
    }
    Ref<Image> texture_2d_get(RID rid) const override {
        const Texture *t = textures.getptr(resolve(rid));
        return t ? t->image : Ref<Image>();
    }
    Image::Format texture_get_format(RID rid) const override {
        const Texture *t = textures.getptr(resolve(rid));
        return t ? t->format : Image::FORMAT_MAX;
    }
    Size2 texture_size_with_proxy(RID rid) override {
        const Texture *t = textures.getptr(resolve(rid));
        return t ? Size2(t->size) : Size2();
    }
    void texture_set_size_override(RID rid, int width, int height) override {
        Texture *t = textures.getptr(resolve(rid));
        if (t) t->size = Size2i(width, height);
    }
    void texture_proxy_initialize(RID rid, RID base) override {
        ERR_FAIL_COND(rid == base || !owns_texture(rid));
        proxies.insert(rid, base);
    }
    void texture_proxy_update(RID rid, RID base) override { texture_proxy_initialize(rid, base); }
    void texture_free(RID rid) override {
        Texture *t = textures.getptr(rid);
        if (t) vita_gxm_texture_free(t->gpu);
        textures.erase(rid);
        proxies.erase(rid);
        canvas_textures.erase(rid);
        RendererDummy::TextureStorage::texture_free(rid);
    }
    void texture_replace(RID rid, RID by) override {
        ERR_FAIL_COND(rid == by || !owns_texture(rid) || !owns_texture(by));
        Texture *source = textures.getptr(by);
        ERR_FAIL_NULL(source);
        Texture moved = *source;
        Texture *old = textures.getptr(rid);
        if (old) vita_gxm_texture_free(old->gpu);
        textures.erase(by);
        textures.insert(rid, moved);
        proxies.erase(rid);
        texture_free(by);
    }
    RID canvas_texture_allocate() override {
        RID rid = texture_allocate();
        canvas_textures.insert(rid, CanvasTexture());
        return rid;
    }
    void canvas_texture_free(RID rid) override { texture_free(rid); }
    void canvas_texture_set_channel(RID rid, RSE::CanvasTextureChannel channel, RID texture) override {
        CanvasTexture *ct = canvas_textures.getptr(rid);
        ERR_FAIL_NULL(ct);
        if (channel == RSE::CANVAS_TEXTURE_CHANNEL_DIFFUSE) ct->diffuse = texture;
    }
    void canvas_texture_set_texture_filter(RID rid, RSE::CanvasItemTextureFilter filter) override {
        CanvasTexture *ct = canvas_textures.getptr(rid);
        if (ct) ct->filter = filter;
    }
    RSE::CanvasItemTextureFilter texture_filter(RID rid, RSE::CanvasItemTextureFilter fallback) const {
        const CanvasTexture *ct = canvas_textures.getptr(rid);
        return ct && ct->filter != RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? ct->filter : fallback;
    }
    void *get_gpu(RID rid) {
        Texture *t = textures.getptr(resolve(rid));
        if (!t || t->image.is_null()) return nullptr;
        if (!t->gpu) {
            t->gpu = vita_gxm_texture_create(t->image->get_width(), t->image->get_height(), t->image->get_data().ptr());
            ERR_FAIL_NULL_V_MSG(t->gpu, nullptr, "GXM texture allocation failed (48 MiB budget).");
        }
        return t->gpu;
    }
    RID render_target_create() override {
        RID rid = targets.make_rid(Target());
        if (screen.is_null()) screen = rid;
        return rid;
    }
    void render_target_free(RID rid) override {
        if (targets.owns(rid)) targets.free(rid);
        if (screen == rid) screen = RID();
    }
    bool is_screen(RID rid) const { return rid == screen; }
    void render_target_set_size(RID rid, int w, int h, uint32_t views) override {
        Target *t = targets.get_or_null(rid);
        ERR_FAIL_NULL(t);
        t->size = Size2i(w, h);
        if (views > 1) {
            WARN_PRINT_ONCE("GXM prototype supports one view only.");
        }
    }
    Size2i render_target_get_size(RID rid) const override {
        const Target *t = targets.get_or_null(rid);
        return t ? t->size : Size2i();
    }
    void render_target_request_clear(RID rid, const Color &c) override {
        Target *t = targets.get_or_null(rid);
        ERR_FAIL_NULL(t);
        t->clear = c; t->pending_clear = true;
    }
    bool render_target_is_clear_requested(RID rid) override {
        const Target *t = targets.get_or_null(rid);
        return t && t->pending_clear;
    }
    Color render_target_get_clear_request_color(RID rid) override {
        const Target *t = targets.get_or_null(rid);
        return t ? t->clear : Color(0, 0, 0, 1);
    }
    void render_target_disable_clear_request(RID rid) override {
        Target *t = targets.get_or_null(rid);
        if (t) t->pending_clear = false;
    }
    void render_target_do_clear_request(RID rid) override {
        if (!is_screen(rid)) return;
        vita_gxm_begin(gxm_color(render_target_get_clear_request_color(rid)));
        render_target_disable_clear_request(rid);
    }
};

class GXMCanvas : public RasterizerCanvasDummy {
    GXMTextureStorage *storage;
    unsigned int logged_rects = 0;
public:
    explicit GXMCanvas(GXMTextureStorage *p_storage) : storage(p_storage) {}
    void canvas_render_items(RID target, Item *items, const Color &modulate, Light *, Light *,
            const Transform2D &, RSE::CanvasItemTextureFilter default_filter, RSE::CanvasItemTextureRepeat,
            bool snap, bool &sdf_used, RenderingServerTypes::RenderInfo * = nullptr) override {
        sdf_used = false;
        if (!storage->is_screen(target) || storage->render_target_get_size(target) != Size2i(960, 544)) {
            WARN_PRINT_ONCE("GXM prototype requires one 960x544 screen viewport.");
            return;
        }
        vita_gxm_begin(gxm_color(storage->render_target_get_clear_request_color(target)));
        storage->render_target_disable_clear_request(target);
        for (Item *item = items; item; item = item->next) {
            if (item->material.is_valid() || (item->material_owner && item->material_owner->material.is_valid()) || item->canvas_group_owner) {
                WARN_PRINT_ONCE("GXM prototype skips custom materials and canvas groups.");
                continue;
            }
            Rect2i clip = item->final_clip_owner ? Rect2i(item->final_clip_owner->final_clip_rect) : Rect2i();
            if (item->final_clip_owner && !clip.has_area()) continue;
            vita_gxm_clip(item->final_clip_owner != nullptr, clip.position.x, clip.position.y, clip.size.x, clip.size.y);
            Transform2D extra;
            for (Item::Command *command = item->commands; command; command = command->next) {
                if (command->type == Item::Command::TYPE_TRANSFORM) {
                    extra = static_cast<Item::CommandTransform *>(command)->xform;
                    continue;
                }
                if (command->type != Item::Command::TYPE_RECT) {
                    WARN_PRINT_ONCE("GXM prototype supports ColorRect commands only.");
                    continue;
                }
                const Item::CommandRect *rect = static_cast<Item::CommandRect *>(command);
                if (rect->flags & (CANVAS_RECT_TILE | CANVAS_RECT_MSDF | CANVAS_RECT_LCD)) {
                    WARN_PRINT_ONCE("GXM: tiled, MSDF and LCD rectangles are not implemented.");
                    continue;
                }
                Transform2D transform = item->final_transform * extra;
                Vector2 points[4] = {rect->rect.position, rect->rect.position + Vector2(rect->rect.size.x, 0),
                        rect->rect.position + rect->rect.size, rect->rect.position + Vector2(0, rect->rect.size.y)};
                float xy[8];
                for (int i = 0; i < 4; i++) {
                    Vector2 p = transform.xform(points[i]);
                    if (snap) p = p.floor();
                    xy[2 * i] = p.x; xy[2 * i + 1] = p.y;
                }
                Color color = rect->modulate * item->final_modulate * modulate;
                if (rect->texture.is_valid()) {
                    Size2 size = storage->texture_size_with_proxy(rect->texture);
                    if (size.x <= 0 || size.y <= 0) continue;
                    void *gpu = storage->get_gpu(rect->texture);
                    if (!gpu) continue;
                    Rect2 src = (rect->flags & CANVAS_RECT_REGION) ? rect->source : Rect2(Vector2(), size);
                    float uv[8];
                    const Vector2 corners[4] = {Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1)};
                    for (int i = 0; i < 4; i++) {
                        Vector2 c = corners[i];
                        if (rect->flags & CANVAS_RECT_TRANSPOSE) SWAP(c.x, c.y);
                        if (rect->flags & CANVAS_RECT_FLIP_H) c.x = 1.0f - c.x;
                        if (rect->flags & CANVAS_RECT_FLIP_V) c.y = 1.0f - c.y;
                        Vector2 p = (src.position + c * src.size) / size;
                        uv[2 * i] = p.x; uv[2 * i + 1] = p.y;
                    }
                    auto filter = item->texture_filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? default_filter : item->texture_filter;
                    filter = storage->texture_filter(rect->texture, filter);
                    bool linear = filter != RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST && filter != RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS;
                    vita_gxm_texture_quad(gpu, xy, uv, gxm_color(color), linear);
                } else {
                    vita_gxm_quad(xy, gxm_color(color));
                }
                if (logged_rects++ < 3) {
                    fprintf(stderr, "[gxm2d] rect submitted xy=(%.1f,%.1f) rgba=%08lx\n", xy[0], xy[1], (unsigned long)gxm_color(color));
                    fflush(stderr);
                }
            }
        }
        vita_gxm_clip(0, 0, 0, 0, 0);
    }
};

RasterizerGXMVita::RasterizerGXMVita() {
    memdelete(canvas);
    memdelete(texture_storage);
    auto *gxm_storage = memnew(GXMTextureStorage);
    texture_storage = gxm_storage;
    canvas = memnew(GXMCanvas(gxm_storage));
    fprintf(stderr, "[gxm2d] native canvas v2: RGBA textures and bitmap font atlases; no GLES shaders\n");
    fflush(stderr);
}
void RasterizerGXMVita::end_frame(bool present) {
    vita_gxm_begin(0xff000000);
    vita_gxm_end(present);
}
RasterizerGXMVita::~RasterizerGXMVita() {
    vita_gxm_end(0);
    static_cast<GXMTextureStorage *>(texture_storage)->release_gpu();
    vita_gxm_finish();
}
