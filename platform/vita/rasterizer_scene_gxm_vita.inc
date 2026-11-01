// Experimental static-mesh scene path. Included after GXMTextureStorage.
#include "servers/rendering/dummy/rasterizer_scene_dummy.h"
#include "servers/rendering/dummy/storage/mesh_storage.h"
#include "servers/rendering/dummy/storage/material_storage.h"
#include "servers/rendering/dummy/storage/light_storage.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/dummy/storage/utilities.h"
#include "core/os/os.h"
#include "core/config/project_settings.h"
#include "core/templates/hash_set.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/sort_array.h"
#include "gxm_camera_lod.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/3d/world_3d.h"
#include "servers/physics_3d/physics_server_3d.h"
#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"

class GXMMeshStorage : public RendererDummy::MeshStorage {
    struct Skeleton {
        Vector<Transform3D> bones;
        Dependency dependency;
        uint64_t version = 1;
    };
    mutable RID_Owner<Skeleton> skeletons;
    HashMap<RID, HashMap<int, Array>> decoded;
    // Bound CPU cache independently from the loader's CDRAM texture budget.
    uint64_t cache_bytes = 0;
public:
    // Static geometry flattened once. `gpu` lives in the bridge's mapped
    // arena and is drawn without per-frame vertex conversion; xyz/uv are
    // only kept when the arena is full or unavailable (CPU fallback).
    struct FlatSurface {
        void *gpu = nullptr;
        unsigned count = 0;
        Vector<float> xyz, uv;
        // Local positions and normals in the same flattened order, kept only
        // for surfaces that receive per-vertex lighting.
        Vector<Vector3> positions, normals;
        Color tint = Color(1, 1, 1, 1); // Vertex-color tint
    };
    struct SurfaceInfo {
        bool triangles = false;
        unsigned triangle_count = 0;
        RID material;
    };
    struct MeshInfo {
        Vector<SurfaceInfo> surfaces;
        unsigned triangle_count = 0;
    };
    unsigned invalidations = 0;
    bool arena_exhausted = false;
private:
    HashMap<RID, HashMap<int, FlatSurface>> flat;
    HashMap<RID, MeshInfo> info;
    uint64_t flat_bytes = 0;
public:
    // Cached per-mesh metadata: mesh_get_surface() copies the whole
    // SurfaceData, which is too expensive to do per surface per frame.
    const MeshInfo &get_info(RID rid) {
        MeshInfo *cached = info.getptr(rid);
        if (cached) return *cached;
        MeshInfo value;
        int count = mesh_get_surface_count(rid);
        value.surfaces.resize(count);
        for (int s = 0; s < count; s++) {
            auto surface = mesh_get_surface(rid, s);
            SurfaceInfo &entry = value.surfaces.write[s];
            entry.triangles = surface.primitive == RSE::PRIMITIVE_TRIANGLES;
            entry.triangle_count = entry.triangles ? unsigned(surface.index_count ? surface.index_count : surface.vertex_count) / 3 : 0;
            entry.material = surface.material;
            value.triangle_count += entry.triangle_count;
        }
        return info.insert(rid, value)->value;
    }
    const FlatSurface *get_flat(RID rid, int surface, const PackedVector3Array &positions, const PackedVector2Array &uvs, const PackedInt32Array &indices) {
        auto *entry = flat.getptr(rid);
        if (entry && entry->has(surface)) return &(*entry)[surface];
        int count = indices.is_empty() ? positions.size() : indices.size();
        count -= count % 3;
        if (!count) return nullptr;
        Vector<float> xyz, uv;
        xyz.resize(count * 3);
        uv.resize(count * 2);
        float *xyz_w = xyz.ptrw();
        float *uv_w = uv.ptrw();
        const Vector3 *source = positions.ptr();
        const Vector2 *source_uv = uvs.ptr();
        const int32_t *source_index = indices.ptr();
        for (int i = 0; i < count; i++) {
            int vertex = indices.is_empty() ? i : source_index[i];
            if (vertex < 0 || vertex >= positions.size()) return nullptr;
            xyz_w[i * 3] = source[vertex].x;
            xyz_w[i * 3 + 1] = source[vertex].y;
            xyz_w[i * 3 + 2] = source[vertex].z;
            uv_w[i * 2] = vertex < uvs.size() ? source_uv[vertex].x : 0;
            uv_w[i * 2 + 1] = vertex < uvs.size() ? source_uv[vertex].y : 0;
        }
        FlatSurface value;
        value.count = count;
        value.gpu = vita_gxm_mesh_create(xyz.ptr(), uv.ptr(), count);
        if (!value.gpu && vita_gxm_mesh_arena_used() > 0) arena_exhausted = true;
        if (!value.gpu) {
            uint64_t bytes = uint64_t(count) * 5 * sizeof(float);
            if (flat_bytes + bytes > 32 * 1024 * 1024) return nullptr;
            value.xyz = xyz;
            value.uv = uv;
            flat_bytes += bytes;
        }
        flat[rid][surface] = value;
        return &flat[rid][surface];
    }
    const FlatSurface *get_flat_geometry(RID rid, int surface) {
        auto *entry = flat.getptr(rid);
        FlatSurface *value = entry ? entry->getptr(surface) : nullptr;
        if (!value) return nullptr;
        if (!value->positions.is_empty()) return value;
        Array arrays = get_decoded(rid, surface);
        if (arrays.is_empty()) return nullptr;
        PackedVector3Array positions = arrays[RSE::ARRAY_VERTEX];
        PackedVector3Array normals = arrays[RSE::ARRAY_NORMAL];
        PackedInt32Array indices = arrays[RSE::ARRAY_INDEX];
        int count = indices.is_empty() ? positions.size() : indices.size();
        count -= count % 3;
        if (unsigned(count) != value->count) return nullptr;
        value->positions.resize(count);
        value->normals.resize(normals.size() == positions.size() ? count : 0);
        Vector3 *out_p = value->positions.ptrw();
        Vector3 *out_n = value->normals.is_empty() ? nullptr : value->normals.ptrw();
        for (int i = 0; i < count; i++) {
            int vertex = indices.is_empty() ? i : indices[i];
            if (vertex < 0 || vertex >= positions.size()) vertex = 0;
            out_p[i] = positions[vertex];
            if (out_n) out_n[i] = normals[vertex];
        }
        flat_bytes += uint64_t(count) * 24;
        return value;
    }
    bool owns_skeleton(RID rid) const { return skeletons.owns(rid); }
    uint64_t skeleton_version(RID rid) const {
        const Skeleton *s = skeletons.get_or_null(rid);
        return s ? s->version : 0;
    }
    const Vector<Transform3D> *skeleton_bones(RID rid) const {
        const Skeleton *s = skeletons.get_or_null(rid);
        return s ? &s->bones : nullptr;
    }
    RID skeleton_allocate() override { return skeletons.allocate_rid(); }
    void skeleton_initialize(RID rid) override { skeletons.initialize_rid(rid); }
    void skeleton_allocate_data(RID rid, int count, bool is_2d = false) override {
        Skeleton *s = skeletons.get_or_null(rid);
        ERR_FAIL_NULL(s);
        ERR_FAIL_COND(count < 0 || is_2d);
        s->bones.resize(count);
        s->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_SKELETON_DATA);
    }
    int skeleton_get_bone_count(RID rid) const override {
        const Skeleton *s = skeletons.get_or_null(rid);
        return s ? s->bones.size() : 0;
    }
    void skeleton_bone_set_transform(RID rid, int bone, const Transform3D &transform) override {
        Skeleton *s = skeletons.get_or_null(rid);
        ERR_FAIL_NULL(s);
        ERR_FAIL_INDEX(bone, s->bones.size());
        s->bones.write[bone] = transform;
        s->version++;
        s->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_SKELETON_BONES);
    }
    Transform3D skeleton_bone_get_transform(RID rid, int bone) const override {
        const Skeleton *s = skeletons.get_or_null(rid);
        return s && bone >= 0 && bone < s->bones.size() ? s->bones[bone] : Transform3D();
    }
    void skeleton_update_dependency(RID rid, DependencyTracker *tracker) override {
        Skeleton *s = skeletons.get_or_null(rid);
        if (s) tracker->update_dependency(&s->dependency);
    }
    void skeleton_free(RID rid) override {
        Skeleton *s = skeletons.get_or_null(rid);
        ERR_FAIL_NULL(s);
        s->dependency.deleted_notify(rid);
        skeletons.free(rid);
    }
    // Only the changed mesh is dropped. Clearing every cache on each mesh
    // change re-decoded the whole level whenever a script touched one mesh.
    // GPU arena space of a dropped mesh is reclaimed when the arena fills.
    void invalidate_mesh(RID rid) {
        invalidations++;
        auto *mesh = decoded.getptr(rid);
        if (mesh) {
            for (const KeyValue<int, Array> &surface : *mesh) {
                PackedVector3Array positions = surface.value.is_empty() ? PackedVector3Array() : PackedVector3Array(surface.value[RSE::ARRAY_VERTEX]);
                uint64_t bytes = uint64_t(positions.size()) * 256 + uint64_t(PackedInt32Array(surface.value[RSE::ARRAY_INDEX]).size()) * 4;
                cache_bytes = cache_bytes > bytes ? cache_bytes - bytes : 0;
            }
            decoded.erase(rid);
        }
        auto *surfaces = flat.getptr(rid);
        if (surfaces) {
            for (const KeyValue<int, FlatSurface> &surface : *surfaces) {
                uint64_t bytes = uint64_t(surface.value.xyz.size() + surface.value.uv.size()) * sizeof(float);
                flat_bytes = flat_bytes > bytes ? flat_bytes - bytes : 0;
            }
            flat.erase(rid);
        }
        info.erase(rid);
    }
    // Called when the GPU arena is full: rebuild resident geometry lazily.
    void reset_gpu_geometry() {
        flat.clear();
        flat_bytes = 0;
        arena_exhausted = false;
        vita_gxm_mesh_reset();
    }
    void mesh_free(RID rid) override { invalidate_mesh(rid); custom_aabbs.erase(rid); RendererDummy::MeshStorage::mesh_free(rid); }
    // In-place region updates (Sprite3D) and custom AABBs.
    HashMap<RID, AABB> custom_aabbs;
    void patch_surface(RID rid, int surface, int offset, const Vector<uint8_t> &data, int which) {
        RendererDummy::DummyMesh *m = get_mesh(rid);
        ERR_FAIL_NULL(m);
        ERR_FAIL_INDEX(surface, m->surfaces.size());
        RenderingServerTypes::SurfaceData &s = m->surfaces.write[surface];
        Vector<uint8_t> &target = which == 0 ? s.vertex_data : (which == 1 ? s.attribute_data : (which == 2 ? s.skin_data : s.index_data));
        ERR_FAIL_COND(offset < 0 || offset + data.size() > target.size());
        memcpy(target.ptrw() + offset, data.ptr(), data.size());
        if (!refresh_in_place(rid, surface)) invalidate_mesh(rid);
        m->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
    }
    // Rewrites an already uploaded surface in its existing GPU arena slot.
    // Dropping and re-flattening on every sprite frame leaked arena space
    // until the renderer rebuilt all geometry at once (a long hitch).
    bool refresh_in_place(RID rid, int surface) {
        auto *surfaces = flat.getptr(rid);
        FlatSurface *value = surfaces ? surfaces->getptr(surface) : nullptr;
        if (!value || !value->gpu) return false;
        Array arrays = RenderingServer::get_singleton()->mesh_create_arrays_from_surface_data(mesh_get_surface(rid, surface));
        if (arrays.is_empty()) return false;
        PackedVector3Array positions = arrays[RSE::ARRAY_VERTEX];
        PackedVector2Array uvs = arrays[RSE::ARRAY_TEX_UV];
        PackedInt32Array indices = arrays[RSE::ARRAY_INDEX];
        int count = indices.is_empty() ? positions.size() : indices.size();
        count -= count % 3;
        if (count <= 0 || unsigned(count) != value->count) return false;
        Vector<float> xyz, uv;
        xyz.resize(count * 3);
        uv.resize(count * 2);
        float *xyz_w = xyz.ptrw();
        float *uv_w = uv.ptrw();
        for (int i = 0; i < count; i++) {
            int vertex = indices.is_empty() ? i : indices[i];
            if (vertex < 0 || vertex >= positions.size()) return false;
            xyz_w[i * 3] = positions[vertex].x;
            xyz_w[i * 3 + 1] = positions[vertex].y;
            xyz_w[i * 3 + 2] = positions[vertex].z;
            uv_w[i * 2] = vertex < uvs.size() ? uvs[vertex].x : 0.0f;
            uv_w[i * 2 + 1] = vertex < uvs.size() ? uvs[vertex].y : 0.0f;
        }
        if (!vita_gxm_mesh_update(value->gpu, xyz.ptr(), uv.ptr(), unsigned(count))) return false;
        value->positions.clear();
        value->normals.clear();
        set_tint(rid, surface, arrays[RSE::ARRAY_COLOR]);
        auto *mesh = decoded.getptr(rid);
        if (mesh) {
            for (const KeyValue<int, Array> &cached : *mesh) {
                PackedVector3Array cached_positions = cached.value.is_empty() ? PackedVector3Array() : PackedVector3Array(cached.value[RSE::ARRAY_VERTEX]);
                uint64_t bytes = uint64_t(cached_positions.size()) * 256 + uint64_t(PackedInt32Array(cached.value[RSE::ARRAY_INDEX]).size()) * 4;
                cache_bytes = cache_bytes > bytes ? cache_bytes - bytes : 0;
            }
            decoded.erase(rid);
        }
        return true;
    }
    // Vertex colors become one tint per surface. Sprite3D carries modulate
    // (hit flashes, tells, fades) only in its vertex colors.
    void set_tint(RID rid, int surface, const PackedColorArray &colors) {
        auto *entry = flat.getptr(rid);
        FlatSurface *value = entry ? entry->getptr(surface) : nullptr;
        if (!value) return;
        if (colors.is_empty()) { value->tint = Color(1, 1, 1, 1); return; }
        Color sum(0, 0, 0, 0);
        for (int i = 0; i < colors.size(); i++) sum += colors[i];
        value->tint = sum / float(colors.size());
    }
    void mesh_surface_update_vertex_region(RID rid, int surface, int offset, const Vector<uint8_t> &data) override { patch_surface(rid, surface, offset, data, 0); }
    void mesh_surface_update_attribute_region(RID rid, int surface, int offset, const Vector<uint8_t> &data) override { patch_surface(rid, surface, offset, data, 1); }
    void mesh_surface_update_skin_region(RID rid, int surface, int offset, const Vector<uint8_t> &data) override { patch_surface(rid, surface, offset, data, 2); }
    void mesh_surface_update_index_region(RID rid, int surface, int offset, const Vector<uint8_t> &data) override { patch_surface(rid, surface, offset, data, 3); }
    void mesh_set_custom_aabb(RID rid, const AABB &aabb) override {
        if (aabb.size == Vector3()) custom_aabbs.erase(rid); else custom_aabbs[rid] = aabb;
        RendererDummy::DummyMesh *m = get_mesh(rid);
        if (m) m->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
    }
    AABB mesh_get_custom_aabb(RID rid) const override {
        const AABB *aabb = custom_aabbs.getptr(rid);
        return aabb ? *aabb : AABB();
    }
    void mesh_clear(RID rid) override { invalidate_mesh(rid); RendererDummy::MeshStorage::mesh_clear(rid); }
    void mesh_surface_remove(RID rid, int surface) override { invalidate_mesh(rid); RendererDummy::MeshStorage::mesh_surface_remove(rid, surface); }
    void mesh_add_surface(RID rid, const RenderingServerTypes::SurfaceData &surface) override {
        invalidate_mesh(rid);
        RendererDummy::MeshStorage::mesh_add_surface(rid, surface);
    }
    Array get_decoded(RID rid, int surface) {
        auto *mesh = decoded.getptr(rid);
        if (mesh) {
            Array *cached = mesh->getptr(surface);
            if (cached) return *cached;
        }
        Array arrays = RenderingServer::get_singleton()->mesh_create_arrays_from_surface_data(mesh_get_surface(rid, surface));
        if (arrays.is_empty()) return arrays;
        // Account by vertex count conservatively; avoid dynamic Variant property lookups.
        PackedVector3Array positions = arrays.is_empty() ? PackedVector3Array() : PackedVector3Array(arrays[RSE::ARRAY_VERTEX]);
        uint64_t bytes = uint64_t(positions.size()) * 256 + uint64_t(PackedInt32Array(arrays[RSE::ARRAY_INDEX]).size()) * 4;
        if (cache_bytes + bytes <= 32 * 1024 * 1024) { decoded[rid][surface] = arrays; cache_bytes += bytes; }
        return arrays;
    }
    AABB mesh_get_aabb(RID rid, RID skeleton = RID()) override {
        const AABB *custom = custom_aabbs.getptr(rid); // Custom AABB wins
        if (custom) return *custom;
        AABB bounds;
        int count = mesh_get_surface_count(rid);
        for (int i = 0; i < count; i++) {
            AABB next = mesh_get_surface(rid, i).aabb;
            auto surface = mesh_get_surface(rid, i);
            if (owns_skeleton(skeleton)) {
                for (int bone = 0; bone < surface.bone_aabbs.size() && bone < skeleton_get_bone_count(skeleton); bone++) {
                    if (surface.bone_aabbs[bone].size != Vector3()) {
                        next = next.merge(skeleton_bone_get_transform(skeleton, bone).xform(surface.bone_aabbs[bone]));
                    }
                }
            }
            bounds = i == 0 ? next : bounds.merge(next);
        }
        return bounds;
    }
};

