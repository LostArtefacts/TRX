#include <trx/game/lara/skin/common.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/dynamic_enum.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/fmv.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/lara.h>
#include <trx/game/lara/skin/gold.h>
#include <trx/version.h>

#define M_NO_OUTFIT (-1)
#define M_NO_MESH (-1)

static LARA_SKIN_TYPE m_SkinType = LARA_SKIN_TYPE_DEFAULT;
static bool m_HolstersVisible = true;
static bool m_UseCombatFace = false;
static int32_t m_SpeechFace = M_NO_MESH;
static LARA_GUN_TYPE m_HolsterType_L = LGT_UNARMED;
static LARA_GUN_TYPE m_HolsterType_R = LGT_UNARMED;
static LARA_SKIN_EQUIPMENT m_Equipment[LM_NUMBER_OF] = {};
static OBJECT_MESH *m_MeshOverrides[LM_NUMBER_OF] = {};
static bool m_IsGolden = false;

// Answers with the default where she wears nothing yet, and leaves the type
// alone: a level being set up has none on purpose, and healing it here once
// left the outfit applied to nothing.
static inline const LARA_SKIN_OUTFIT *M_GetWornOutfit(void)
{
    const LARA_SKIN_TYPE type = Lara_Skin_IsOutfitAvailable(m_SkinType)
        ? m_SkinType
        : Lara_Skin_GetDefaultType();
    return Lara_Skin_GetOutfit(type);
}

static inline const LARA_SKIN_OUTFIT *M_GetCurrentOutfit(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetWornOutfit();
    if (!m_IsGolden) {
        return outfit;
    }
    const LARA_SKIN_OUTFIT *const gold = Lara_Skin_GetGoldOutfit(outfit);
    return gold != nullptr ? gold : outfit;
}

static LARA_SKIN_TYPE M_ResolveOutfitTypeFromName(
    const char *const outfit_name, const bool warn_on_invalid,
    const char *const source)
{
    if (outfit_name == nullptr) {
        return LARA_SKIN_TYPE_DEFAULT;
    }

    const LARA_SKIN_TYPE type = Lara_Skin_FindOutfitByName(outfit_name);
    if (Lara_Skin_IsOutfitAvailable(type)) {
        return type;
    }

    if (warn_on_invalid) {
        LOG_WARNING(
            "Invalid outfit '%s' from %s; falling back to default", outfit_name,
            source);
    }
    return LARA_SKIN_TYPE_DEFAULT;
}

static LARA_SKIN_TYPE M_GetFallbackOutfitType(void)
{
    return Lara_Skin_GetDefaultType();
}

static void M_SetConfigOutfit(const char *const outfit_name)
{
    ASSERT(outfit_name != nullptr);
    CONFIG_SET(g_Config.visuals.lara_outfit, outfit_name);
}

static LARA_SKIN_TYPE M_GetCurrentLevelOutfitType(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level == nullptr) {
        return M_GetFallbackOutfitType();
    }

    const LARA_SKIN_TYPE level_type = M_ResolveOutfitTypeFromName(
        level->lara_outfit, true, "gameflow level setting");
    if (level_type != LARA_SKIN_TYPE_DEFAULT) {
        return level_type;
    }
    return M_GetFallbackOutfitType();
}

static int32_t M_GetBraidDependentMeshIdx(
    const LARA_MESH mesh_idx, const LARA_SKIN_OUTFIT *const outfit)
{
    if (mesh_idx != LM_TORSO && mesh_idx != LM_HEAD) {
        return M_NO_MESH;
    }

    LARA_SKIN_EXTRA_MESH extra_id;
    switch (outfit->braid.mode) {
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
    default:
        return M_NO_MESH;
    }

    const OBJECT *const extra_obj = Object_Get(outfit->extra_obj_id);
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

    const OBJECT *const obj = Object_Get(outfit->legs_obj_id);
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

static inline const LARA_SKIN_OUTFIT *M_GetBraidOutfit(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if ((lara->mesh_effects & (1 << LM_HEAD)) == 0) {
        return outfit;
    }
    const LARA_SKIN_OUTFIT *const gold = Lara_Skin_GetGoldOutfit(outfit);
    return gold != nullptr ? gold : outfit;
}

static inline int32_t M_GetRelativeBraidOffset(const int32_t braid_idx)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    if (outfit->braid.count == 0) {
        return M_NO_MESH;
    }

    if (braid_idx < 0 || braid_idx >= outfit->braid.count) {
        return M_NO_MESH;
    }

    return outfit->braid.setup[braid_idx].mesh_offset;
}

