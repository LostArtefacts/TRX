#include <trx/game/lara/skin/gold.h>

#include <trx/core/vector.h>
#include <trx/game/game_buf.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/common.h>

typedef struct {
    OBJECT_ID src_id;
    OBJECT_ID twin_id;
    RGB_888 color;
} M_TWIN;

typedef struct {
    const OBJECT_MESH *src;
    OBJECT_MESH *twin;
    RGB_888 color;
} M_MESH_TWIN;

static VECTOR *m_Twins = nullptr;
static VECTOR *m_MeshTwins = nullptr;
static LARA_SKIN_OUTFIT m_GoldOutfit;

static bool M_ColorMatches(const RGB_888 a, const RGB_888 b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

// The twin shares the vertices, normals and faces it was minted from - only
// the header says to draw them in gold.
static OBJECT_MESH *M_CloneMesh(
    const OBJECT_MESH *const src, const RGB_888 color)
{
    OBJECT_MESH *const mesh = GameBuf_Alloc(sizeof(OBJECT_MESH), GBUF_MESHES);
    *mesh = *src;
    mesh->enable_solid_color = true;
    mesh->solid_color = color;
    mesh->enable_reflections = true;
    Object_StoreMesh(mesh);
    return mesh;
}

static OBJECT_ID M_FindTwin(const OBJECT_ID src_id, const RGB_888 color)
{
    if (m_Twins == nullptr) {
        return NO_OBJECT;
    }
    for (int32_t i = 0; i < m_Twins->count; i++) {
        const M_TWIN *const twin = Vector_Get(m_Twins, i);
        if (twin->src_id == src_id && M_ColorMatches(twin->color, color)) {
            return twin->twin_id;
        }
    }
    return NO_OBJECT;
}

static OBJECT_ID M_MintTwin(
    const OBJECT_ID src_id, const RGB_888 color, bool *const minted)
{
    const OBJECT *const src = Object_TryGet(src_id);
    if (src == nullptr || !src->loaded) {
        return src_id;
    }

    const OBJECT_ID twin_id = Object_Mint();
    OBJECT *const twin = Object_Get(twin_id);
    twin->mesh_count = src->mesh_count;
    twin->bone_idx = src->bone_idx;
    twin->mesh_idx = Object_GetMeshCount();
    twin->loaded = true;

    for (int32_t i = 0; i < src->mesh_count; i++) {
        M_CloneMesh(Object_GetMesh(src->mesh_idx + i), color);
    }

    if (m_Twins == nullptr) {
        m_Twins = Vector_Create(sizeof(M_TWIN));
    }
    const M_TWIN entry = {
        .src_id = src_id,
        .twin_id = twin_id,
        .color = color,
    };
    Vector_Add(m_Twins, (void *)&entry);
    *minted = true;
    return twin_id;
}

static OBJECT_ID M_GetTwin(
    const OBJECT_ID src_id, const RGB_888 color, bool *const minted)
{
    const OBJECT_ID twin_id = M_FindTwin(src_id, color);
    if (twin_id != NO_OBJECT) {
        return twin_id;
    }
    return M_MintTwin(src_id, color, minted);
}

OBJECT_MESH *Lara_Skin_GetGoldMesh(OBJECT_MESH *const mesh, const RGB_888 color)
{
    if (mesh == nullptr) {
        return nullptr;
    }

    if (m_MeshTwins == nullptr) {
        m_MeshTwins = Vector_Create(sizeof(M_MESH_TWIN));
    }
    for (int32_t i = 0; i < m_MeshTwins->count; i++) {
        const M_MESH_TWIN *const twin = Vector_Get(m_MeshTwins, i);
        if (twin->src == mesh && M_ColorMatches(twin->color, color)) {
            return twin->twin;
        }
    }

    const M_MESH_TWIN entry = {
        .src = mesh,
        .twin = M_CloneMesh(mesh, color),
        .color = color,
    };
    Vector_Add(m_MeshTwins, (void *)&entry);
    Output_RefreshObjectMeshes();
    return entry.twin;
}

const LARA_SKIN_OUTFIT *Lara_Skin_GetGoldOutfit(
    const LARA_SKIN_OUTFIT *const outfit)
{
    if (outfit == nullptr || !outfit->is_defined) {
        return nullptr;
    }

    const RGB_888 color = outfit->gold_color;
    bool minted = false;
    m_GoldOutfit = *outfit;
    m_GoldOutfit.is_gold = true;
    m_GoldOutfit.mesh_obj_id = M_GetTwin(outfit->mesh_obj_id, color, &minted);
    m_GoldOutfit.joints_obj_id =
        M_GetTwin(outfit->joints_obj_id, color, &minted);
    m_GoldOutfit.extra_obj_id = M_GetTwin(outfit->extra_obj_id, color, &minted);
    m_GoldOutfit.guns_obj_id = M_GetTwin(outfit->guns_obj_id, color, &minted);
    m_GoldOutfit.legs_obj_id = M_GetTwin(outfit->legs_obj_id, color, &minted);

    // The meshes arrive after the level's were batched for drawing, so the
    // batches have to be built again to take them in.
    if (minted) {
        Output_RefreshObjectMeshes();
    }

    return &m_GoldOutfit;
}

void Lara_Skin_ResetGold(void)
{
    if (m_Twins != nullptr) {
        Vector_Free(m_Twins);
        m_Twins = nullptr;
    }
    if (m_MeshTwins != nullptr) {
        Vector_Free(m_MeshTwins);
        m_MeshTwins = nullptr;
    }
}