// Keeps the light parameters the renderer needs for its per-object lighting
// approximation. Shadows, projectors and light baking remain unsupported.
class GXMLightStorage : public RendererDummy::LightStorage {
public:
    struct Light {
        RSE::LightType type = RSE::LIGHT_OMNI;
        Color color = Color(1, 1, 1);
        float params[RSE::LIGHT_PARAM_MAX] = {};
        uint32_t cull_mask = 0xFFFFFFFF;
        bool negative = false;
        uint64_t version = 0;
    };
    struct LightInstance {
        RID light;
        Transform3D transform;
    };
private:
    mutable RID_Owner<Light, true> lights;
    mutable RID_Owner<LightInstance> light_instances;
    RID allocate(RSE::LightType type) {
        Light light;
        light.type = type;
        light.params[RSE::LIGHT_PARAM_ENERGY] = 1.0f;
        light.params[RSE::LIGHT_PARAM_INDIRECT_ENERGY] = 1.0f;
        light.params[RSE::LIGHT_PARAM_RANGE] = 5.0f;
        light.params[RSE::LIGHT_PARAM_ATTENUATION] = 1.0f;
        light.params[RSE::LIGHT_PARAM_SPOT_ANGLE] = 45.0f;
        light.params[RSE::LIGHT_PARAM_SPOT_ATTENUATION] = 1.0f;
        return lights.make_rid(light);
    }
public:
    bool owns_light(RID rid) const { return lights.owns(rid); }
    const Light *get_light(RID rid) const { return lights.get_or_null(rid); }
    const LightInstance *get_light_instance(RID rid) const { return light_instances.get_or_null(rid); }

