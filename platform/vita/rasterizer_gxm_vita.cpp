#include "rasterizer_gxm_vita.h"
#include "gxm_bridge.h"
#include "core/config/project_settings.h"
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
        // Offscreen targets (SubViewport) render into a GXM texture.
        RID texture;
        void *gpu = nullptr;
        Size2i gpu_size;
    };
    HashMap<RID, RID> rt_textures; // texture RID -> render target RID
    mutable RID_Owner<Target> targets;
    RID screen;
    struct Texture {
        Ref<Image> image;
        Image::Format format = Image::FORMAT_MAX;
        Size2i size;
        void *gpu = nullptr;
        unsigned long long failed_epoch = ~0ULL;
        bool has_alpha = false;
    };
    struct CanvasTexture {
        RID diffuse;
        RSE::CanvasItemTextureFilter filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
    };
    HashMap<RID, Texture> textures;
    HashMap<RID, RID> proxies;
    HashMap<RID, CanvasTexture> canvas_textures;
    unsigned long long allocation_epoch = 0;
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
        if (old && old->gpu) {
            vita_gxm_texture_free(old->gpu);
            allocation_epoch++;
        }
        Texture t;
        t.image = rgba;
        t.format = image->get_format();
        t.size = Size2i(image->get_width(), image->get_height());
        // Cut-out and blended textures skip the multiplicative lighting pass,
        // which would also darken whatever shows through their holes.
        t.has_alpha = rgba->detect_alpha() != Image::ALPHA_NONE;
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
        RID base = resolve(rid);
        const RID *target = rt_textures.getptr(base);
        if (target) {
            const Target *rt = targets.get_or_null(*target);
            return rt ? Size2(rt->size) : Size2();
        }
        const Texture *t = textures.getptr(base);
        return t ? Size2(t->size) : Size2();
    }
    bool is_render_target_texture(RID rid) const { return rt_textures.has(resolve(rid)); }
    // GPU surface of an offscreen target, (re)created to match its size.
    void *target_gpu(RID rid) {
        Target *t = targets.get_or_null(rid);
        if (!t || t->size.x < 1 || t->size.y < 1) return nullptr;
        if (t->gpu && t->gpu_size != t->size) {
            vita_gxm_texture_free(t->gpu);
            t->gpu = nullptr;
        }
        if (!t->gpu) {
            t->gpu = vita_gxm_target_create(t->size.x, t->size.y);
            t->gpu_size = t->size;
            fprintf(stderr, "[gxm2d] offscreen target %dx%d %s\n", t->size.x, t->size.y, t->gpu ? "created" : "FAILED");
            fflush(stderr);
        }
        return t->gpu;
    }
    RID render_target_get_texture(RID rid) override {
        Target *t = targets.get_or_null(rid);
        ERR_FAIL_NULL_V(t, RID());
        if (t->texture.is_null()) {
            t->texture = texture_allocate();
            rt_textures.insert(t->texture, rid);
        }
        return t->texture;
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
        if (t && t->gpu) {
            vita_gxm_texture_free(t->gpu);
            allocation_epoch++;
        }
        textures.erase(rid);
        proxies.erase(rid);
        canvas_textures.erase(rid);
        if (rt_textures.has(rid)) {
            // Owned by its render target; released in render_target_free.
            return;
        }
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
    bool texture_has_alpha(RID rid) const {
        const Texture *t = textures.getptr(resolve(rid));
        return t && t->has_alpha;
    }
    void *get_gpu(RID rid) {
        RID base = resolve(rid);
        const RID *target = rt_textures.getptr(base);
        if (target) {
            Target *rt = targets.get_or_null(*target);
            return rt ? rt->gpu : nullptr;
        }
        Texture *t = textures.getptr(base);
        if (!t || t->image.is_null()) return nullptr;
        if (!t->gpu) {
            // When the GPU budget is exhausted, suppress *per-frame* retries
            // for the same texture until something has been freed. Previously
            // 60,000+ errors tanked FPS and caused log writes each frame.
            if (t->failed_epoch == allocation_epoch) return nullptr;
            t->gpu = vita_gxm_texture_create(t->image->get_width(),
                    t->image->get_height(), t->image->get_data().ptr());
            if (!t->gpu) {
                t->failed_epoch = allocation_epoch;
                static unsigned failed_once;
                if (failed_once++ < 8) {
                    fprintf(stderr, "[gxm2d] GPU budget limit: skipped texture %dx%d (no immediate retry)\n",
                            t->image->get_width(), t->image->get_height());
                    fflush(stderr);
                }
                return nullptr;
            }
        }
        return t->gpu;
    }
    // Decals: stored so the 3D path can draw them as textured quads on the
    // projection plane. No real projection onto curved geometry.
    struct Decal {
        Vector3 size = Vector3(2, 2, 2);
        RID albedo;
        Color modulate = Color(1, 1, 1, 1);
        float albedo_mix = 1.0f;
        uint32_t cull_mask = 0xFFFFFFFF;
    };
    struct DecalInstance {
        RID decal;
        Transform3D transform;
    };
    mutable RID_Owner<Decal, true> decals;
    mutable RID_Owner<DecalInstance> decal_instances;
    bool owns_decal(RID rid) const { return decals.owns(rid); }
    const Decal *get_decal(RID rid) const { return decals.get_or_null(rid); }
    const DecalInstance *get_decal_instance(RID rid) const { return decal_instances.get_or_null(rid); }
    RID decal_allocate() override { return decals.allocate_rid(); }
    void decal_initialize(RID rid) override { decals.initialize_rid(rid); }
    void decal_free(RID rid) override { if (decals.owns(rid)) decals.free(rid); }
    void decal_set_size(RID rid, const Vector3 &size) override { Decal *d = decals.get_or_null(rid); if (d) d->size = size; }
    void decal_set_texture(RID rid, RSE::DecalTexture type, RID texture) override {
        Decal *d = decals.get_or_null(rid);
        if (d && type == RSE::DECAL_TEXTURE_ALBEDO) d->albedo = texture;
    }
    void decal_set_albedo_mix(RID rid, float mix) override { Decal *d = decals.get_or_null(rid); if (d) d->albedo_mix = mix; }
    void decal_set_modulate(RID rid, const Color &modulate) override { Decal *d = decals.get_or_null(rid); if (d) d->modulate = modulate; }
    void decal_set_cull_mask(RID rid, uint32_t layers) override { Decal *d = decals.get_or_null(rid); if (d) d->cull_mask = layers; }
    AABB decal_get_aabb(RID rid) const override {
        const Decal *d = decals.get_or_null(rid);
        return d ? AABB(-d->size * 0.5f, d->size) : AABB();
    }
    uint32_t decal_get_cull_mask(RID rid) const override {
        const Decal *d = decals.get_or_null(rid);
        return d ? d->cull_mask : 0;
    }
    RID decal_instance_create(RID decal) override {
        DecalInstance instance;
        instance.decal = decal;
        return decal_instances.make_rid(instance);
    }
    void decal_instance_free(RID rid) override { if (decal_instances.owns(rid)) decal_instances.free(rid); }
    void decal_instance_set_transform(RID rid, const Transform3D &transform) override {
        DecalInstance *instance = decal_instances.get_or_null(rid);
        if (instance) instance->transform = transform;
    }
    RID render_target_create() override {
        RID rid = targets.make_rid(Target());
        if (screen.is_null()) screen = rid;
        return rid;
    }
    void render_target_free(RID rid) override {
        Target *t = targets.get_or_null(rid);
        if (t) {
            if (t->gpu) vita_gxm_texture_free(t->gpu);
            if (t->texture.is_valid()) {
                rt_textures.erase(t->texture);
                RendererDummy::TextureStorage::texture_free(t->texture);
            }
        }
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

#include "rasterizer_scene_gxm_vita.inc"

#include "gxm_canvas_vita.inc"

RasterizerGXMVita::RasterizerGXMVita() {
    memdelete(canvas);
    memdelete(texture_storage);
    auto *gxm_storage = memnew(GXMTextureStorage);
    texture_storage = gxm_storage;
    canvas = memnew(GXMCanvas(gxm_storage));
    FILE *experimental = fopen("app0:gxm3d.cfg", "rb");
    if (experimental) {
        fclose(experimental);
        memdelete(scene);
        memdelete(mesh_storage);
        memdelete(material_storage);
        memdelete(light_storage);
        auto *gxm_meshes = memnew(GXMMeshStorage);
        auto *gxm_materials = memnew(GXMMaterialStorage);
        auto *gxm_lights = memnew(GXMLightStorage);
        mesh_storage = gxm_meshes;
        material_storage = gxm_materials;
        light_storage = gxm_lights;
        memdelete(utilities);
        utilities = memnew(GXMUtilities(gxm_meshes, gxm_lights, gxm_storage));
        scene = memnew(GXMScene(gxm_storage, gxm_meshes, gxm_materials, gxm_lights));
        fprintf(stderr, "[gxm3d] experimental static mesh renderer enabled\n");
    }
    fprintf(stderr, "[gxm2d] native canvas v3: RGBA8->RGBA4444 loader upload; flexible viewport canvas; ninepatch/polygon/primitive; no GLES shaders\n");
    fflush(stderr);
}
void RasterizerGXMVita::set_boot_image_with_stretch(const Ref<Image> &p_image, const Color &p_color, RSE::SplashStretchMode p_stretch_mode, bool p_use_filter) {
    vita_gxm_begin(gxm_color(Color(p_color.r, p_color.g, p_color.b, 1.0f)));
    void *texture = nullptr;
    if (p_image.is_valid() && !p_image->is_empty() && p_image->get_width() > 1) {
        Ref<Image> image = p_image->duplicate();
        if (image->is_compressed()) image->decompress();
        image->convert(Image::FORMAT_RGBA8);
        const Vector2 size(image->get_width(), image->get_height());
        texture = vita_gxm_texture_create(image->get_width(), image->get_height(), image->get_data().ptr());
        if (texture) {
            float scale = MIN(960.0f / size.x, 544.0f / size.y);
            if (p_stretch_mode == RSE::SPLASH_STRETCH_MODE_DISABLED) {
                scale = MIN(scale, 1.0f);
            } else if (!p_use_filter && scale >= 1.0f) {
                scale = Math::floor(scale); // Pixel art: whole-pixel scaling.
            }
            const Vector2 extent = size * scale, origin = (Vector2(960, 544) - extent) * 0.5f;
            const float xy[8] = { origin.x, origin.y, origin.x + extent.x, origin.y, origin.x + extent.x, origin.y + extent.y, origin.x, origin.y + extent.y };
            const float uv[8] = { 0, 0, 1, 0, 1, 1, 0, 1 };
            vita_gxm_texture_quad(texture, xy, uv, 0xffffffff, p_use_filter);
        }
    }
    vita_gxm_end(1);
    if (texture) vita_gxm_texture_free(texture);
    fprintf(stderr, "[gxm2d] boot splash shown image=%dx%d\n", p_image.is_valid() ? p_image->get_width() : 0, p_image.is_valid() ? p_image->get_height() : 0);
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
