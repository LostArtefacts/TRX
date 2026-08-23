#include <trx/game/lara/mesh.h>

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>

static OBJECT_MESH *m_Meshes[LM_NUMBER_OF] = {};

static LARA_GUN_TYPE M_DetermineHolsterGun(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->holsters_gun_type != LGT_UNARMED) {
        return lara_info->holsters_gun_type;
    }
    // What she is holding goes back into her holsters ahead of anything else,
    // so long as it is small enough to fit in them.
    if (lara_info->gun_type != LGT_UNARMED
        && !Gun_IsRifleType(lara_info->gun_type)) {
        return lara_info->gun_type;
    }
    return Gun_GetHolsterChoice(Inv_GetState());
}

static LARA_GUN_TYPE M_DetermineBackGun(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->back_gun_type != LGT_UNARMED) {
        return lara_info->back_gun_type;
    }
    return Gun_GetBackChoice(Inv_GetState());
}

static void M_EnsureDefaultDualPistolMesh(const LARA_GUN_TYPE holster_gun)
{
    if (Gun_Registry_Get(holster_gun)->type != WEAPON_TYPE_SINGLE_PISTOL) {
        return;
    }

    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const weapon = Gun_Registry_GetByIndex(i);
        const LARA_GUN_TYPE gun = weapon->gun_type;
        if (weapon->type == WEAPON_TYPE_DUAL_PISTOLS
            && Inv_HasItem(Gun_GetGunObject(gun))) {
            Lara_Skin_SetGunEquipment(LM_THIGH_L, gun);
            break;
        }
    }
}

static void M_InitialiseCutsceneLevel(void)
{
    Lara_Skin_SetGunEquipment(LM_THIGH_L, Gun_GetDefaultType());
    Lara_Skin_SetGunEquipment(LM_THIGH_R, Gun_GetDefaultType());
}

static void M_InitialiseNormalLevel(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = SG_Resume_GetEntry(level);

    const LARA_GUN_TYPE holster_gun = M_DetermineHolsterGun();
    if (holster_gun != LGT_UNARMED && !Gun_IsFlareType(holster_gun)) {
        Gun_SetLaraHolsterLMesh(holster_gun);
        Gun_SetLaraHolsterRMesh(holster_gun);
        M_EnsureDefaultDualPistolMesh(holster_gun);
    }

    const LARA_GUN_TYPE back_gun = M_DetermineBackGun();
    if (back_gun != LGT_UNARMED) {
        Gun_SetLaraBackMesh(back_gun);
    }

    if (resume != nullptr && Gun_IsFlareType(resume->equipped_gun_type)) {
        Lara_Skin_SetGunEquipment(LM_HAND_L, resume->equipped_gun_type);
    }
}

void Lara_Mesh_Initialise(const GF_LEVEL *const level)
{
    const OBJECT *const skin_obj = Object_Get(O_LARA_SKIN);
    if (skin_obj->loaded) {
        OBJECT *const lara_obj = Object_Get(O_LARA);
        lara_obj->mesh_idx = skin_obj->mesh_idx;
    }

    switch (level->type) {
    // A title has no save to read a holster gun out of, and the scenes it
    // plays behind the menu want her armed as any other cutscene does.
    case GFL_CUTSCENE:
    case GFL_TITLE:
        M_InitialiseCutsceneLevel();
        break;
    default:
        M_InitialiseNormalLevel(level);
        break;
    }
}

void Lara_Mesh_SwapSingle(const LARA_MESH mesh, const OBJECT_ID obj_id)
{
    const OBJECT *const obj = Object_Get(obj_id);
    Lara_Mesh_Set(mesh, Object_GetMesh(obj->mesh_idx + mesh));
}

void Lara_Mesh_SwapAll(const OBJECT_ID obj_id)
{
    if (!Object_Get(obj_id)->loaded) {
        return;
    }

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_Mesh_SwapSingle(mesh, obj_id);
    }
}

void Lara_Mesh_Set(const LARA_MESH mesh, OBJECT_MESH *const mesh_ptr)
{
    m_Meshes[mesh] = mesh_ptr;
}

OBJECT_MESH *Lara_Mesh_Get(const LARA_MESH mesh)
{
    return m_Meshes[mesh];
}

RGBA_F Lara_GetMeshTint(const GAME_VECTOR pos)
{
    if (!g_Config.visuals.enable_responsive_mesh_tint || g_Camera.underwater) {
        return Output_GetTint();
    }

    int16_t room_num = pos.room_num;
    Room_GetSector(pos.pos, &room_num);
    const int32_t water_height = Room_GetWaterHeight(pos.pos, room_num);

    if (!Room_Get(room_num)->flags.underwater) {
        return COLOR_RGBA_F_WHITE;
    } else if (water_height == NO_HEIGHT) {
        return Color_RGBToRGBA(Output_GetWaterColor());
    } else if (pos.y > water_height) {
        return Color_RGBToRGBA(Output_GetWaterColor());
    } else {
        return COLOR_RGBA_F_WHITE;
    }
}

int32_t Lara_GetMeshIndex(const ITEM *const item, const int32_t mesh_idx)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const int32_t fallback = obj->mesh_idx + mesh_idx;

    const OBJECT_MESH *const mesh = Lara_Mesh_Get(mesh_idx);
    if (mesh == nullptr) {
        return fallback;
    }

    const int32_t resolved = Object_GetMeshIndex(mesh);
    if (resolved < 0) {
        return fallback;
    }

    return resolved;
}
