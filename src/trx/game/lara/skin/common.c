#include <trx/game/lara/skin/common.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/lara.h>
#include <trx/version.h>

#define M_NO_OUTFIT (-1)
#define M_NO_MESH (-1)

static LARA_SKIN_TYPE m_SkinType = LARA_SKIN_TYPE_01;
static bool m_HolstersVisible = true;
static bool m_UseCombatFace = false;
static LARA_GUN_TYPE m_HolsterType_L = LGT_UNARMED;
static LARA_GUN_TYPE m_HolsterType_R = LGT_UNARMED;
static LARA_SKIN_EQUIPMENT m_Equipment[LM_NUMBER_OF] = {};

static inline const LARA_SKIN_OUTFIT *M_GetCurrentOutfit(void)
{
    return Lara_Skin_GetOutfit(m_SkinType);
}

static int32_t M_GetBraidDependentMeshIdx(
    const LARA_MESH mesh_idx, const LARA_SKIN_OUTFIT *const outfit)
{
    if (mesh_idx != LM_TORSO && mesh_idx != LM_HEAD) {
        return M_NO_MESH;
    }

    LARA_SKIN_EXTRA_MESH extra_id;
    switch (outfit->braid_mode) {
    case BRAID_MODE_TR1_HEAD_ONLY:
        if (mesh_idx != LM_HEAD) {
            return M_NO_MESH;
        }
        extra_id = EXTRA_MESH_TR1_BRAID_DEFAULT_HEAD;
        break;
    case BRAID_MODE_TR1_FULL:
        extra_id = mesh_idx == LM_TORSO ? EXTRA_MESH_TR1_BRAID_DEFAULT_TORSO
                                        : EXTRA_MESH_TR1_BRAID_DEFAULT_HEAD;
        break;
    case BRAID_MODE_TR1_MAULED:
        extra_id = mesh_idx == LM_TORSO ? EXTRA_MESH_TR1_BRAID_MAULED_TORSO
                                        : EXTRA_MESH_TR1_BRAID_DEFAULT_HEAD;
        break;
    case BRAID_MODE_TR1_GOLD:
        extra_id = mesh_idx == LM_TORSO ? EXTRA_MESH_TR1_BRAID_GOLD_TORSO
                                        : EXTRA_MESH_TR1_BRAID_GOLD_HEAD;
        break;
    default:
        return M_NO_MESH;
    }

    const OBJECT *const extra_obj = Object_Get(O_LARA_SKIN_SWAP_EXTRA);
    const int32_t offset = Lara_Skin_GetExtraMeshOffset(extra_id);
    return extra_obj->mesh_idx + offset;
}

static int32_t M_GetNoHolsterMeshIdx(
    const LARA_MESH mesh, const LARA_SKIN_OUTFIT *const outfit)
{
    if (m_HolstersVisible) {
        return M_NO_MESH;
    }

    if (mesh != LM_THIGH_L && mesh != LM_THIGH_R) {
        return M_NO_MESH;
    }

    const OBJECT *const obj = Object_Get(O_LARA_SKIN_SWAP_LEGS);
    if (!obj->loaded) {
        return M_NO_MESH;
    }

    const int32_t offset = mesh == LM_THIGH_L
        ? outfit->no_holster_offsets.left
        : outfit->no_holster_offsets.right;
    if (offset == M_NO_MESH) {
        return M_NO_MESH;
    }

    return obj->mesh_idx + offset;
}

static inline int32_t M_GetMeshIdx(
    const LARA_MESH mesh, const LARA_SKIN_OUTFIT *const outfit)
{
    const OBJECT *const skin_obj = Object_Get(outfit->obj_id);
    int32_t offset = M_NO_MESH;

    if (g_Config.visuals.enable_braid) {
        offset = M_GetBraidDependentMeshIdx(mesh, outfit);
    }
    if (offset == M_NO_MESH) {
        offset = M_GetNoHolsterMeshIdx(mesh, outfit);
    }
    if (offset == M_NO_MESH) {
        offset = skin_obj->mesh_idx + outfit->mesh_offset + mesh;
    }
    return offset;
}

static inline void M_ApplyMeshIfValid(
    const LARA_MESH mesh, const LARA_SKIN_OUTFIT *const outfit)
{
    const int32_t mesh_idx = M_GetMeshIdx(mesh, outfit);
    if (mesh_idx != M_NO_MESH) {
        Lara_Mesh_Set(mesh, Object_GetMesh(mesh_idx));
    }
}