static bool M_IsBraidEnabled(const LARA_SKIN_OUTFIT *const outfit)
{
    if (outfit->braid.count == 0
        || g_Config.visuals.braid_status == BRAID_STATUS_OFF) {
        return false;
    }

    return g_Config.visuals.braid_status == BRAID_STATUS_ON
        || outfit->braid.auto_enabled;
}

static inline int32_t M_GetMeshIdx(
    const LARA_MESH mesh, const LARA_SKIN_OUTFIT *const outfit)
{
    const OBJECT *const skin_obj = Object_Get(outfit->mesh_obj_id);
    int32_t offset = M_NO_MESH;

    if (M_IsBraidEnabled(outfit)) {
        offset = M_GetBraidDependentMeshIdx(mesh, outfit);
    }
    if (offset == M_NO_MESH) {
        offset = M_GetNoHolsterMeshIdx(mesh, outfit);
    }
    if (offset == M_NO_MESH) {
        offset = skin_obj->mesh_idx + mesh;
    }
    return offset;
}

static inline void M_ApplyMeshIfValid(
    const LARA_MESH mesh, const LARA_SKIN_OUTFIT *const outfit)
{
    if (m_MeshOverrides[mesh] != nullptr) {
        // A level's own mesh belongs to no outfit object, so it takes a twin
        // of its own to follow her into gold.
        OBJECT_MESH *const mesh_ptr = outfit->is_gold
            ? Lara_Skin_GetGoldMesh(m_MeshOverrides[mesh], outfit->gold_color)
            : m_MeshOverrides[mesh];
        Lara_Mesh_Set(mesh, mesh_ptr);
        return;
    }

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

    if (M_IsBraidEnabled(outfit)) {
        switch (outfit->braid.mode) {
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

    const OBJECT *const extra_obj = Object_Get(outfit->extra_obj_id);
    return extra_obj->mesh_idx + offset;
}

static const LARA_SKIN_OUTFIT *M_GetExtraOutfit(const LARA_EXTRA_STATE state)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    if (state == LS_EXTRA_MIDAS_KILL) {
        return Lara_Skin_GetGoldOutfit(outfit);
    }

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
    const int32_t data, const int32_t offset,
    const LARA_SKIN_OUTFIT *const outfit)
{
    LARA_SKIN_EQUIPMENT *const equipment = &m_Equipment[mesh];
    equipment->type = type;
    equipment->data = data;
    switch (type) {
    case EQUIPMENT_TYPE_WEAPON:
        const OBJECT *const gun_swap_obj = Object_Get(outfit->guns_obj_id);
        equipment->mesh = Object_GetMesh(gun_swap_obj->mesh_idx + offset);
        equipment->offset = (XYZ_16) {};
        break;
    case EQUIPMENT_TYPE_EXTRA:
        const OBJECT *const extra_obj = Object_Get(outfit->extra_obj_id);
        equipment->mesh = Object_GetMesh(extra_obj->mesh_idx + offset);
        equipment->offset = outfit->extra_mesh_positions[data];
        break;
    default:
        equipment->mesh = nullptr;
        equipment->offset = (XYZ_16) {};
        break;
    }
}

static void M_SetGunEquipment(
    const LARA_MESH mesh, const LARA_GUN_TYPE gun_type,
    const LARA_SKIN_OUTFIT *const outfit)
{
    const LARA_SKIN_MESH_MAP map =
        Lara_Skin_GetGunMeshMap(outfit->gun_map, gun_type);

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
    case LM_TORSO:
        offset = map.torso;
        break;
    default:
        break;
    }

    if (offset == M_NO_MESH) {
        Lara_Skin_ClearEquipment(mesh);
    } else {
        M_SetEquipment(mesh, EQUIPMENT_TYPE_WEAPON, gun_type, offset, outfit);
    }
}

