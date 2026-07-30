// One Lara, standing still. Her state is a real LARA_INFO, reached through the
// real reflection layer - that is what is under test. The handful of things
// that are not fields of it (her item, her outfit, her holsters) are faked
// here.

#include <fakes/lara.h>

#include <harness/fake_calls.h>

#include <fakes/rooms.h>

#include <trx/game/const.h>
#include <trx/game/gun/types.h>
#include <trx/game/items/manager.h>
#include <trx/game/items/types.h>
#include <trx/game/lara/skin/types.h>

#include <lauxlib.h>
#include <string.h>

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

// Lara is item 0 in the pool, which a script sees as item 1: Lua indexes items
// from 1. trx.lara.item is a live Item handle out of the same pool as any
// other.
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
    FAKE_RECORD("clear_equipment", FV(mesh));
}

void Lara_Skin_SetExtraEquipment(
    const LARA_MESH mesh, const LARA_SKIN_EXTRA_MESH extra_mesh)
{
    FAKE_RECORD("set_equipment", FV(mesh), FV(extra_mesh));
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

static void M_Reset(void)
{
    memset(&m_Lara, 0, sizeof(m_Lara));

    m_Lara.air = 1800;
    m_Lara.exposure_timer = 600;
    m_Lara.water_status = LWS_ABOVE_WATER;
    m_Lara.gun_status = LGS_ARMLESS;
    m_Lara.gun_type = LGT_UNARMED;
    m_Lara.request_gun_type = LGT_UNARMED;
    m_Lara.hit_direction = -1;
    // One damp mesh, so a test can watch is_wet flip when Lara is dried.
    m_Lara.wet[LM_HEAD] = 1;
    m_HolstersVisible = true;
    m_HasPistols = true;
    m_Skin = 0;
}

FAKE_ON_RESET(M_Reset)

void Lara_Poison_Cure(void)
{
    FAKE_RECORD("cure_poison");
    m_Lara.poison.value = 0;
    m_Lara.poison.target = 0;
}

void Lara_CatchFire(void)
{
    FAKE_RECORD("catch_fire");
    m_Lara.burn = true;
}

void Lara_Extinguish(void)
{
    FAKE_RECORD("extinguish");
    m_Lara.burn = false;
    m_Lara.electric = 0;
}

void Lara_Dry(void)
{
    FAKE_RECORD("dry");
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        m_Lara.wet[mesh] = 0;
    }
}

bool Lara_IsWet(void)
{
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        if (m_Lara.wet[mesh] != 0) {
            return true;
        }
    }
    return false;
}

// Lara lands where she was sent, and nowhere west of the origin: that is how
// the fake says a position has no floor to stand on.
bool Lara_Cheat_Teleport(const XYZ_32 pos, const int16_t room_num)
{
    FAKE_RECORD("teleport", FV(room_num));
    if (pos.x < 0) {
        return false;
    }
    Lara_GetItem()->pos = pos;
    return true;
}

// The room count the teleport argument check measures a room against. The same
// count the room fake has, so the two agree about how big a level is.
int32_t Room_GetCount(void)
{
    return FAKE_ROOM_COUNT;
}

bool Lara_Cheat_EnterFlyMode(void)
{
    m_Lara.water_status = LWS_CHEAT;
    return true;
}

bool Lara_Cheat_ExitFlyMode(void)
{
    m_Lara.water_status = LWS_ABOVE_WATER;
    return true;
}