static int32_t M_GetCombatFaceMeshIdx(const LARA_SKIN_OUTFIT *const outfit)
{
    int32_t offset = outfit->combat_face_offset;
    if (offset == M_NO_MESH) {
        return M_NO_MESH;
    }

    if (g_Config.visuals.enable_braid) {
        switch (outfit->braid_mode) {
        case BRAID_MODE_TR1_HEAD_ONLY:
        case BRAID_MODE_TR1_FULL:
        case BRAID_MODE_TR1_MAULED:
            offset =
                Lara_Skin_GetExtraMeshOffset(EXTRA_MESH_TR1_BRAID_COMBAT_HEAD);
            break;
        default:
            break;
        }
    }

    const OBJECT *const extra_obj = Object_Get(O_LARA_SKIN_SWAP_EXTRA);
    return extra_obj->mesh_idx + offset;
}

static const LARA_SKIN_OUTFIT *M_GetExtraOutfit(const LARA_EXTRA_STATE state)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    const LARA_SKIN_TYPE extra_type = outfit->extra_outfits[state];
    if (extra_type == LARA_SKIN_TYPE_DEFAULT) {
        return nullptr;
    }

    if (!Lara_Skin_IsOutfitAvailable(extra_type)) {
        return nullptr;
    }

    return Lara_Skin_GetOutfit(extra_type);
}

static void M_SetEquipment(
    const LARA_MESH mesh, const LARA_SKIN_EQUIPMENT_TYPE type,
    const int32_t data, const int32_t offset)
{
    LARA_SKIN_EQUIPMENT *const equipment = &m_Equipment[mesh];
    equipment->type = type;
    equipment->data = data;
    switch (type) {
    case EQUIPMENT_TYPE_WEAPON:
        const OBJECT *const gun_swap_obj = Object_Get(O_LARA_SKIN_SWAP_GUNS);
        equipment->mesh = Object_GetMesh(gun_swap_obj->mesh_idx + offset);
        break;
    case EQUIPMENT_TYPE_EXTRA:
        const OBJECT *const extra_obj = Object_Get(O_LARA_SKIN_SWAP_EXTRA);
        equipment->mesh = Object_GetMesh(extra_obj->mesh_idx + offset);
        break;
    default:
        equipment->mesh = nullptr;
        break;
    }
}

static void M_SetGunEquipment(
    const LARA_MESH mesh, const LARA_GUN_TYPE gun_type,
    const LARA_SKIN_OUTFIT *const outfit)
{
    const OBJECT *const gun_swap_obj = Object_Get(O_LARA_SKIN_SWAP_GUNS);
    const LARA_SKIN_MESH_MAP map = outfit->gun_map->mesh_offsets[gun_type];

    int32_t offset = M_NO_MESH;
    switch (mesh) {
    case LM_THIGH_L:
        offset = map.thigh.left;
        break;
    case LM_THIGH_R:
        offset = map.thigh.right;
        break;
    case LM_HAND_L:
        offset = map.hand.left;
        break;
    case LM_HAND_R:
        offset = map.hand.right;
        break;
    default:
        break;
    }

    if (offset == M_NO_MESH) {
        Lara_Skin_ClearEquipment(mesh);
    } else {
        M_SetEquipment(mesh, EQUIPMENT_TYPE_WEAPON, gun_type, offset);
    }
}

void Lara_Skin_Initialise(void)
{
    m_SkinType = M_NO_OUTFIT;
    m_HolsterType_L = LGT_UNARMED;
    m_HolsterType_R = LGT_UNARMED;
    m_UseCombatFace = false;

    m_HolstersVisible = true;
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        m_Equipment[i].visible = true;
    }

    const OBJECT *const extra_obj = Object_Get(O_LARA_SKIN_SWAP_EXTRA);
    ASSERT(extra_obj->loaded);

    const OBJECT *const gun_swap_obj = Object_Get(O_LARA_SKIN_SWAP_GUNS);
    ASSERT(gun_swap_obj->loaded);

    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        Lara_Skin_ClearEquipment(i);
    }

    const int32_t outfit_count = Lara_Skin_GetOutfitCount();
    for (int32_t i = 0; i < outfit_count; i++) {
        const LARA_SKIN_OUTFIT *const outfit = Lara_Skin_GetOutfit(i);
        if (!outfit->is_defined) {
            continue;
        }

        const OBJECT *const skin_obj = Object_Get(outfit->obj_id);
        ASSERT(skin_obj->loaded);
        if (!outfit->is_reflective) {
            continue;
        }

        for (int32_t j = 0; j < LM_NUMBER_OF; j++) {
            Object_SetMeshReflective(
                outfit->obj_id, outfit->mesh_offset + j, true);
            const int32_t extra_idx = M_GetBraidDependentMeshIdx(j, outfit);
            if (extra_idx != M_NO_MESH) {
                Object_SetMeshReflectiveEx(extra_idx, true);
            }
        }

        for (int32_t j = 0; j < NUM_WEAPONS; j++) {
            const LARA_SKIN_MESH_MAP map = outfit->gun_map->mesh_offsets[j];
            if (map.thigh.left != M_NO_MESH) {
                Object_SetMeshReflectiveEx(
                    gun_swap_obj->mesh_idx + map.thigh.left, true);
            }
            if (map.thigh.right != M_NO_MESH) {
                Object_SetMeshReflectiveEx(
                    gun_swap_obj->mesh_idx + map.thigh.right, true);
            }
        }
    }

    const int32_t braid_count = Lara_Skin_GetBraidCount();
    const int32_t hair_segment_count = Lara_Hair_GetSegmentCount();
    for (int32_t i = 0; i < braid_count; i++) {
        const LARA_SKIN_BRAID *const braid = Lara_Skin_GetBraid(i);
        if (braid->gold_offset == M_NO_MESH) {
            continue;
        }

        for (int32_t j = 0; j < hair_segment_count; j++) {
            Object_SetMeshReflective(
                O_LARA_SKIN_SWAP_EXTRA, braid->gold_offset + j, true);
        }
    }

    Lara_Skin_ApplyOutfitFromConfig();
}