static void M_SetSpeechFace(const int32_t index)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    if (index == M_NO_MESH) {
        const int32_t mesh_idx = M_GetMeshIdx(LM_HEAD, outfit);
        if (mesh_idx != M_NO_MESH) {
            Lara_Mesh_Set(LM_HEAD, Object_GetMesh(mesh_idx));
        }
        return;
    }

    if (outfit->speech_face_offset == M_NO_MESH) {
        return;
    }

    const OBJECT *const extra_obj = Object_Get(outfit->extra_obj_id);
    const int32_t offset = outfit->speech_face_offset + index;
    if (offset >= 0 && offset < extra_obj->mesh_count) {
        Lara_Mesh_Set(LM_HEAD, Object_GetMesh(extra_obj->mesh_idx + offset));
    }
}

static void M_SetCombatFace(const bool enabled)
{
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

static void M_UpdateSunglasses(void)
{
    const SUNGLASSES_MODE mode = g_Config.visuals.sunglasses_mode;
    if (mode == SUNGLASSES_MODE_OFF
        || !M_GetCurrentOutfit()->supports_sunglasses) {
        Lara_Skin_ClearEquipment(LM_HEAD);
        return;
    }

    const LARA_SKIN_EXTRA_MESH mesh = mode == SUNGLASSES_MODE_OPAQUE
        ? EXTRA_MESH_GLASSES_OPAQUE
        : EXTRA_MESH_GLASSES_TRANSPARENT;
    Lara_Skin_SetExtraEquipment(LM_HEAD, mesh);
}

static void M_ReapplyEquipmentEx(
    const LARA_SKIN_OUTFIT *const outfit, const LARA_MESH mesh)
{
    const LARA_SKIN_EQUIPMENT *const equipment = &m_Equipment[mesh];
    switch (equipment->type) {
    case EQUIPMENT_TYPE_WEAPON:
        M_SetGunEquipment(mesh, equipment->data, outfit);
        break;
    case EQUIPMENT_TYPE_EXTRA:
        M_SetEquipment(
            mesh, EQUIPMENT_TYPE_EXTRA, equipment->data,
            Lara_Skin_GetExtraMeshOffset(equipment->data), outfit);
        break;
    default:
        break;
    }
}

// What Lara carries comes from the outfit's objects too - the gun in her hands
// and the one on her back among it - so a change of outfit has to read it again
// from what each slot already holds. A slot holding nothing keeps holding
// nothing, which is what leaves the Midas death's cleared hands cleared.
static void M_ReapplyEquipment(const LARA_SKIN_OUTFIT *const outfit)
{
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        if (i == LM_THIGH_L || i == LM_THIGH_R) {
            continue;
        }
        M_ReapplyEquipmentEx(outfit, i);
    }
}

// Offer the player the outfits the level just loaded can dress her in. What it
// cannot is left in the setting, so an outfit chosen on one level is not lost
// on the next, but it is passed over as the setting is cycled.
static void M_RefreshOutfitChoices(void)
{
    const CONFIG_OPTION *const option =
        Config_FindOptionByMirror(&g_Config.visuals.lara_outfit);
    if (option == nullptr) {
        return;
    }

    const void *const token = Config_Option_GetEnumKey(option);
    const int32_t outfit_count = Lara_Skin_GetOutfitCount();
    for (int32_t i = 0; i < outfit_count; i++) {
        DynamicEnum_SetValueEnabled(
            token, Lara_Skin_GetOutfitName(i), Lara_Skin_IsOutfitAvailable(i));
    }
}