    RID directional_light_allocate() override { return allocate(RSE::LIGHT_DIRECTIONAL); }
    void directional_light_initialize(RID) override {}
    RID omni_light_allocate() override { return allocate(RSE::LIGHT_OMNI); }
    void omni_light_initialize(RID) override {}
    RID spot_light_allocate() override { return allocate(RSE::LIGHT_SPOT); }
    void spot_light_initialize(RID) override {}
    void light_free(RID rid) override {
        if (lights.owns(rid)) lights.free(rid);
    }
    void light_set_color(RID rid, const Color &color) override {
        Light *light = lights.get_or_null(rid);
        if (light) { light->color = color; light->version++; }
    }
    void light_set_param(RID rid, RSE::LightParam param, float value) override {
        Light *light = lights.get_or_null(rid);
        ERR_FAIL_INDEX(param, RSE::LIGHT_PARAM_MAX);
        if (light) { light->params[param] = value; light->version++; }
    }
    void light_set_negative(RID rid, bool enable) override {
        Light *light = lights.get_or_null(rid);
        if (light) light->negative = enable;
    }
    void light_set_cull_mask(RID rid, uint32_t mask) override {
        Light *light = lights.get_or_null(rid);
        if (light) light->cull_mask = mask;
    }
    RSE::LightType light_get_type(RID rid) const override {
        const Light *light = lights.get_or_null(rid);
        return light ? light->type : RSE::LIGHT_OMNI;
    }
    AABB light_get_aabb(RID rid) const override {
        const Light *light = lights.get_or_null(rid);
        if (!light) return AABB();
        float range = light->params[RSE::LIGHT_PARAM_RANGE];
        if (light->type == RSE::LIGHT_OMNI) return AABB(Vector3(-range, -range, -range), Vector3(range, range, range) * 2);
        if (light->type == RSE::LIGHT_SPOT) {
            float size = Math::tan(Math::deg_to_rad(MIN(light->params[RSE::LIGHT_PARAM_SPOT_ANGLE], 89.0f))) * range;
            return AABB(Vector3(-size, -size, -range), Vector3(size * 2, size * 2, range));
        }
        return AABB();
    }
    float light_get_param(RID rid, RSE::LightParam param) override {
        const Light *light = lights.get_or_null(rid);
        return light && param >= 0 && param < RSE::LIGHT_PARAM_MAX ? light->params[param] : 0.0f;
    }
    Color light_get_color(RID rid) override {
        const Light *light = lights.get_or_null(rid);
        return light ? light->color : Color();
    }
    uint64_t light_get_version(RID rid) const override {
        const Light *light = lights.get_or_null(rid);
        return light ? light->version : 0;
    }
    uint32_t light_get_cull_mask(RID rid) const override {
        const Light *light = lights.get_or_null(rid);
        return light ? light->cull_mask : 0;
    }
    RID light_instance_create(RID light) override {
        LightInstance instance;
        instance.light = light;
        return light_instances.make_rid(instance);
    }
    void light_instance_free(RID rid) override {
        if (light_instances.owns(rid)) light_instances.free(rid);
    }
    void light_instance_set_transform(RID rid, const Transform3D &transform) override {
        LightInstance *instance = light_instances.get_or_null(rid);
        if (instance) instance->transform = transform;
    }
};

class GXMUtilities : public RendererDummy::Utilities {
    GXMMeshStorage *meshes;
    GXMLightStorage *lights;
    GXMTextureStorage *textures;
public:
    GXMUtilities(GXMMeshStorage *p_meshes, GXMLightStorage *p_lights, GXMTextureStorage *p_textures) : meshes(p_meshes), lights(p_lights), textures(p_textures) {}
    bool free(RID rid) override {
        if (meshes->owns_skeleton(rid)) { meshes->skeleton_free(rid); return true; }
        if (lights->owns_light(rid)) { lights->light_free(rid); return true; }
        if (textures->owns_decal(rid)) { textures->decal_free(rid); return true; }
        return RendererDummy::Utilities::free(rid);
    }
    // Without this the scene cull rejects light and decal instances
    // ("unimplemented base type") and render_scene never receives them.
    RSE::InstanceType get_base_type(RID rid) const override {
        if (lights->owns_light(rid)) return RSE::INSTANCE_LIGHT;
        if (textures->owns_decal(rid)) return RSE::INSTANCE_DECAL;
        return RendererDummy::Utilities::get_base_type(rid);
    }
};

class GXMMaterialStorage : public RendererDummy::MaterialStorage {
    HashMap<RID, HashMap<StringName, Variant>> parameters;
public:
    HashMap<RID, uint32_t> versions;
    void material_set_param(RID rid, const StringName &name, const Variant &value) override {
        parameters[rid][name] = value;
        versions[rid]++;
    }
    uint32_t material_version(RID rid) const {
        const uint32_t *version = versions.getptr(rid);
        return version ? *version : 0;
    }
    Variant material_get_param(RID rid, const StringName &name) const override {
        const auto *map = parameters.getptr(rid);
        if (!map) return Variant();
        const auto *value = map->getptr(name);
        return value ? *value : Variant();
    }
    void material_free(RID rid) override {
        parameters.erase(rid);
        versions.erase(rid);
        RendererDummy::MaterialStorage::material_free(rid);
    }
};

class GXMSceneBuffers : public RenderSceneBuffers {
public:
    RID target;
    void configure(const RenderSceneBuffersConfiguration *config) override { target = config->get_render_target(); }
    void set_fsr_sharpness(float) override {}
    void set_texture_mipmap_bias(float) override {}
    void set_anisotropic_filtering_level(RSE::ViewportAnisotropicFiltering) override {}
    void set_use_debanding(bool) override {}
};

