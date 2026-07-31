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
static int32_t m_Ammo[NUM_WEAPONS];

// Which pickups share a backpack entry with another, as the scion's states do.
static struct {
    OBJECT_ID variant;
    OBJECT_ID base;
} m_InvShared[FAKE_INV_SHARED];

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

// The backpack, as a count per object. The engine maps a pickup to the icon it
// goes into before it counts; the fake takes whichever id it is given, so a
// test has to name one of them consistently.
static int32_t m_InvCounts[FAKE_INV_SLOTS];
static OBJECT_ID m_InvObjects[FAKE_INV_SLOTS];
static bool m_CanAdd = true;

// The engine holds one backpack entry per inventory icon, and several pickups
// can share one - the scion in each of its states, a waterskin at each fill
// level. The fake keeps that split, so a test can tell a command that works in
// the wrong id space from one that does not.
OBJECT_ID Inv_GetItemOption(const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < FAKE_INV_SHARED; i++) {
        if (m_InvShared[i].variant == object_id) {
            return m_InvShared[i].base;
        }
    }
    return object_id;
}

static int32_t *M_InvSlot(const OBJECT_ID raw_object_id)
{
    const OBJECT_ID object_id = Inv_GetItemOption(raw_object_id);
    for (int32_t i = 0; i < FAKE_INV_SLOTS; i++) {
        if (m_InvObjects[i] == object_id) {
            return &m_InvCounts[i];
        }
        if (m_InvCounts[i] == 0 && m_InvObjects[i] == NO_OBJECT) {
            m_InvObjects[i] = object_id;
            return &m_InvCounts[i];
        }
    }
    return nullptr;
}

// The same pairing gun/common.c makes, which is what keeps a bridge that hands
// an object id on reachable from a test.
// clang-format off
OBJECT_ID FakeLara_GunObject(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:      return O_PISTOL_ITEM;
    case LGT_MAGNUMS:      return O_MAGNUM_ITEM;
    case LGT_AUTOS:        return O_AUTOS_ITEM;
    case LGT_DESERT_EAGLE: return O_DESERT_EAGLE_ITEM;
    case LGT_UZIS:         return O_UZI_ITEM;
    case LGT_SHOTGUN:      return O_SHOTGUN_ITEM;
    case LGT_HARPOON:      return O_HARPOON_ITEM;
    case LGT_M16:          return O_M16_ITEM;
    case LGT_MP5:          return O_MP5_ITEM;
    case LGT_GRENADE:      return O_GRENADE_GUN_ITEM;
    case LGT_ROCKET:       return O_ROCKET_GUN_ITEM;
    case LGT_CROSSBOW:     return O_CROSSBOW_ITEM;
    case LGT_REVOLVER:     return O_REVOLVER_ITEM;
    default:               return NO_OBJECT;
    }
}

OBJECT_ID FakeLara_AmmoObject(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:      return O_PISTOL_AMMO_ITEM;
    case LGT_MAGNUMS:      return O_MAGNUM_AMMO_ITEM;
    case LGT_AUTOS:        return O_AUTOS_AMMO_ITEM;
    case LGT_DESERT_EAGLE: return O_DESERT_EAGLE_AMMO_ITEM;
    case LGT_UZIS:         return O_UZI_AMMO_ITEM;
    case LGT_SHOTGUN:      return O_SHOTGUN_AMMO_ITEM;
    case LGT_HARPOON:      return O_HARPOON_AMMO_ITEM;
    case LGT_M16:          return O_M16_AMMO_ITEM;
    case LGT_MP5:          return O_MP5_AMMO_ITEM;
    case LGT_GRENADE:      return O_GRENADE_AMMO_ITEM;
    case LGT_ROCKET:       return O_ROCKET_AMMO_ITEM;
    case LGT_CROSSBOW:     return O_CROSSBOW_AMMO_1_ITEM;
    case LGT_REVOLVER:     return O_REVOLVER_AMMO_ITEM;
    default:               return NO_OBJECT;
    }
}
// clang-format on

OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    return FakeLara_GunObject(gun_type);
}

OBJECT_ID Gun_GetAmmoObject(const LARA_GUN_TYPE gun_type)
{
    return FakeLara_AmmoObject(gun_type);
}

int32_t Inv_GetAmmo(const LARA_GUN_TYPE gun_type)
{
    return gun_type == LGT_UNARMED ? 0 : m_Ammo[gun_type];
}

void Inv_SetAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    if (gun_type != LGT_UNARMED) {
        m_Ammo[gun_type] = rounds;
    }
}

bool Inv_CanAddItem(const OBJECT_ID object_id)
{
    return m_CanAdd;
}

bool Inv_AddItem(const OBJECT_ID object_id)
{
    FAKE_RECORD("inv_add", FV(object_id));
    int32_t *const slot = M_InvSlot(object_id);
    if (!m_CanAdd || slot == nullptr) {
        return false;
    }
    (*slot)++;
    return true;
}

bool Inv_RemoveItem(const OBJECT_ID object_id)
{
    FAKE_RECORD("inv_remove", FV(object_id));
    int32_t *const slot = M_InvSlot(object_id);
    if (slot == nullptr || *slot == 0) {
        return false;
    }
    (*slot)--;
    return true;
}

int32_t Inv_RequestItem(const OBJECT_ID object_id)
{
    // Lara's pistols are not in the backpack the fake models; the surface asks
    // for them by way of has_pistol_weapon, which is what m_HasPistols answers.
    if (object_id == FakeLara_GunObject(LGT_PISTOLS)) {
        return m_HasPistols ? 1 : 0;
    }
    const int32_t *const slot = M_InvSlot(object_id);
    return slot == nullptr ? 0 : *slot;
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
    m_CanAdd = true;
    for (int32_t i = 0; i < FAKE_INV_SHARED; i++) {
        m_InvShared[i] = (typeof(m_InvShared[0])) { NO_OBJECT, NO_OBJECT };
    }
    memset(m_Ammo, 0, sizeof(m_Ammo));
    memset(m_InvCounts, 0, sizeof(m_InvCounts));
    for (int32_t i = 0; i < FAKE_INV_SLOTS; i++) {
        m_InvObjects[i] = NO_OBJECT;
    }
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

// The room count a teleport measures a room against, for a test that stands
// Lara up without the rooms she stands in. fakes/rooms.c owns the real one, so
// this is weak: linking both leaves the room fake's answer.
__attribute__((weak)) int32_t Room_GetCount(void)
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

void FakeLara_SetCanAdd(const bool can_add)
{
    m_CanAdd = can_add;
}

void FakeLara_SetWeaponAvailable(
    const LARA_GUN_TYPE gun_type, const bool available)
{
    g_Weapons[gun_type].is_available = available;
}

void FakeLara_ShareInvEntry(const OBJECT_ID variant, const OBJECT_ID base)
{
    for (int32_t i = 0; i < FAKE_INV_SHARED; i++) {
        if (m_InvShared[i].variant == NO_OBJECT) {
            m_InvShared[i] = (typeof(m_InvShared[0])) { variant, base };
            return;
        }
    }
}