// Whether an outfit has this level's meshes to bind to. A title level running
// behind the menu is dressed like any other, so this asks what is loaded
// rather than whether a game is under way.
static bool M_CanDress(void)
{
    return GF_GetCurrentLevel() != nullptr && !FMV_IsPlaying()
        && Object_Get(O_LARA)->loaded
        && Object_Get(O_LARA_SKIN_SWAP_EXTRA)->loaded
        && Object_Get(O_LARA_SKIN_SWAP_GUNS)->loaded
        && Lara_Skin_IsOutfitAvailable(Lara_Skin_GetDefaultType());
}

void Lara_Skin_Reset(void)
{
    Lara_Skin_ResetGold();
}

void Lara_Skin_Initialise(void)
{
    // Before the bail below, so a level that cannot dress her does not inherit
    // the outgoing level's outfit - the meshes it names are not loaded here.
    m_SkinType = M_NO_OUTFIT;
    m_HolsterType_L = LGT_UNARMED;
    m_HolsterType_R = LGT_UNARMED;
    m_UseCombatFace = false;
    m_SpeechFace = M_NO_MESH;

    m_HolstersVisible = true;
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        m_Equipment[i].visible = true;
        // Before the clear, which applies a mesh and would read it.
        m_MeshOverrides[i] = nullptr;
        Lara_Skin_ClearEquipment(i);
    }

    M_RefreshOutfitChoices();

    // A level need not carry the swap objects: a title that never shows her
    // has no use for them. No outfit is applied then, and she keeps the meshes
    // the level loaded for her. A level that does hold her and not them
    // used to fail an assertion here, and is still an incomplete install.
    if (!M_CanDress()) {
        if (GF_GetCurrentLevel() != nullptr && Object_Get(O_LARA)->loaded) {
            LOG_WARNING("No skin swap objects here; no outfit applied");
        }
        return;
    }

    const int32_t outfit_count = Lara_Skin_GetOutfitCount();
    for (int32_t i = 0; i < outfit_count; i++) {
        if (!Lara_Skin_IsOutfitAvailable(i) && Lara_Skin_IsOutfitDefined(i)) {
            LOG_WARNING(
                "Outfit '%s' has no meshes in this level",
                Lara_Skin_GetOutfitName(i));
        }
    }
    Lara_Skin_ApplyOutfitFromConfig();

    // She wears nothing yet, and the outfit above only dresses her where it
    // differs from the type she already carries, which after a level change is
    // the one the outgoing level left there. Dress her regardless: a mesh left
    // unset here is read as a mesh when she is drawn.
    Lara_Skin_ApplyOutfit();
}

void Lara_Skin_ApplyOutfitFromConfig(void)
{
    if (!M_CanDress()) {
        return;
    }

    const bool was_golden = m_IsGolden;
    m_IsGolden = g_Config.visuals.golden_lara;
    LARA_SKIN_TYPE skin_type = M_GetCurrentLevelOutfitType();
    if (g_Config.visuals.lara_outfit != nullptr) {
        const LARA_SKIN_TYPE config_type =
            Lara_Skin_FindOutfitByName(g_Config.visuals.lara_outfit);
        if (!Lara_Skin_IsOutfitAvailable(config_type)) {
            LOG_WARNING(
                "Invalid outfit '%s' from config.visuals.lara_outfit; falling "
                "back to default",
                g_Config.visuals.lara_outfit);
            skin_type = M_GetCurrentLevelOutfitType();
        } else {
            skin_type = config_type;
        }
    }

    Lara_Skin_SetType(skin_type);

    // Gold is not an outfit of its own, so a change of it leaves the type
    // where it was, and the meshes she is wearing have to be read again.
    if (was_golden != m_IsGolden) {
        Lara_Skin_ApplyOutfit();
    }
}