class GXMScene : public RasterizerSceneDummy {
    class Instance : public GeometryInstanceDummy {
    public:
        RID mesh, material;
        Vector<RID> materials;
        Transform3D transform;
        AABB bounds, world_bounds;
        uint32_t layers = 1;
        RID skeleton;
        void set_transform(const Transform3D &t, const AABB &aabb, const AABB &transformed) override { transform = t; bounds = aabb; world_bounds = transformed; }
        Transform3D get_transform() override { return transform; }
        AABB get_aabb() override { return bounds; }
        void set_material_override(RID rid) override { material = rid; }
        void set_surface_materials(const Vector<RID> &rids) override { materials = rids; }
        void set_layer_mask(uint32_t value) override { layers = value; }
        void set_skeleton(RID rid) override { skeleton = rid; }
    };
    // Deferred transparent surfaces.
    struct TransparentDraw {
        void *mesh = nullptr;
        void *texture = nullptr;
        float matrix[16];
        uint32_t color = 0;
        unsigned count = 0;
        float depth = 0.0f;
        struct Sort {
            _FORCE_INLINE_ bool operator()(const TransparentDraw &a, const TransparentDraw &b) const { return a.depth > b.depth; }
        };
    };
    LocalVector<TransparentDraw> transparent_draws;
    struct DrawCandidate {
        Instance *instance = nullptr;
        float center_distance = 0.0f;
        float radius = 0.0f;
        float priority = 0.0f;
        bool structural = false;
        struct Sort {
            _FORCE_INLINE_ bool operator()(const DrawCandidate &a, const DrawCandidate &b) const {
                if (a.structural != b.structural) return a.structural;
                if (a.priority != b.priority) return a.priority > b.priority;
                return a.instance < b.instance;
            }
        };
    };
    struct ActiveLight {
        RSE::LightType type;
        Vector3 position, direction;
        Color color;
        float range, attenuation, spot_cos;
        uint32_t cull_mask;
    };
    GXMTextureStorage *textures;
    GXMMeshStorage *meshes;
    GXMMaterialStorage *materials;
    GXMLightStorage *lights;
    unsigned frames = 0;
    unsigned adaptive_triangle_budget = 60000;
    float submit_average_us = 0.0f;
    float gpu_wait_average_us = 0.0f;
    Vector<DrawCandidate> draw_candidates;
    Vector<ActiveLight> active_lights;
    // Camera-facing radial glows: the item/note sparkle shader and lamp
    // flares (the original's glow). Drawn after geometry, depth-tested.
    struct Glow {
        Vector3 position;
        float half_size;
        Color color;
    };
    Vector<Glow> glows;
    void *glow_texture = nullptr;
    void *get_glow_texture() {
        if (glow_texture) return glow_texture;
        const int size = 32;
        uint8_t pixels[size * size * 4];
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = (x + 0.5f) / size * 2.0f - 1.0f, dy = (y + 0.5f) / size * 2.0f - 1.0f;
                float falloff = CLAMP(1.0f - Math::sqrt(dx * dx + dy * dy), 0.0f, 1.0f);
                uint8_t *p = pixels + (y * size + x) * 4;
                // Premultiplied: additive blending ignores alpha.
                p[0] = p[1] = p[2] = uint8_t(falloff * falloff * 255.0f);
                p[3] = uint8_t(falloff * falloff * 255.0f);
            }
        }
        glow_texture = vita_gxm_texture_create(size, size, pixels);
        return glow_texture;
    }
    // Instances submitted in the previous frame. Only compared as keys, never
    // dereferenced, so a freed instance cannot be touched through this set.
    HashSet<Instance *> drawn_last, drawn_now;
    // Reused CPU skinning buffers.
    Vector<Vector3> skinned_positions;
    Vector<float> batch_xyz, batch_uv;

    // Skinned geometry flattened per instance surface. Re-skinned only when
    // the skeleton changed, and at most every other frame (30 Hz animation
    // at 60 FPS), which halves the CPU skinning cost of visible characters.
    struct SkinCache {
        uint64_t version = 0;
        unsigned frame = 0;
        Vector<float> xyz, uv;
    };
    HashMap<Instance *, HashMap<int, SkinCache>> skin_cache;

    // Per-vertex lighting: colors for the multiplicative pass, kept in the
    // GPU arena per instance surface and recomputed only when the lights
    // reaching the instance change (bounded per frame).
    struct LitSurface {
        void *gpu = nullptr;
        unsigned count = 0;
        uint64_t hash = 0;
        // Shadows: one bit per light slot and vertex, filled progressively by
        // rays from the light against static geometry.
        Vector<uint8_t> shadow;
        unsigned shadow_next = 0;
        bool shadow_complete = false;
    };
    // Per-object shadow for surfaces that use the tint (props, characters):
    // whether static geometry blocks each light from the object's center.
    struct ObjectShadow {
        HashMap<uint64_t, bool> occluded;
        unsigned refreshed = 0;
    };
    HashMap<Instance *, ObjectShadow> object_shadows;
    PhysicsDirectSpaceState3D *space_state = nullptr;
    int shadow_rays_left = 0;
    unsigned shadow_rays_cast = 0;

    static uint64_t light_key(const ActiveLight &light) {
        return hash_murmur3_one_64(uint64_t(int64_t(light.position.x * 20)) ^ (uint64_t(int64_t(light.position.y * 20)) << 21) ^ (uint64_t(int64_t(light.position.z * 20)) << 42));
    }
    // True when static geometry lies between the light and the point.
    bool light_blocked(const ActiveLight &light, const Vector3 &point, const AABB *own_bounds = nullptr) {
        if (!space_state || light.type == RSE::LIGHT_DIRECTIONAL) return false;
        Vector3 offset = point - light.position;
        float distance = offset.length();
        if (distance < 0.3f) return false;
        Vector3 direction = offset / distance;
        PhysicsServer3DTypes::RayParameters ray;
        // Start outside the lamp's own collider and stop short of the surface.
        ray.from = light.position + direction * 0.15f;
        ray.to = point - direction * 0.06f;
        ray.collide_with_areas = false;
        PhysicsServer3DTypes::RayResult hit;
        shadow_rays_left--;
        shadow_rays_cast++;
        if (!space_state->intersect_ray(ray, hit)) return false;
        if (hit.position.distance_to(point) < 0.35f) return false;
        if (own_bounds && own_bounds->grow(0.1f).has_point(hit.position)) return false;
        // Only level geometry casts these cached shadows; the player and
        // physics props move and would leave stale shadows behind.
        return Object::cast_to<StaticBody3D>(ObjectDB::get_instance(hit.collider_id)) != nullptr;
    }
    HashMap<Instance *, HashMap<int, LitSurface>> lit_cache;
    Vector<uint32_t> lit_colors;
    Vector<int> instance_lights;
    Color lit_base;
    float lit_gain = 1.0f;

    void collect_instance_lights(const Instance *instance) {
        instance_lights.clear();
        const AABB &box = instance->world_bounds;
        for (int i = 0; i < active_lights.size(); i++) {
            const ActiveLight &light = active_lights[i];
            if (!(light.cull_mask & instance->layers)) continue;
            if (light.type != RSE::LIGHT_DIRECTIONAL) {
                Vector3 nearest = light.position.clamp(box.position, box.position + box.size);
                if (nearest.distance_to(light.position) >= light.range) continue;
            }
            instance_lights.push_back(i);
        }
        // Sort by light position: the culled light list changes order between
        // frames, and shadow bits must keep referring to the same light.
        SortArray<int, LightOrder> sorter;
        sorter.compare.lights = &active_lights;
        sorter.sort(instance_lights.ptrw(), instance_lights.size());
    }
    struct LightOrder {
        const Vector<ActiveLight> *lights = nullptr;
        _FORCE_INLINE_ bool operator()(int a, int b) const {
            const Vector3 &pa = (*lights)[a].position, &pb = (*lights)[b].position;
            if (pa.x != pb.x) return pa.x < pb.x;
            if (pa.y != pb.y) return pa.y < pb.y;
            return pa.z < pb.z;
        }
    };
    uint64_t instance_light_hash(const Instance *instance) const {
        uint64_t hash = hash_murmur3_one_64(uint64_t(instance->mesh.get_id()));
        const Vector3 &o = instance->transform.origin;
        hash = hash_murmur3_one_64(uint64_t(int64_t(o.x * 100)) ^ (uint64_t(int64_t(o.y * 100)) << 21) ^ (uint64_t(int64_t(o.z * 100)) << 42), hash);
        hash = hash_murmur3_one_64(uint64_t(lit_base.r * 255) | uint64_t(lit_base.g * 255) << 8 | uint64_t(lit_base.b * 255) << 16 | uint64_t(lit_gain * 64) << 24, hash);
        for (int index : instance_lights) {
            const ActiveLight &light = active_lights[index];
            hash = hash_murmur3_one_64(uint64_t(int64_t(light.position.x * 20)) ^ (uint64_t(int64_t(light.position.y * 20)) << 21) ^ (uint64_t(int64_t(light.position.z * 20)) << 42), hash);
            hash = hash_murmur3_one_64(uint64_t(light.range * 8) | uint64_t(light.type) << 32, hash);
        }
        return hash;
    }
    LitSurface *get_lit_surface(Instance *instance, int surface, unsigned count, unsigned &vertex_budget, uint64_t &build_us) {
        LitSurface &lit = lit_cache[instance][surface];
        collect_instance_lights(instance);
        uint64_t hash = instance_light_hash(instance);
        bool colors_valid = lit.gpu && lit.count == count && lit.hash == hash;
        bool trace = colors_valid && !lit.shadow_complete && shadow_rays_left > 0;
        if (colors_valid && !trace) return &lit;
        // Over budget this frame: keep stale colors rather than stalling.
        if (vertex_budget < count) return lit.gpu && lit.count == count ? &lit : nullptr;
        uint64_t started = OS::get_singleton()->get_ticks_usec();
        const GXMMeshStorage::FlatSurface *geometry = meshes->get_flat_geometry(instance->mesh, surface);
        if (!geometry || geometry->count != count) return nullptr;
        const Transform3D &xform = instance->transform;
        if (!colors_valid) {
            // New light set: shadows start over (lit until traced).
            lit.shadow.resize(count);
            memset(lit.shadow.ptrw(), 0, count);
            lit.shadow_next = 0;
            lit.shadow_complete = !space_state;
        } else {
            // Trace the next vertices; recolor only once the surface is done.
            const Vector3 *trace_positions = geometry->positions.ptr();
            uint8_t *bits = lit.shadow.ptrw();
            while (lit.shadow_next < count && shadow_rays_left > 0) {
                unsigned i = lit.shadow_next++;
                Vector3 world = xform.xform(trace_positions[i]);
                for (int slot = 0; slot < instance_lights.size() && slot < 8; slot++) {
                    const ActiveLight &light = active_lights[instance_lights[slot]];
                    if (light.type == RSE::LIGHT_DIRECTIONAL || world.distance_to(light.position) >= light.range) continue;
                    if (light_blocked(light, world)) bits[i] |= uint8_t(1 << slot);
                }
            }
            build_us += OS::get_singleton()->get_ticks_usec() - started;
            if (lit.shadow_next < count) return &lit;
            lit.shadow_complete = true;
            started = OS::get_singleton()->get_ticks_usec();
        }
        vertex_budget -= count;
        if (!lit.gpu || lit.count != count) {
            Vector<float> xyz;
            xyz.resize(count * 3);
            float *w = xyz.ptrw();
            for (unsigned i = 0; i < count; i++) {
                w[i * 3] = geometry->positions[i].x;
                w[i * 3 + 1] = geometry->positions[i].y;
                w[i * 3 + 2] = geometry->positions[i].z;
            }
            lit.gpu = vita_gxm_light_mesh_create(xyz.ptr(), count);
            lit.count = count;
            if (!lit.gpu) {
                if (vita_gxm_mesh_arena_used() > 0) meshes->arena_exhausted = true;
                return nullptr;
            }
        }
        lit_colors.resize(count);
        uint32_t *out = lit_colors.ptrw();
        const Vector3 *positions = geometry->positions.ptr();
        const Vector3 *normals = geometry->normals.is_empty() ? nullptr : geometry->normals.ptr();
        const uint8_t *shadow_bits = lit.shadow.ptr();
        for (unsigned i = 0; i < count; i++) {
            Vector3 world = xform.xform(positions[i]);
            Vector3 normal = normals ? xform.basis.xform(normals[i]).normalized() : Vector3();
            Color sum = lit_base;
            for (int slot = 0; slot < instance_lights.size(); slot++) {
                if (slot < 8 && (shadow_bits[i] & (1 << slot))) continue;
                const ActiveLight &light = active_lights[instance_lights[slot]];
                if (light.type == RSE::LIGHT_DIRECTIONAL) {
                    float facing = normals ? MAX(normal.dot(-light.direction), 0.0f) * 0.85f + 0.15f : 0.6f;
                    sum += light.color * facing;
                    continue;
                }
                Vector3 offset = light.position - world;
                float distance = offset.length();
                if (distance >= light.range) continue;
                float t = 1.0f - distance / light.range;
                // Attenuation 1 (the default) is linear; avoid pow() per vertex.
                float falloff = Math::abs(light.attenuation - 1.0f) < 0.01f ? t : Math::pow(t, MAX(light.attenuation, 0.25f));
                Vector3 direction = distance > 0.001f ? offset / distance : Vector3(0, 1, 0);
                if (light.type == RSE::LIGHT_SPOT) {
                    float cosine = (-direction).dot(light.direction);
                    if (cosine < light.spot_cos) continue;
                    falloff *= CLAMP((cosine - light.spot_cos) / MAX(1.0f - light.spot_cos, 0.001f) * 4.0f, 0.0f, 1.0f);
                }
                float facing = normals ? MAX(normal.dot(direction), 0.0f) * 0.85f + 0.15f : 1.0f;
                sum += light.color * (falloff * facing);
            }
            // Stored as light / 2 for the 2x multiply blend.
            float r = CLAMP(sum.r * lit_gain * 0.5f, 0.0f, 1.0f);
            float g = CLAMP(sum.g * lit_gain * 0.5f, 0.0f, 1.0f);
            float b = CLAMP(sum.b * lit_gain * 0.5f, 0.0f, 1.0f);
            out[i] = uint32_t(r * 255.0f) | (uint32_t(g * 255.0f) << 8) | (uint32_t(b * 255.0f) << 16) | 0xff000000u;
        }
        vita_gxm_light_mesh_set_colors(lit.gpu, out);
        lit.hash = hash;
        build_us += OS::get_singleton()->get_ticks_usec() - started;
        return &lit;
    }
    float camera_fade(const Instance *instance, const Vector3 &camera_position, bool distance_darkening) const {
        if (!distance_darkening) return 1.0f;
        const AABB &box = instance->world_bounds;
        float distance = camera_position.distance_to(camera_position.clamp(box.position, box.position + box.size));
        return CLAMP(1.0f - (distance - 2.0f) / 14.0f, 0.18f, 1.0f);
    }

    // Material properties resolved once per material version: per-surface
    // Variant/StringName lookups were a large share of the submission time.
    struct MaterialInfo {
        uint32_t version = ~0u;
        Color albedo = Color(1, 1, 1, 1);
        Variant texture;
        bool sparkle = false;
        Color sparkle_color;
        float sparkle_size = 0.2f;
        float pulse_speed = 3.0f;
        bool emissive = false;
        Color glow;
    };
    HashMap<RID, MaterialInfo> material_cache;
    const MaterialInfo &material_info(RID material) {
        uint32_t version = materials->material_version(material);
        MaterialInfo *info = material_cache.getptr(material);
        if (info && info->version == version) return *info;
        static const char *const albedo_names[] = { "albedo", "albedo_color", "color", nullptr };
        static const char *const texture_names[] = { "texture_albedo", "albedo_texture", "texture", "main_texture", nullptr };
        static const StringName sparkle_name("sparkle_color"), size_name("size"), speed_name("pulse_speed"),
                emission_name("emission"), energy_name("emission_energy_multiplier");
        MaterialInfo value;
        value.version = version;
        Variant albedo = material_param(material, albedo_names);
        if (albedo.get_type() == Variant::COLOR) value.albedo = albedo;
        value.texture = material_param(material, texture_names);
        Variant sparkle = materials->material_get_param(material, sparkle_name);
        if (sparkle.get_type() == Variant::COLOR) {
            value.sparkle = true;
            value.sparkle_color = sparkle;
            Variant size = materials->material_get_param(material, size_name);
            Variant speed = materials->material_get_param(material, speed_name);
            if (size.get_type() == Variant::FLOAT) value.sparkle_size = size;
            if (speed.get_type() == Variant::FLOAT) value.pulse_speed = speed;
        }
        Variant emission = materials->material_get_param(material, emission_name);
        if (emission.get_type() == Variant::COLOR) {
            float energy = 1.0f;
            Variant emission_energy = materials->material_get_param(material, energy_name);
            if (emission_energy.get_type() == Variant::FLOAT || emission_energy.get_type() == Variant::INT) energy = float(emission_energy);
            value.emissive = true;
            value.glow = Color(emission) * MIN(energy, 4.0f) * 0.35f;
        }
        if (material_cache.size() > 4096) material_cache.clear();
        return material_cache.insert(material, value)->value;
    }

    // StandardMaterial names first, then the uniform names used by the
    // game's own spatial shaders (curtains, clipping, opacity...).
    Variant material_param(RID material, const char *const *names) const {
        for (int i = 0; names[i]; i++) {
            Variant value = materials->material_get_param(material, StringName(names[i]));
            if (value.get_type() != Variant::NIL) return value;
        }
        return Variant();
    }

    Color instance_light(const Instance *instance, const Color &ambient, float floor, const Vector3 &camera_position, float gain, bool distance_darkening) {
        const AABB &box = instance->world_bounds;
        ObjectShadow *shadow = nullptr;
        bool refresh = false;
        if (space_state) {
            shadow = &object_shadows[const_cast<Instance *>(instance)];
            refresh = frames - shadow->refreshed > 20 && shadow_rays_left > 0;
            if (refresh) shadow->refreshed = frames;
        }
        Color total = ambient + Color(floor, floor, floor);
        if (distance_darkening) {
            // The original relies on SDFGI and volumetric fog, which leave far
            // rooms dark. Fade the unlit base with distance from the camera.
            float distance = camera_position.distance_to(camera_position.clamp(box.position, box.position + box.size));
            float fade = CLAMP(1.0f - (distance - 2.0f) / 14.0f, 0.18f, 1.0f);
            total = total * fade;
        }
        for (const ActiveLight &light : active_lights) {
            if (!(light.cull_mask & instance->layers)) continue;
            if (light.type == RSE::LIGHT_DIRECTIONAL) {
                total += light.color * 0.6f;
                continue;
            }
            // Nearest point of the object's bounds: a long wall is lit where
            // a lamp is close to it, not only when its center is.
            Vector3 nearest = light.position.clamp(box.position, box.position + box.size);
            Vector3 offset = nearest - light.position;
            float distance = offset.length();
            if (distance >= light.range) continue;
            if (shadow) {
                uint64_t key = light_key(light);
                bool *blocked = shadow->occluded.getptr(key);
                if ((!blocked || refresh) && shadow_rays_left > 0) {
                    bool value = light_blocked(light, box.get_center(), &box);
                    if (blocked) *blocked = value;
                    else blocked = &shadow->occluded.insert(key, value)->value;
                }
                if (blocked && *blocked) continue;
            }
            float center_distance = MIN(box.get_center().distance_to(light.position), light.range);
            float falloff = Math::pow(1.0f - (distance + center_distance) * 0.5f / light.range, MAX(light.attenuation, 0.25f));
            if (light.type == RSE::LIGHT_SPOT && distance > 0.001f) {
                float cosine = (offset / distance).dot(light.direction);
                if (cosine < light.spot_cos) continue;
                falloff *= CLAMP((cosine - light.spot_cos) / MAX(1.0f - light.spot_cos, 0.001f) * 4.0f, 0.0f, 1.0f);
            }
            total += light.color * falloff;
        }
        total = total * gain;
        return Color(MIN(total.r, 1.0f), MIN(total.g, 1.0f), MIN(total.b, 1.0f), 1.0f);
    }
