#include <fakes/lara.h>

#include <harness/fake_calls.h>

#include <fakes/rooms.h>

#include <trx/game/const.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/routines.h>
#include <trx/game/gun/types.h>
#include <trx/game/inventory.h>
#include <trx/game/items/manager.h>
#include <trx/game/items/types.h>
#include <trx/game/lara/skin/types.h>

#include <lauxlib.h>
#include <string.h>

static WEAPON_INFO m_Weapons[MAX_WEAPONS] = {
    [LGT_PISTOLS] = { .type = WEAPON_TYPE_DUAL_PISTOLS },
};

static LARA_INFO m_Lara;
static bool m_HolstersVisible;
static int32_t m_SpeechFace;
static bool m_HasPistols;
static LARA_SKIN_TYPE m_Skin;

static struct {
    OBJECT_ID variant;
    OBJECT_ID base;
} m_InvShared[FAKE_INV_SHARED];

static INVENTORY_STATE m_LiveState;
static bool m_CanAdd = true;

static const WEAPON_INFO m_GunTypes[] = {
    { .gun_type = LGT_PISTOLS },
};

static INVENTORY_ENTRY *M_StateEntry(
    INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < state->count; i++) {
        if (state->entries[i].object_id == object_id) {
            return &state->entries[i];
        }
    }
    return nullptr;
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
    m_Lara.wet[LM_HEAD] = 1;
    m_HolstersVisible = true;
    m_SpeechFace = -1;
    m_HasPistols = true;
    m_Skin = 0;
    m_CanAdd = true;
    for (int32_t i = 0; i < FAKE_INV_SHARED; i++) {
        m_InvShared[i] = (typeof(m_InvShared[0])) { NO_OBJECT, NO_OBJECT };
    }
    m_LiveState = (INVENTORY_STATE) {};
}

RESULT Gun_Registry_SetKind(const WEAPON_TYPE type, WEAPON_INFO *const target)
{
    target->type = type;
    return OK;
}

RESULT Gun_Registry_Declare(
    const LARA_GUN_TYPE gun_type, const WEAPON_TYPE type,
    WEAPON_INFO *const target)
{
    FAIL_IF(
        gun_type <= LGT_UNARMED || gun_type >= MAX_WEAPONS,
        "there is no weapon %d", (int32_t)gun_type);
    FAIL_IF(target->is_declared, "already declared");
    target->type = type;
    target->is_declared = true;
    return OK;
}

WEAPON_INFO *Gun_Registry_Get(const LARA_GUN_TYPE gun_type)
{
    m_Weapons[gun_type].gun_type = gun_type;
    return &m_Weapons[gun_type];
}

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &m_Lara;
}

ITEM *Lara_GetItem(void)
{
    return Item_Get(0);
}

OBJECT_ID Lara_GetAnimationObject(void)
{
    return O_LARA;
}

ITEM *Lara_Vehicle_GetItem(void)
{
    return nullptr;
}

bool Lara_IsControllable(void)
{
    return true;
}

int16_t Item_GetRelativeObjAnim(const ITEM *const item, const OBJECT_ID obj_id)
{
    return -1;
}

OBJECT_ID Inv_GetItemOption(const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < FAKE_INV_SHARED; i++) {
        if (m_InvShared[i].variant == object_id) {
            return m_InvShared[i].base;
        }
    }
    return object_id;
}

// clang-format off
OBJECT_ID FakeLara_GunObject(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:      return O_PISTOLS_ITEM;
    case LGT_MAGNUMS:      return O_MAGNUMS_ITEM;
    case LGT_AUTOS:        return O_AUTOS_ITEM;
    case LGT_DESERT_EAGLE: return O_DESERT_EAGLE_ITEM;
    case LGT_UZIS:         return O_UZIS_ITEM;
    case LGT_SHOTGUN:      return O_SHOTGUN_ITEM;
    case LGT_HARPOON:      return O_HARPOON_GUN_ITEM;
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
    case LGT_PISTOLS:      return O_PISTOLS_AMMO_ITEM;
    case LGT_MAGNUMS:      return O_MAGNUMS_AMMO_ITEM;
    case LGT_AUTOS:        return O_AUTOS_AMMO_ITEM;
    case LGT_DESERT_EAGLE: return O_DESERT_EAGLE_AMMO_ITEM;
    case LGT_UZIS:         return O_UZIS_AMMO_ITEM;
    case LGT_SHOTGUN:      return O_SHOTGUN_AMMO_ITEM;
    case LGT_HARPOON:      return O_HARPOON_GUN_AMMO_ITEM;
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

OBJECT_ID Gun_GetWeaponAnim(const LARA_GUN_TYPE gun_type)
{
    return FakeLara_GunObject(gun_type);
}

bool Gun_HasInfiniteAmmo(const LARA_GUN_TYPE gun_type)
{
    return false;
}

int32_t Gun_GetRoundsPerShot(const LARA_GUN_TYPE gun_type)
{
    return gun_type == LGT_SHOTGUN ? 6 : 1;
}

int32_t Gun_GetRoundsPerBox(const LARA_GUN_TYPE gun_type)
{
    return Gun_GetRoundsPerShot(gun_type) * 10;
}

void Inv_AddAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    Inv_SetAmmo(gun_type, Inv_GetAmmo(gun_type) + rounds);
}

