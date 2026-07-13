// One Lara, standing still. Her state is a real LARA_INFO, reached through the
// real reflection layer - that is what is under test. The handful of things
// that are not fields of it (her item, her outfit, her holsters) are faked
// here.

#include "fake_engine_lara.h"

#include <trx/game/const.h>
#include <trx/game/gun/types.h>
#include <trx/game/items/manager.h>
#include <trx/game/items/types.h>
#include <trx/game/lara/skin/types.h>

#include <stdbool.h>
#include <string.h>

FAKE_LARA_CALLS g_FakeLaraCalls;

static LARA_INFO m_Lara;
static bool m_HolstersVisible;
static bool m_HasPistols;
static LARA_SKIN_TYPE m_Skin;

// The weapon table the bridge walks to decide whether Lara has a pistol at all.
WEAPON_INFO g_Weapons[NUM_WEAPONS] = {
    [LGT_PISTOLS] = { .type = WEAPON_TYPE_DUAL_PISTOLS },
};

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &m_Lara;
}

// Lara is item 1 in the fake level, so trx.lara.item is a live Item handle out
// of the same pool as any other.
ITEM *Lara_GetItem(void)
{
    return Item_Get(0);
}

int16_t Item_GetRelativeObjAnim(const ITEM *const item, const OBJECT_ID obj_id)
{
    return -1;
}

OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    return 0;
}

bool Inv_RequestItem(const OBJECT_ID obj_id)
{
    return m_HasPistols;
}

LARA_SKIN_TYPE Lara_Skin_GetType(void)
{
    return m_Skin;
}

void Lara_Skin_SetType(const LARA_SKIN_TYPE skin_type)
{
    m_Skin = skin_type;
}

bool Lara_Skin_AreHolstersVisible(void)
{
    return m_HolstersVisible;
}

void Lara_Skin_SetHolstersVisible(const bool visible)
{
    m_HolstersVisible = visible;
}

void Lara_Skin_ClearEquipment(const LARA_MESH mesh)
{
    g_FakeLaraCalls.clear_equipment++;
    g_FakeLaraCalls.last_mesh = mesh;
}

void Lara_Skin_SetExtraEquipment(
    const LARA_MESH mesh, const LARA_SKIN_EXTRA_MESH extra_mesh)
{
    g_FakeLaraCalls.set_equipment++;
    g_FakeLaraCalls.last_mesh = mesh;
    g_FakeLaraCalls.last_extra_mesh = extra_mesh;
}

bool Lara_Skin_IsOutfitAvailable(const LARA_SKIN_TYPE skin_type)
{
    return true;
}

const char *Lara_Skin_GetOutfitName(const LARA_SKIN_TYPE skin_type)
{
    return skin_type == 0 ? "default" : "gold";
}

LARA_SKIN_TYPE Lara_Skin_FindOutfitByName(const char *const name)
{
    if (strcmp(name, "default") == 0) {
        return 0;
    }
    if (strcmp(name, "gold") == 0) {
        return 1;
    }
    return -1;
}

void FakeLara_Reset(void)
{
    memset(&m_Lara, 0, sizeof(m_Lara));

    m_Lara.air = 1800;
    m_Lara.exposure_timer = 600;
    m_Lara.water_status = LWS_ABOVE_WATER;
    m_Lara.gun_status = LGS_ARMLESS;
    m_Lara.gun_type = LGT_UNARMED;
    m_Lara.request_gun_type = LGT_UNARMED;
    m_Lara.hit_direction = -1;

    g_FakeLaraCalls = (FAKE_LARA_CALLS) {};
    m_HolstersVisible = true;
    m_HasPistols = true;
    m_Skin = 0;
}