void Lara_Skin_CycleOutfit(const int32_t dir)
{
    if (!Game_IsLoaded()) {
        return;
    }

    const CONFIG_OPTION *const option =
        Config_FindOptionByMirror(&g_Config.visuals.lara_outfit);
    if (option != nullptr && Config_Option_IsHeld(option)) {
        return;
    }

    // A level whose meshes dress none of the outfits leaves her undressed,
    // with nothing to cycle from.
    if (!Lara_Skin_IsOutfitAvailable(m_SkinType)) {
        return;
    }

    // Update the config twice to guarantee the change is submitted in cases
    // where Lara_Skin_SetType has been called manually for non-permanent swaps
    // e.g. by Lua in cutscenes.
    const char *const current_name = Lara_Skin_GetOutfitName(m_SkinType);
    ASSERT(current_name != nullptr);

    if (g_Config.visuals.lara_outfit == nullptr
        || !String_Equivalent(g_Config.visuals.lara_outfit, current_name)) {
        M_SetConfigOutfit(current_name);
        Config_Update();
    }

    const int32_t outfit_count = Lara_Skin_GetOutfitCount();
    int32_t type = m_SkinType;
    for (int32_t i = 0; i < outfit_count; i++) {
        type = (type + dir + outfit_count) % outfit_count;
        if (Lara_Skin_IsOutfitAvailable(type)
            && Lara_Skin_GetOutfit(type)->is_selectable) {
            break;
        }
    }

    M_SetConfigOutfit(Lara_Skin_GetOutfitName(type));
    Config_Update();
}

LARA_SKIN_TYPE Lara_Skin_GetType(void)
{
    return m_SkinType;
}

bool Lara_Skin_IsDefaultType(void)
{
    if (g_Config.visuals.lara_outfit != nullptr) {
        const LARA_SKIN_TYPE config_type =
            Lara_Skin_FindOutfitByName(g_Config.visuals.lara_outfit);
        if (Lara_Skin_IsOutfitAvailable(config_type)) {
            return m_SkinType == config_type;
        }
        return m_SkinType == M_GetCurrentLevelOutfitType();
    }
    return m_SkinType == M_GetCurrentLevelOutfitType();
}

void Lara_Skin_SetType(const LARA_SKIN_TYPE skin_type)
{
    LARA_SKIN_TYPE new_skin_type = skin_type;
    if (!Lara_Skin_IsOutfitAvailable(new_skin_type)) {
        new_skin_type = M_GetFallbackOutfitType();
    }
    if (m_SkinType == new_skin_type) {
        return;
    }

    m_SkinType = new_skin_type;
    Lara_Skin_ApplyOutfit();
}

void Lara_Skin_ApplyOutfit(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        M_ApplyMeshIfValid(i, outfit);
    }

    // The thighs answer to the holster type rather than to what they hold, so
    // that hidden holsters come back with the outfit that has them.
    M_SetGunEquipment(LM_THIGH_L, m_HolsterType_L, outfit);
    M_SetGunEquipment(LM_THIGH_R, m_HolsterType_R, outfit);
    M_ReapplyEquipment(outfit);
    M_SetCombatFace(m_UseCombatFace);
    if (m_SpeechFace != M_NO_MESH) {
        M_SetSpeechFace(m_SpeechFace);
    }
    M_UpdateSunglasses();
    Lara_Joints_Initialise(outfit);
    Lara_Hair_InitJoints(outfit);
}

void Lara_Skin_SetMeshOverride(
    const LARA_MESH mesh, OBJECT_MESH *const mesh_ptr)
{
    m_MeshOverrides[mesh] = mesh_ptr;
    // A level script runs before she has been dressed, and an outfit that is
    // not applied yet names meshes that are not loaded. Applying an outfit
    // reads the override, so the one set here still reaches her.
    if (M_CanDress()) {
        M_ApplyMeshIfValid(mesh, M_GetCurrentOutfit());
    }
}