int32_t Inv_GetAmmo(const LARA_GUN_TYPE gun_type)
{
    return Inv_State_GetAmmo(&m_LiveState, gun_type);
}

void Inv_SetAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    Inv_State_SetAmmo(&m_LiveState, gun_type, rounds);
}

bool Inv_CanAddItem(const OBJECT_ID object_id)
{
    return m_CanAdd;
}

bool Inv_AddItem(const OBJECT_ID object_id)
{
    FAKE_RECORD("inv_add", FV(object_id));
    if (!m_CanAdd) {
        return false;
    }
    Inv_State_AddCount(&m_LiveState, object_id, 1);
    return true;
}

bool Inv_RemoveItem(const OBJECT_ID object_id)
{
    FAKE_RECORD("inv_remove", FV(object_id));
    const int32_t held = Inv_State_GetCount(&m_LiveState, object_id);
    if (held == 0) {
        return false;
    }
    Inv_State_SetCount(&m_LiveState, object_id, held - 1);
    return true;
}

int32_t Inv_GetItemCount(const OBJECT_ID object_id)
{
    if (object_id == FakeLara_GunObject(LGT_PISTOLS)) {
        return m_HasPistols ? 1 : 0;
    }
    return Inv_State_GetCount(&m_LiveState, object_id);
}

bool Inv_HasItem(const OBJECT_ID object_id)
{
    return Inv_GetItemCount(object_id) > 0;
}

int32_t Inv_GetDrawnEntries(
    INVENTORY_ENTRY *const entries, const int32_t max_count)
{
    return Inv_State_GetDrawnEntries(&m_LiveState, entries, max_count);
}

INVENTORY_STATE *Inv_GetState(void)
{
    return &m_LiveState;
}

OBJECT_ID Inv_GetItemPickup(const OBJECT_ID object_id)
{
    return object_id;
}

int32_t Inv_State_GetAmmo(
    const INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type)
{
    for (int32_t i = 0; i < state->ammo_count; i++) {
        if (state->ammo[i].gun_type == gun_type) {
            return state->ammo[i].rounds;
        }
    }
    return 0;
}

void Inv_State_SetAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const int32_t rounds)
{
    if (gun_type == LGT_UNARMED) {
        return;
    }
    for (int32_t i = 0; i < state->ammo_count; i++) {
        if (state->ammo[i].gun_type == gun_type) {
            state->ammo[i].rounds = rounds;
            return;
        }
    }
    state->ammo[state->ammo_count++] =
        (INVENTORY_AMMO) { .gun_type = gun_type, .rounds = rounds };
}

void Inv_State_ClearAmmo(INVENTORY_STATE *const state)
{
    state->ammo_count = 0;
}

void Inv_State_CopyAmmo(
    INVENTORY_STATE *const dst, const INVENTORY_STATE *const src)
{
    memcpy(dst->ammo, src->ammo, sizeof(dst->ammo));
    dst->ammo_count = src->ammo_count;
}

int32_t Inv_State_GetDrawnEntries(
    const INVENTORY_STATE *const state, INVENTORY_ENTRY *const entries,
    const int32_t max_count)
{
    int32_t count = 0;
    for (int32_t i = 0; i < state->count && count < max_count; i++) {
        entries[count++] = state->entries[i];
    }
    return count;
}

int32_t Gun_Registry_GetCount(void)
{
    return sizeof(m_GunTypes) / sizeof(m_GunTypes[0]);
}

const WEAPON_INFO *Gun_Registry_GetByIndex(const int32_t idx)
{
    return &m_GunTypes[idx];
}

bool Gun_Registry_IsValidType(const LARA_GUN_TYPE gun_type)
{
    return gun_type >= LGT_UNARMED && gun_type < MAX_WEAPONS;
}

LARA_GUN_TYPE Gun_GetType(const OBJECT_ID object_id)
{
    for (LARA_GUN_TYPE gun_type = LGT_UNARMED + 1; gun_type < NUM_WEAPONS;
         gun_type++) {
        if (FakeLara_GunObject(gun_type) == object_id) {
            return gun_type;
        }
    }
    return LGT_UNARMED;
}