public:
    GXMScene(GXMTextureStorage *t, GXMMeshStorage *m, GXMMaterialStorage *s, GXMLightStorage *l) : textures(t), meshes(m), materials(s), lights(l) {}
    RenderGeometryInstance *geometry_instance_create(RID rid) override {
        auto *instance = memnew(Instance);
        instance->mesh = rid;
        return instance;
    }
    void geometry_instance_free(RenderGeometryInstance *instance) override {
        skin_cache.erase(static_cast<Instance *>(instance));
        lit_cache.erase(static_cast<Instance *>(instance));
        object_shadows.erase(static_cast<Instance *>(instance));
        drawn_last.erase(static_cast<Instance *>(instance));
        memdelete(static_cast<Instance *>(instance));
    }
    Ref<RenderSceneBuffers> render_buffers_create() override {
        return Ref<RenderSceneBuffers>(memnew(GXMSceneBuffers));
    }
    void render_scene(const Ref<RenderSceneBuffers> &buffers, const CameraData *camera, const CameraData *,
            const PagedArray<RenderGeometryInstance *> &instances, const PagedArray<RID> &light_list, const PagedArray<RID> &,
            const PagedArray<RID> &, const PagedArray<RID> &decal_list, const PagedArray<RID> &, const PagedArray<RID> &,
            RID environment, RID, RID, RID, RID, RID, RID, int, float, const RenderShadowData *, int,
            const RenderSDFGIData *, int, float, const RenderSDFGIUpdateData *, RenderingServerTypes::RenderInfo *) override {
        if (buffers.is_null() || !camera) return;
        auto *target = static_cast<GXMSceneBuffers *>(buffers.ptr());
        if (!textures->is_screen(target->target)) {
            WARN_PRINT_ONCE("GXM3D: offscreen 3D viewports are not implemented.");
            return;
        }
        // Tilt-shift renders the 3D pass offscreen first.
        uint32_t clear_color = gxm_color(textures->render_target_get_clear_request_color(target->target));
        if (!bool(ProjectSettings::get_singleton()->get_setting("rendering/vita/tilt_shift", false)) || !vita_gxm_begin_offscreen(clear_color)) {
            vita_gxm_begin(clear_color);
        }
        textures->render_target_disable_clear_request(target->target);
        uint64_t started = OS::get_singleton()->get_ticks_usec();
        unsigned triangles = 0, attempted = 0, distance_culled = 0;
        Size2i viewport = textures->render_target_get_size(target->target);
        ProjectSettings *settings = ProjectSettings::get_singleton();
        float fit_x = 1.0f, fit_y = 1.0f;
        if (viewport.x > 0 && viewport.y > 0 && bool(settings->get_setting("rendering/vita/keep_aspect", false))) {
            float fit = MIN(960.0f / viewport.x, 544.0f / viewport.y);
            fit_x = viewport.x * fit / 960.0f;
            fit_y = viewport.y * fit / 544.0f;
        }
        vita_gxm_clip(1, int(480 * (1 - fit_x)), int(272 * (1 - fit_y)), int(960 * fit_x), int(544 * fit_y));
        // Static geometry is GPU-resident, so the ceiling is set by GPU time,
        // not by CPU vertex copies. Older game_data asking for more is clamped.
        unsigned configured_budget = CLAMP(int(settings->get_setting("rendering/vita/triangle_budget", 60000)), 2000, 80000);
        unsigned budget = MIN(configured_budget, adaptive_triangle_budget);
        uint64_t build_us = 0;

        // Lights and ambient for the per-object lighting approximation.
        bool lighting = bool(settings->get_setting("rendering/vita/per_object_lighting", true));
        float light_floor = float(settings->get_setting("rendering/vita/light_floor", 0.12f));
        float brightness = CLAMP(float(settings->get_setting("rendering/vita/brightness", 1.0f)), 0.4f, 2.0f);
        bool distance_darkening = bool(settings->get_setting("rendering/vita/distance_darkening", true));
        bool vertex_lighting = lighting && bool(settings->get_setting("rendering/vita/vertex_lighting", true)) && vita_gxm_vertex_lighting_available();
        unsigned vertex_budget = 3000, lit_draws = 0;
        shadow_rays_left = lighting ? int(settings->get_setting("rendering/vita/shadow_rays", 96)) : 0;
        shadow_rays_cast = 0;
        space_state = nullptr;
        if (shadow_rays_left > 0) {
            SceneTree *tree = SceneTree::get_singleton();
            Window *root = tree ? tree->get_root() : nullptr;
            Ref<World3D> world = root ? root->get_world_3d() : Ref<World3D>();
            if (world.is_valid()) space_state = PhysicsServer3D::get_singleton()->space_get_direct_state(world->get_space());
        }
        Color ambient;
        if (lighting && is_environment(environment) && environment_get_ambient_source(environment) != RSE::ENV_AMBIENT_SOURCE_DISABLED) {
            ambient = environment_get_ambient_light(environment) * environment_get_ambient_light_energy(environment);
        }
        active_lights.clear();
        if (lighting) {
            for (uint32_t i = 0; i < light_list.size(); i++) {
                const GXMLightStorage::LightInstance *instance = lights->get_light_instance(light_list[i]);
                const GXMLightStorage::Light *light = instance ? lights->get_light(instance->light) : nullptr;
                if (!light) continue;
                float energy = light->params[RSE::LIGHT_PARAM_ENERGY];
                if (energy <= 0.0f || light->negative) continue;
                ActiveLight active;
                active.type = light->type;
                active.position = instance->transform.origin;
                active.direction = -instance->transform.basis.get_column(2).normalized();
                active.color = light->color * MIN(energy, 4.0f);
                active.range = MAX(light->params[RSE::LIGHT_PARAM_RANGE], 0.01f);
                active.attenuation = light->params[RSE::LIGHT_PARAM_ATTENUATION];
                active.spot_cos = Math::cos(Math::deg_to_rad(CLAMP(light->params[RSE::LIGHT_PARAM_SPOT_ANGLE], 1.0f, 89.0f)));
                active.cull_mask = light->cull_mask;
                active_lights.push_back(active);
            }
        }

        lit_base = ambient + Color(light_floor, light_floor, light_floor);
        lit_gain = brightness;
        // Godot's visible-instance order is not stable while the camera turns.
        // Sort every visible set so room shells and large nearby surfaces are
        // always submitted before small props consume the adaptive budget.
        draw_candidates.clear();
        draw_candidates.reserve(instances.size());
        for (uint32_t i = 0; i < instances.size(); i++) {
            auto *instance = static_cast<Instance *>(instances[i]);
            if (!meshes->owns_mesh(instance->mesh) || !(instance->layers & camera->visible_layers)) continue;
            // Godot's visibility pass performs frustum culling, but this
            // backend has no mesh LOD. Cull distant detail before decoding or
            // submitting it: rooms keep their large walls while small props
            // beyond useful Vita screen detail do not consume CPU/GXM budget.
            Vector3 center = instance->transform.xform(instance->bounds.get_center());
            Vector3 scale(instance->transform.basis.get_column(0).length(), instance->transform.basis.get_column(1).length(), instance->transform.basis.get_column(2).length());
            float radius = instance->bounds.size.length() * 0.5f * MAX(scale.x, MAX(scale.y, scale.z));
            float center_distance = camera->main_transform.origin.distance_to(center);
            bool was_drawn = drawn_last.has(instance);
            if (GXMCameraLOD::should_cull(center_distance, radius, was_drawn)) {
                distance_culled++;
                continue;
            }
            float edge_distance = center_distance - radius;
            DrawCandidate candidate;
            candidate.instance = instance;
            candidate.center_distance = center_distance;
            candidate.radius = radius;
            // Favor what was on screen last frame so props near the budget
            // limit do not swap places every frame while the camera moves.
            candidate.priority = radius / MAX(center_distance, 0.25f) * (was_drawn ? 1.25f : 1.0f);
            candidate.structural = radius >= 2.0f || edge_distance <= 1.0f;
            draw_candidates.push_back(candidate);
        }
        draw_candidates.sort_custom<DrawCandidate::Sort>();
        // Depth along the camera's horizontal heading orders the transparent pass.
        const Vector3 eye = camera->main_transform.origin;
        Vector3 view_forward = -camera->main_transform.basis.get_column(2);
        view_forward.y = 0.0f;
        view_forward = view_forward.length_squared() > 0.0001f ? view_forward.normalized() : -camera->main_transform.basis.get_column(2);
        transparent_draws.clear();
        drawn_now.clear();
        unsigned structural_drawn = 0, skinned_drawn = 0;
        uint64_t skinned_us = 0;
        for (uint32_t i = 0; i < draw_candidates.size(); i++) {
            auto *instance = draw_candidates[i].instance;
            const GXMMeshStorage::MeshInfo &info = meshes->get_info(instance->mesh);
            // Whole instances only: cutting a mesh at the budget leaves holes
            // that move with the camera. Room structure is bound only by the
            // configured ceiling, so the adaptive budget never removes walls.
            bool structural = draw_candidates[i].structural;
            // Skip only the instance that does not fit; smaller props behind
            // it in priority order can still use the remaining budget.
            if (attempted + info.triangle_count > (structural ? configured_budget : budget)) continue;
            drawn_now.insert(instance);
            if (structural) structural_drawn++;
            Projection mvp = camera->main_projection * Projection(camera->main_transform.affine_inverse() * instance->transform);
            for (int column = 0; column < 4; column++) {
                mvp[column][0] *= fit_x;
                mvp[column][1] *= fit_y;
            }
            float matrix[16];
            for (int column = 0; column < 4; column++)
                for (int row = 0; row < 4; row++) matrix[column * 4 + row] = mvp[column][row];
            float instance_fade = camera_fade(instance, camera->main_transform.origin, distance_darkening);
            Color light_color = lighting ? instance_light(instance, ambient, light_floor, camera->main_transform.origin, brightness, distance_darkening) :
                    Color(MIN(brightness, 1.0f), MIN(brightness, 1.0f), MIN(brightness, 1.0f), 1.0f);
            const Vector<Transform3D> *bones = meshes->owns_skeleton(instance->skeleton) ? meshes->skeleton_bones(instance->skeleton) : nullptr;
            uint64_t skin_started = bones ? OS::get_singleton()->get_ticks_usec() : 0;
            for (int s = 0; s < info.surfaces.size(); s++) {
                const GXMMeshStorage::SurfaceInfo &surface = info.surfaces[s];
                if (!surface.triangles) continue;
                RID material = instance->material.is_valid() ? instance->material :
                        (s < instance->materials.size() && instance->materials[s].is_valid() ? instance->materials[s] : surface.material);
                const MaterialInfo &material_props = material_info(material);
                if (material_props.sparkle) {
                    // item_spark.gdshader: pulsing round billboard instead of the
                    // white quad its mesh would otherwise draw.
                    float pulse = Math::abs(Math::sin(float(OS::get_singleton()->get_ticks_msec()) * 0.001f * material_props.pulse_speed));
                    Glow glow;
                    glow.position = instance->transform.origin;
                    glow.half_size = 0.5f * material_props.sparkle_size;
                    glow.color = material_props.sparkle_color;
                    glow.color.a *= pulse;
                    glows.push_back(glow);
                    continue;
                }
                Color color = material_props.albedo;
                const Variant &tex = material_props.texture;
                // First use of a texture uploads it to CDRAM inside get_gpu();
                // count that like a geometry build, not as steady submission.
                uint64_t texture_started = OS::get_singleton()->get_ticks_usec();
                void *gpu = tex.get_type() == Variant::RID ? textures->get_gpu(tex) : nullptr;
                uint64_t texture_us = OS::get_singleton()->get_ticks_usec() - texture_started;
                if (texture_us > 500) build_us += texture_us;
                // Opaque static surfaces near the camera get per-vertex light
                // from a second, multiplicative pass; the first pass then only
                // carries the distance fade. Far, skinned or transparent
                // surfaces keep the cheaper per-object tint.
                bool vertex_lit = vertex_lighting && !bones && color.a > 0.99f &&
                        !(tex.get_type() == Variant::RID && textures->texture_has_alpha(tex));
                const GXMMeshStorage::FlatSurface *flat = nullptr;
                if (!bones) {
                    flat = meshes->get_flat(instance->mesh, s, PackedVector3Array(), PackedVector2Array(), PackedInt32Array());
                    if (!flat) {
                        // First use: decode with Godot's own format implementation
                        // (including compressed imported vertices) and flatten.
                        uint64_t build_started = OS::get_singleton()->get_ticks_usec();
                        Array arrays = meshes->get_decoded(instance->mesh, s);
                        if (arrays.is_empty()) continue;
                        flat = meshes->get_flat(instance->mesh, s, arrays[RSE::ARRAY_VERTEX], arrays[RSE::ARRAY_TEX_UV], arrays[RSE::ARRAY_INDEX]);
                        meshes->set_tint(instance->mesh, s, arrays[RSE::ARRAY_COLOR]); // Tint on first use
                        build_us += OS::get_singleton()->get_ticks_usec() - build_started;
                    }
                }
                LitSurface *lit = vertex_lit && flat && flat->gpu ? get_lit_surface(instance, s, flat->count, vertex_budget, build_us) : nullptr;
                // Until the vertex colors exist, draw once with the tint.
                bool lit_ready = lit && lit->gpu;
                float tint_r = lit_ready ? instance_fade : light_color.r;
                float tint_g = lit_ready ? instance_fade : light_color.g;
                float tint_b = lit_ready ? instance_fade : light_color.b;
                color.r *= tint_r;
                color.g *= tint_g;
                color.b *= tint_b;
                // Approximate StandardMaterial emission without a custom GPU
                // shader. This preserves illuminated signs, keys and weapon
                // flashes at the cost of one material lookup per surface.
                if (material_props.emissive) {
                    const Color &glow = material_props.glow;
                    color.r = MIN(1.0f, color.r + glow.r);
                    color.g = MIN(1.0f, color.g + glow.g);
                    color.b = MIN(1.0f, color.b + glow.b);
                }
                // Vertex-color tint; channels above 1 brighten toward white.
                if (flat) {
                    const Color &t = flat->tint;
                    color.r = MIN(1.0f, color.r * MIN(t.r, 1.0f) + MAX(t.r - 1.0f, 0.0f));
                    color.g = MIN(1.0f, color.g * MIN(t.g, 1.0f) + MAX(t.g - 1.0f, 0.0f));
                    color.b = MIN(1.0f, color.b * MIN(t.b, 1.0f) + MAX(t.b - 1.0f, 0.0f));
                    color.a *= t.a;
                }
                // Transparent surfaces wait for the sorted pass.
                bool transparent = color.a < 0.99f || (tex.get_type() == Variant::RID && textures->texture_has_alpha(tex));
                if (transparent && !bones && flat && flat->gpu) {
                    TransparentDraw deferred;
                    deferred.mesh = flat->gpu;
                    deferred.texture = gpu;
                    memcpy(deferred.matrix, matrix, sizeof(matrix));
                    deferred.color = gxm_color(color);
                    deferred.count = flat->count;
                    deferred.depth = (instance->world_bounds.get_center() - eye).dot(view_forward);
                    transparent_draws.push_back(deferred);
                    attempted += flat->count / 3;
                    continue;
                }
                if (!bones) {
                    if (flat && flat->gpu) {
                        if (vita_gxm_mesh_draw(flat->gpu, gpu, matrix, gxm_color(color))) triangles += flat->count / 3;
                        attempted += flat->count / 3;
                        if (lit_ready && vita_gxm_light_mesh_draw(lit->gpu, matrix)) lit_draws++;
                        continue;
                    }
                    if (flat) {
                        unsigned available = unsigned(flat->xyz.size() / 9) * 3;
                        for (unsigned offset = 0; offset < available; offset += 3072) {
                            unsigned batch = MIN(3072u, available - offset);
                            if (vita_gxm_triangles3d(gpu, flat->xyz.ptr() + offset * 3, flat->uv.ptr() + offset * 2, batch, matrix, gxm_color(color))) triangles += batch / 3;
                        }
                        attempted += available / 3;
                        continue;
                    }
                }
                SkinCache *cache = bones ? &skin_cache[instance][s] : nullptr;
                uint64_t version = bones ? meshes->skeleton_version(instance->skeleton) : 0;
                if (!cache || cache->xyz.is_empty() || (cache->version != version && frames - cache->frame >= 2)) {
                    Array arrays = meshes->get_decoded(instance->mesh, s);
                    if (arrays.is_empty()) continue;
                    PackedVector3Array positions = arrays[RSE::ARRAY_VERTEX];
                    PackedVector2Array uvs = arrays[RSE::ARRAY_TEX_UV];
                    PackedInt32Array indices = arrays[RSE::ARRAY_INDEX];
                    const Vector3 *vertex_data = positions.ptr();
                    int vertex_count = positions.size();
                    if (bones && vertex_count) {
                        // CPU skinning into a reused buffer: no copy-on-write of the
                        // cached decode and no RID lookup per bone influence.
                        PackedInt32Array bone_ids = arrays[RSE::ARRAY_BONES];
                        PackedFloat32Array weights = arrays[RSE::ARRAY_WEIGHTS];
                        int influences = bone_ids.size() == weights.size() ? bone_ids.size() / vertex_count : 0;
                        if (influences == 4 || influences == 8) {
                            skinned_positions.resize(vertex_count);
                            Vector3 *posed = skinned_positions.ptrw();
                            const int32_t *ids = bone_ids.ptr();
                            const float *w = weights.ptr();
                            const Transform3D *pose = bones->ptr();
                            int bone_count = bones->size();
                            for (int vertex = 0; vertex < vertex_count; vertex++) {
                                Vector3 sum;
                                float total = 0;
                                const Vector3 &rest = vertex_data[vertex];
                                for (int k = 0; k < influences; k++) {
                                    int offset = vertex * influences + k;
                                    float weight = w[offset];
                                    int bone = ids[offset];
                                    if (weight <= 0 || bone < 0 || bone >= bone_count) continue;
                                    sum += pose[bone].xform(rest) * weight;
                                    total += weight;
                                }
                                posed[vertex] = total > 0 ? sum / total : rest;
                            }
                            vertex_data = posed;
                        }
                    }
                    int count = indices.is_empty() ? vertex_count : indices.size();
                    count -= count % 3;
                    const int32_t *index_data = indices.ptr();
                    const Vector2 *uv_data = uvs.ptr();
                    int uv_count = uvs.size();
                    // Flatten into the instance cache (skinned) or a transient
                    // buffer (static geometry the arena/CPU cache could not hold).
                    Vector<float> &out_xyz = cache ? cache->xyz : batch_xyz;
                    Vector<float> &out_uv = cache ? cache->uv : batch_uv;
                    out_xyz.resize(count * 3);
                    out_uv.resize(count * 2);
                    float *bxyz = out_xyz.ptrw();
                    float *buv = out_uv.ptrw();
                    int written = 0;
                    for (int i = 0; i < count; i++) {
                        int index = indices.is_empty() ? i : index_data[i];
                        if (index < 0 || index >= vertex_count) index = 0;
                        bxyz[written * 3] = vertex_data[index].x;
                        bxyz[written * 3 + 1] = vertex_data[index].y;
                        bxyz[written * 3 + 2] = vertex_data[index].z;
                        buv[written * 2] = index < uv_count ? uv_data[index].x : 0;
                        buv[written * 2 + 1] = index < uv_count ? uv_data[index].y : 0;
                        written++;
                    }
                    if (cache) {
                        cache->version = version;
                        cache->frame = frames;
                    }
                }
                const Vector<float> &draw_xyz = cache ? cache->xyz : batch_xyz;
                const Vector<float> &draw_uv = cache ? cache->uv : batch_uv;
                unsigned available = unsigned(draw_xyz.size() / 9) * 3;
                for (unsigned offset = 0; offset < available; offset += 3072) {
                    unsigned batch = MIN(3072u, available - offset);
                    if (vita_gxm_triangles3d(gpu, draw_xyz.ptr() + offset * 3, draw_uv.ptr() + offset * 2, batch, matrix, gxm_color(color))) triangles += batch / 3;
                }
                attempted += available / 3;
            }
            if (bones) {
                skinned_drawn++;
                skinned_us += OS::get_singleton()->get_ticks_usec() - skin_started;
            }
        }
        // Transparent pass, far to near, depth-tested but not depth-written.
        unsigned transparent_drawn = 0;
        if (transparent_draws.size()) {
            transparent_draws.sort_custom<TransparentDraw::Sort>();
            vita_gxm_depth_write(0);
            for (uint32_t i = 0; i < transparent_draws.size(); i++) {
                const TransparentDraw &d = transparent_draws[i];
                if (vita_gxm_mesh_draw(d.mesh, d.texture, d.matrix, d.color)) {
                    triangles += d.count / 3;
                    transparent_drawn++;
                }
            }
            vita_gxm_depth_write(1);
        }
        // Decals as textured quads on their projection plane (the box's local
        // XZ plane through its center), blended over the geometry already
        // drawn: depth-tested, not depth-written, lifted slightly to avoid
        // z-fighting with the floor or wall they sit on.
        unsigned decals_drawn = 0;
        if (decal_list.size()) {
            vita_gxm_depth_write(0);
            for (uint32_t i = 0; i < decal_list.size(); i++) {
                const GXMTextureStorage::DecalInstance *decal_instance = textures->get_decal_instance(decal_list[i]);
                const GXMTextureStorage::Decal *decal = decal_instance ? textures->get_decal(decal_instance->decal) : nullptr;
                if (!decal || decal->albedo.is_null() || !(decal->cull_mask & camera->visible_layers)) continue;
                void *gpu = textures->get_gpu(decal->albedo);
                if (!gpu) continue;
                float hx = decal->size.x * 0.5f, hz = decal->size.z * 0.5f, lift = MIN(0.01f, decal->size.y * 0.25f);
                const float xyz[18] = {
                    -hx, lift, -hz,  hx, lift, -hz,  hx, lift, hz,
                    -hx, lift, -hz,  hx, lift, hz,  -hx, lift, hz,
                };
                // Godot maps the decal texture with U along +X and V along +Z.
                const float uv[12] = { 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1 };
                Projection mvp = camera->main_projection * Projection(camera->main_transform.affine_inverse() * decal_instance->transform);
                for (int column = 0; column < 4; column++) {
                    mvp[column][0] *= fit_x;
                    mvp[column][1] *= fit_y;
                }
                float matrix[16];
                for (int column = 0; column < 4; column++)
                    for (int row = 0; row < 4; row++) matrix[column * 4 + row] = mvp[column][row];
                Color color = decal->modulate;
                if (lighting) {
                    Instance probe;
                    probe.world_bounds = AABB(decal_instance->transform.origin, Vector3());
                    probe.layers = decal->cull_mask;
                    // The probe lives on the stack: keep it out of the shadow cache.
                    PhysicsDirectSpaceState3D *saved_space = space_state;
                    space_state = nullptr;
                    Color light = instance_light(&probe, ambient, light_floor, camera->main_transform.origin, brightness, distance_darkening);
                    space_state = saved_space;
                    color.r *= light.r;
                    color.g *= light.g;
                    color.b *= light.b;
                }
                color.a *= CLAMP(decal->albedo_mix, 0.0f, 1.0f);
                if (color.a <= 0.01f) continue;
                if (vita_gxm_triangles3d(gpu, xyz, uv, 6, matrix, gxm_color(color))) decals_drawn++;
            }
            vita_gxm_depth_write(1);
        }
        // Lamp flares: a soft halo pulled slightly toward the camera so the
        // lamp's own mesh does not hide it.
        if (lighting && bool(settings->get_setting("rendering/vita/light_flares", true))) {
            for (const ActiveLight &light : active_lights) {
                if (light.type == RSE::LIGHT_DIRECTIONAL) continue;
                float peak = MAX(light.color.r, MAX(light.color.g, light.color.b));
                if (peak <= 0.01f) continue;
                Vector3 to_camera = camera->main_transform.origin - light.position;
                float distance = to_camera.length();
                if (distance < 0.3f || distance > 30.0f) continue;
                Glow glow;
                glow.position = light.position + to_camera / distance * 0.3f;
                glow.half_size = CLAMP(light.range * 0.12f, 0.25f, 0.9f);
                glow.color = Color(light.color.r / MAX(peak, 1.0f), light.color.g / MAX(peak, 1.0f), light.color.b / MAX(peak, 1.0f), CLAMP(peak * 0.45f, 0.2f, 0.7f));
                glows.push_back(glow);
            }
        }
        unsigned glows_drawn = 0;
        if (!glows.is_empty() && get_glow_texture()) {
            vita_gxm_depth_write(0);
            vita_gxm_blend_add(1);
            Vector3 right = camera->main_transform.basis.get_column(0).normalized();
            Vector3 up = camera->main_transform.basis.get_column(1).normalized();
            Projection mvp = camera->main_projection * Projection(camera->main_transform.affine_inverse());
            for (int column = 0; column < 4; column++) {
                mvp[column][0] *= fit_x;
                mvp[column][1] *= fit_y;
            }
            float matrix[16];
            for (int column = 0; column < 4; column++)
                for (int row = 0; row < 4; row++) matrix[column * 4 + row] = mvp[column][row];
            static const float uv[12] = { 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0 };
            for (const Glow &glow : glows) {
                if (glow.color.a <= 0.01f) continue;
                Vector3 r = right * glow.half_size, u = up * glow.half_size;
                Vector3 corners[6] = { glow.position - r - u, glow.position + r - u, glow.position + r + u,
                    glow.position - r - u, glow.position + r + u, glow.position - r + u };
                float xyz[18];
                for (int k = 0; k < 6; k++) {
                    xyz[k * 3] = corners[k].x; xyz[k * 3 + 1] = corners[k].y; xyz[k * 3 + 2] = corners[k].z;
                }
                Color premultiplied(glow.color.r * glow.color.a, glow.color.g * glow.color.a, glow.color.b * glow.color.a, 1.0f);
                if (vita_gxm_triangles3d(glow_texture, xyz, uv, 6, matrix, gxm_color(premultiplied))) glows_drawn++;
            }
            vita_gxm_blend_add(0);
            vita_gxm_depth_write(1);
        }
        glows.clear();
        if (meshes->arena_exhausted) {
            // Arena exhausted by geometry of meshes no longer in use: rebuild
            // resident geometry from the next frame on.
            meshes->reset_gpu_geometry();
            lit_cache.clear();
        }
        vita_gxm_scene3d_end();
        vita_gxm_tiltshift_resolve(float(settings->get_setting("rendering/vita/tilt_shift_top", 0.42f)), // Tilt-shift resolve
                float(settings->get_setting("rendering/vita/tilt_shift_bottom", 0.86f)),
                float(settings->get_setting("rendering/vita/tilt_shift_strength", 1.0f)));
        vita_gxm_clip(0, 0, 0, 960, 544);
        uint64_t submit_us = OS::get_singleton()->get_ticks_usec() - started;
        bool budget_reached = drawn_now.size() < draw_candidates.size();
        SWAP(drawn_last, drawn_now);
        // Moving average of CPU submission, excluding first-use geometry
        // builds, so loading a room does not shrink the budget and make props
        // pop out. The GPU wait is logged only: with vsync it measures idle
        // time waiting for the display, not GPU load.
        float gpu_wait = float(MIN(vita_gxm_last_gpu_wait_us(), 30000u));
        uint64_t steady_us = submit_us > build_us ? submit_us - build_us : 0;
        float sample = float(MIN(steady_us, uint64_t(30000)));
        submit_average_us = frames == 0 ? sample : submit_average_us * 0.9f + sample * 0.1f;
        gpu_wait_average_us = frames == 0 ? gpu_wait : gpu_wait_average_us * 0.9f + gpu_wait * 0.1f;
        if (submit_average_us > 14000.0f && adaptive_triangle_budget > configured_budget * 4 / 5) {
            adaptive_triangle_budget = MAX(configured_budget * 4 / 5, adaptive_triangle_budget - 500);
        } else if (submit_average_us < 9000.0f && adaptive_triangle_budget < configured_budget) {
            adaptive_triangle_budget = MIN(configured_budget, adaptive_triangle_budget + 1000);
        }
        if (frames++ < 8 || frames % 300 == 0) {
            fprintf(stderr, "[gxm3d] frame=%u visible_instances=%llu candidates=%u drawn=%u structural=%u skinned=%u skinned_us=%llu lights=%u decals=%u glows=%u lit=%u rays=%u distance_culled=%u submitted_triangles=%u budget=%u cpu_submit_us=%llu avg_submit_us=%u avg_gpu_wait_us=%u build_us=%u arena_kb=%u invalidations=%u\n",
                    frames, (unsigned long long)instances.size(), (unsigned int)draw_candidates.size(), (unsigned int)drawn_last.size(), structural_drawn, skinned_drawn, (unsigned long long)skinned_us, (unsigned int)active_lights.size(), decals_drawn, glows_drawn, lit_draws, shadow_rays_cast,
                    distance_culled, triangles, budget, (unsigned long long)submit_us, (unsigned int)submit_average_us, (unsigned int)gpu_wait_average_us, (unsigned int)build_us, vita_gxm_mesh_arena_used() / 1024, meshes->invalidations);
            fflush(stderr);
        }
        if (budget_reached) {
            WARN_PRINT_ONCE("GXM3D: prototype triangle budget reached; remaining geometry skipped.");
        }
    }
};