void Lara_Skin_ApplyOutfitFromConfig(void)
{
    if (!Game_IsLoaded()) {
        return;
    }

    const LARA_SKIN_TYPE skin_type =
        g_Config.visuals.lara_skin_type == LARA_SKIN_TYPE_DEFAULT
        ? GF_GetCurrentLevel()->lara_skin_type
        : g_Config.visuals.lara_skin_type;
    Lara_Skin_SetType(skin_type);
}

void Lara_Skin_CycleOutfit(const int32_t dir)
{
    if (!Game_IsLoaded()) {
        return;
    }

    if (Config_IsOptionEnforced(&g_Config.visuals.lara_skin_type)) {
        return;
    }

    // Update the config twice to guarantee the change is submitted in cases
    // where Lara_Skin_SetType has been called manually for non-permanent swaps
    // e.g. by Lua in cutscenes.
    if (m_SkinType != g_Config.visuals.lara_skin_type) {
        g_Config.visuals.lara_skin_type = m_SkinType;
        Config_Update();
    }

    const int32_t outfit_count = Lara_Skin_GetOutfitCount();
    int32_t type = m_SkinType;
    do {
        type += dir;
        type += outfit_count;
        type %= outfit_count;
    } while (!Lara_Skin_IsOutfitAvailable(type));

    g_Config.visuals.lara_skin_type = type;
    Config_Update();
}

LARA_SKIN_TYPE Lara_Skin_GetType(void)
{
    return m_SkinType;
}

bool Lara_Skin_IsDefaultType(void)
{
    return m_SkinType == g_Config.visuals.lara_skin_type
        || m_SkinType == GF_GetCurrentLevel()->lara_skin_type;
}

void Lara_Skin_SetType(const LARA_SKIN_TYPE skin_type)
{
    if (m_SkinType == skin_type) {
        return;
    }

    m_SkinType = skin_type;
    Lara_Skin_ApplyOutfit();
}

void Lara_Skin_ApplyOutfit(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        M_ApplyMeshIfValid(i, outfit);
    }

    M_SetGunEquipment(LM_THIGH_L, m_HolsterType_L, outfit);
    M_SetGunEquipment(LM_THIGH_R, m_HolsterType_R, outfit);
}

void Lara_Skin_SetCombatFace(const bool enabled)
{
    if (m_UseCombatFace == enabled) {
        return;
    }

    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    int32_t mesh_idx = M_NO_MESH;
    if (enabled) {
        mesh_idx = M_GetCombatFaceMeshIdx(outfit);
    } else {
        mesh_idx = M_GetMeshIdx(LM_HEAD, outfit);
    }

    if (mesh_idx != M_NO_MESH) {
        Lara_Mesh_Set(LM_HEAD, Object_GetMesh(mesh_idx));
        m_UseCombatFace = enabled;
    }
}

void Lara_Skin_SwapAllExtra(const LARA_EXTRA_STATE state)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetExtraOutfit(state);
    if (outfit == nullptr) {
        return;
    }

    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        M_ApplyMeshIfValid(i, outfit);
    }

    M_SetGunEquipment(LM_THIGH_L, m_HolsterType_L, outfit);
    M_SetGunEquipment(LM_THIGH_R, m_HolsterType_R, outfit);
}

void Lara_Skin_SwapSingleExtra(
    const LARA_MESH mesh, const LARA_EXTRA_STATE state)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetExtraOutfit(state);
    if (outfit == nullptr) {
        return;
    }

    M_ApplyMeshIfValid(mesh, outfit);

    if (mesh == LM_THIGH_L) {
        M_SetGunEquipment(LM_THIGH_L, m_HolsterType_L, outfit);
    } else if (mesh == LM_THIGH_R) {
        M_SetGunEquipment(LM_THIGH_R, m_HolsterType_R, outfit);
    }
}