void Inv_State_AddCount(
    INVENTORY_STATE *const state, const OBJECT_ID object_id, const int32_t qty)
{
    Inv_State_SetCount(
        state, object_id, Inv_State_GetCount(state, object_id) + qty);
}

void Inv_State_AddAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const int32_t rounds)
{
    Inv_State_SetAmmo(
        state, gun_type, Inv_State_GetAmmo(state, gun_type) + rounds);
}

int32_t Gun_GetInitialRounds(const LARA_GUN_TYPE gun_type)
{
    return Gun_GetRoundsPerBox(gun_type);
}

int32_t Inv_State_GetCount(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    const INVENTORY_ENTRY *const entry =
        M_StateEntry((INVENTORY_STATE *)state, Inv_GetItemOption(object_id));
    return entry == nullptr ? 0 : entry->qty;
}

bool Inv_State_Has(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    return Inv_State_GetCount(state, object_id) > 0;
}

void Inv_State_SetCount(
    INVENTORY_STATE *const state, const OBJECT_ID raw_object_id,
    const int32_t qty)
{
    const OBJECT_ID object_id = Inv_GetItemOption(raw_object_id);
    INVENTORY_ENTRY *const entry = M_StateEntry(state, object_id);
    if (entry != nullptr) {
        entry->qty = qty;
        return;
    }
    if (qty > 0 && state->count < INV_MAX_ENTRIES) {
        state->entries[state->count++] = (INVENTORY_ENTRY) {
            .object_id = object_id,
            .qty = qty,
        };
    }
}

void Inv_SetItemCount(const OBJECT_ID object_id, const int32_t qty)
{
    FAKE_RECORD("inv_set_count", FV(object_id), FV(qty));
    Inv_State_SetCount(&m_LiveState, object_id, qty);
}

LARA_SKIN_TYPE Lara_Skin_GetType(void)
{
    return m_Skin;
}

void Lara_Skin_SetType(const LARA_SKIN_TYPE skin_type)
{
    m_Skin = skin_type;
}

// Lara always has a pose to strike here. What decides it in the game - the
// outfit she wears, the kind of level running - is outside what these tests
// reach.
bool Lara_Pose_IsAvailable(void)
{
    return true;
}

bool Lara_Skin_AreHolstersVisible(void)
{
    return m_HolstersVisible;
}

void Lara_Skin_SetHolstersVisible(const bool visible)
{
    m_HolstersVisible = visible;
}

int32_t Lara_Skin_GetSpeechFace(void)
{
    return m_SpeechFace;
}

void Lara_Skin_SetSpeechFace(const int32_t index)
{
    m_SpeechFace = index;
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

void Lara_Skin_SetMeshOverride(
    const LARA_MESH mesh, OBJECT_MESH *const mesh_ptr)
{
    FAKE_RECORD("set_mesh_override", FV(mesh), FV(mesh_ptr != nullptr));
}

OBJECT_MESH *Lara_Skin_GetMeshOverride(const LARA_MESH mesh)
{
    return nullptr;
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
    Gun_Registry_Get(gun_type)->is_available = available;
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

const GUN_KIND_ROUTINES *Gun_Routines_GetKind(const WEAPON_TYPE type)
{
    static const GUN_KIND_ROUTINES routines = {};
    switch (type) {
    case WEAPON_TYPE_SINGLE_PISTOL:
    case WEAPON_TYPE_DUAL_PISTOLS:
    case WEAPON_TYPE_RIFLE:
    case WEAPON_TYPE_FLARE:
        return &routines;
    default:
        return nullptr;
    }
}

void Gun_Registry_AddSetupHook(void (*const hook)(void))
{
}

const char *Gun_Registry_KeepString(const char *const str)
{
    return str;
}

void Gun_Registry_SetInputRole(
    const LARA_GUN_TYPE gun_type, const INPUT_ROLE role)
{
    Gun_Registry_Get(gun_type)->equip_input_role = role;
}

void (*Gun_Routines_GetFire(const char *const name))(LARA_GUN_TYPE, bool)
{
    return nullptr;
}

GUN_FLASH (*Gun_Routines_GetFlash(const char *const name))(void)
{
    return nullptr;
}

void (*Gun_Routines_GetSound(const char *const name))(bool)
{
    return nullptr;
}

int16_t (*Gun_Routines_GetReadyAnim(const char *const name))(void)
{
    return nullptr;
}

uint8_t (*Gun_Routines_GetSmokeSize(const char *const name))(void)
{
    return nullptr;
}

LARA_GUN_TYPE Lara_Vehicle_GetGunType(void)
{
    return LGT_UNARMED;
}