OBJECT_MESH *Lara_Skin_GetMeshOverride(const LARA_MESH mesh)
{
    return m_MeshOverrides[mesh];
}

void Lara_Skin_SetCombatFace(const bool enabled)
{
    if (m_UseCombatFace != enabled) {
        M_SetCombatFace(enabled);
    }
}

void Lara_Skin_SetSpeechFace(const int32_t index)
{
    m_SpeechFace = index;
    M_SetSpeechFace(index);
}

int32_t Lara_Skin_GetSpeechFace(void)
{
    return m_SpeechFace;
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
    Lara_Joints_SwapSingle(mesh, outfit);
    M_ReapplyEquipmentEx(outfit, mesh);
}

const ANIM_BONE *Lara_Skin_GetBoneBase(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    const OBJECT *const skin_obj = Object_Get(outfit->mesh_obj_id);
    return Object_TryGetBone(skin_obj, 0);
}

bool Lara_Skin_IsBraidSupported(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    return M_IsBraidEnabled(outfit)
        && Lara_Skin_GetBraidMeshIdx(0) != M_NO_MESH;
}

const LARA_SKIN_BRAID *Lara_Skin_GetBraid(void)
{
    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    return &outfit->braid;
}

int32_t Lara_Skin_GetBraidMeshIdx(const int32_t braid_idx)
{
    const int32_t offset = M_GetRelativeBraidOffset(braid_idx);
    if (offset == M_NO_MESH) {
        return offset;
    }

    const OBJECT *const obj = Object_Get(M_GetBraidOutfit()->extra_obj_id);
    return obj->mesh_idx + offset;
}

const ANIM_BONE *Lara_Skin_GetBraidBoneBase(const int32_t braid_idx)
{
    const int32_t offset = M_GetRelativeBraidOffset(braid_idx);
    if (offset == M_NO_MESH) {
        return nullptr;
    }

    // The worn outfit, not the gilded one: a twin carries the bones of the
    // object it was minted from, and this is asked while a level is still
    // loading, before there is anything to mint from.
    const OBJECT *const obj = Object_Get(M_GetWornOutfit()->extra_obj_id);
    return Object_TryGetBone(obj, offset);
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
    M_SetEquipment(
        mesh, EQUIPMENT_TYPE_NONE, M_NO_MESH, M_NO_MESH, M_GetWornOutfit());
}

void Lara_Skin_SetExtraEquipment(
    const LARA_MESH mesh, const LARA_SKIN_EXTRA_MESH extra_mesh)
{
    const int32_t offset = Lara_Skin_GetExtraMeshOffset(extra_mesh);
    M_SetEquipment(
        mesh, EQUIPMENT_TYPE_EXTRA, extra_mesh, offset, M_GetCurrentOutfit());
}

void Lara_Skin_SetGunEquipment(
    const LARA_MESH mesh, const LARA_GUN_TYPE gun_type)
{
    // The armed meshes live in the swap object, and a level need not carry it.
    if (!Gun_Registry_IsValidType(gun_type)
        || !Object_Get(M_GetCurrentOutfit()->guns_obj_id)->loaded) {
        return;
    }
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

SAMPLE_SLOT Lara_Skin_GetAnimSFX(const SAMPLE_SLOT sample_id)
{
    if (g_TRVersion == 2 && !g_Config.audio.enable_ps1_sfx) {
        return sample_id;
    }

    const LARA_SKIN_OUTFIT *const outfit = M_GetCurrentOutfit();
    if (!outfit->is_barefoot) {
        return sample_id;
    }

    const SAMPLE_ID trx_id = Sound_SlotToID(sample_id);
    switch (trx_id) {
    case SFX_LARA_FOOTSTEP:
        return Sound_IDToSlot(SFX_LARA_BAREFOOT);
    case SFX_LARA_LAND:
        return Sound_IDToSlot(SFX_LARA_BAREFOOT_LAND);
    default:
        return sample_id;
    }
}