const ANIM_BONE *Lara_Skin_GetBoneBase(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    const OBJECT *const skin_obj = Object_Get(outfit->obj_id);
    return Object_GetBone(skin_obj, outfit->mesh_offset);
}

bool Lara_Skin_IsBraidSupported(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    return outfit->braid != nullptr && Lara_Skin_GetBraidMeshIdx() != M_NO_MESH;
}

XYZ_32 Lara_Skin_GetBraidOffset(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    return outfit->braid != nullptr ? outfit->braid->hair_pos : (XYZ_32) {};
}

int32_t Lara_Skin_GetBraidMeshIdx(void)
{
    const OBJECT *const obj = Object_Get(O_LARA_SKIN_SWAP_EXTRA);
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    if (outfit->is_reflective || (lara->mesh_effects & (1 << LM_HEAD)) != 0) {
        return obj->mesh_idx + outfit->braid->gold_offset;
    }
    return obj->mesh_idx + outfit->braid->mesh_offset;
}

bool Lara_Skin_AreHolstersVisible(void)
{
    return m_HolstersVisible;
}

void Lara_Skin_SetHolstersVisible(const bool visible)
{
    m_HolstersVisible = visible;
    m_Equipment[LM_THIGH_L].visible = visible;
    m_Equipment[LM_THIGH_R].visible = visible;

    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    M_ApplyMeshIfValid(LM_THIGH_L, outfit);
    M_ApplyMeshIfValid(LM_THIGH_R, outfit);
}

void Lara_Skin_ClearEquipment(const LARA_MESH mesh)
{
    M_SetEquipment(mesh, EQUIPMENT_TYPE_NONE, M_NO_MESH, M_NO_MESH);
}

void Lara_Skin_SetExtraEquipment(
    const LARA_MESH mesh, const LARA_SKIN_EXTRA_MESH extra_mesh)
{
    const int32_t offset = Lara_Skin_GetExtraMeshOffset(extra_mesh);
    M_SetEquipment(mesh, EQUIPMENT_TYPE_EXTRA, extra_mesh, offset);
}

void Lara_Skin_SetGunEquipment(
    const LARA_MESH mesh, const LARA_GUN_TYPE gun_type)
{
    M_SetGunEquipment(mesh, gun_type, M_GetCurrentOutfit());

    if (mesh == LM_THIGH_L) {
        m_HolsterType_L = gun_type;
    } else if (mesh == LM_THIGH_R) {
        m_HolsterType_R = gun_type;
    }

    if ((mesh == LM_THIGH_L || mesh == LM_THIGH_R)
        && !Gun_IsRifleType(gun_type)) {
        Lara_Skin_SetHolstersVisible(true);
    }
}

const LARA_SKIN_EQUIPMENT *Lara_Skin_GetEquipment(const LARA_MESH mesh)
{
    return &m_Equipment[mesh];
}

SAMPLE_ID Lara_Skin_GetAnimSFX(const SAMPLE_ID sample_id)
{
    if (g_TRVersion == 2 && !g_Config.audio.enable_ps1_sfx) {
        return sample_id;
    }

    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    if (outfit->footstep_sample_id == SFX_TRX_INVALID
        || Sound_FromGameID(sample_id) != SFX_LARA_FOOTSTEP) {
        return sample_id;
    }

    return Sound_ToGameID(outfit->footstep_sample_id);
}

// TODO: remove in TRX 1.5.
void Lara_Skin_ExtractLegacyEquipment(const OBJECT_MESH **const meshes)
{
#define L_DETERMINE_EQUIPMENT(mesh)                                            \
    do {                                                                       \
        if (meshes[mesh] == nullptr) {                                         \
            break;                                                             \
        }                                                                      \
        for (int32_t i = 0; i < NUM_WEAPONS; i++) {                            \
            if (i == LGT_SKIDOO) {                                             \
                continue;                                                      \
            }                                                                  \
            const OBJECT *const obj = Object_Get(Gun_GetWeaponAnim(i));        \
            if (obj->loaded                                                    \
                && Object_GetMesh(obj->mesh_idx + mesh) == meshes[mesh]) {     \
                Lara_Skin_SetGunEquipment(mesh, i);                            \
                break;                                                         \
            }                                                                  \
        }                                                                      \
    } while (0)

    L_DETERMINE_EQUIPMENT(LM_THIGH_L);
    L_DETERMINE_EQUIPMENT(LM_THIGH_R);
    L_DETERMINE_EQUIPMENT(LM_HAND_L);
    L_DETERMINE_EQUIPMENT(LM_HAND_R);

#undef L_DETERMINE_EQUIPMENT
}
