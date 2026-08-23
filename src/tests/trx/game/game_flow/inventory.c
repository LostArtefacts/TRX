// The inventory modifiers a level's sequence declares: what Lara is handed on
// arrival, and what the all-secrets reward gives her. The module is linked for
// real; the inventory, the weapons and the pickup counter around it are faked
// to the little each asks of them.
//
// What is pinned here is the arithmetic the statistics screen depends on. The
// maximum a level advertises comes from GF_GetSecretRewardCount, counted off
// the sequence before play; the tally comes from the pickups the reward
// registers as it is applied. The two are computed by different code from the
// same declaration, so a reward is only correct when they agree - and they have
// to agree whether or not Lara already carries the gun, which is what a bonus
// game makes the ordinary case.

#include <harness/harness.h>

#include <trx/config.h>
#include <trx/game/game_flow/inventory.h>
#include <trx/game/gun.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/objects.h>
#include <trx/game/overlay.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>

#include <string.h>

#define M_ROUNDS_PER_BOX 10
#define M_INITIAL_ROUNDS 3

static INVENTORY_STATE m_Inv;
static RESUME_INFO m_Resume;
static int32_t m_PickupCount;
static int32_t m_Ammo[MAX_WEAPONS];

// The gun types the engine has registered, which the code under test
// walks instead of counting weapon slots.
static const WEAPON_INFO m_GunTypes[] = {
    { .gun_type = LGT_PISTOLS },
    { .gun_type = LGT_GRENADE },
};

// The Great Wall's reward, which is the shape the bug was reported against: a
// gun, two boxes of ammunition for it, and a medipack.
static GF_ADD_ITEM_DATA m_GunReward = {
    .object_id = O_GRENADE_GUN_ITEM,
    .inv_type = GF_INV_SECRET,
    .quantity = 1,
};
static GF_ADD_ITEM_DATA m_AmmoReward = {
    .object_id = O_GRENADE_AMMO_ITEM,
    .inv_type = GF_INV_SECRET,
    .quantity = 2,
};
static GF_ADD_ITEM_DATA m_MedipackReward = {
    .object_id = O_SMALL_MEDIPACK_ITEM,
    .inv_type = GF_INV_SECRET,
    .quantity = 1,
};

static GF_SEQUENCE_EVENT m_Events[] = {
    { .type = GFS_ADD_SECRET_REWARD, .data = &m_GunReward },
    { .type = GFS_ADD_SECRET_REWARD, .data = &m_AmmoReward },
    { .type = GFS_ADD_SECRET_REWARD, .data = &m_MedipackReward },
};

static GF_LEVEL m_Level = {
    .num = 0,
    .type = GFL_NORMAL,
    .sequence = { .length = 3, .events = m_Events },
};

static void M_SetUp(void)
{
    g_ConfigStorage = (CONFIG) {};
    m_Inv = (INVENTORY_STATE) {};
    m_Resume = (RESUME_INFO) {};
    m_PickupCount = 0;
    memset(m_Ammo, 0, sizeof(m_Ammo));
    m_Level.sequence.length = 3;
}

// Everything the module reaches for outside itself. The inventory is a set of
// object ids - the reward asks whether Lara has a thing, never how many.

const OBJECT_ID g_GunObjects[] = {
    O_PISTOL_ITEM,
    O_GRENADE_GUN_ITEM,
    NO_OBJECT,
};

bool Object_IsType(const OBJECT_ID object_id, const OBJECT_ID *const test_arr)
{
    for (int32_t i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == object_id) {
            return true;
        }
    }
    return false;
}

int32_t Gun_Registry_GetCount(void)
{
    return sizeof(m_GunTypes) / sizeof(m_GunTypes[0]);
}

const WEAPON_INFO *Gun_Registry_GetByIndex(const int32_t idx)
{
    return &m_GunTypes[idx];
}

LARA_GUN_TYPE Gun_GetDefaultType(void)
{
    return LGT_PISTOLS;
}

OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:
        return O_PISTOL_ITEM;
    case LGT_GRENADE:
        return O_GRENADE_GUN_ITEM;
    default:
        return NO_OBJECT;
    }
}

OBJECT_ID Gun_GetAmmoObject(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:
        return O_PISTOL_AMMO_ITEM;
    case LGT_GRENADE:
        return O_GRENADE_AMMO_ITEM;
    default:
        return NO_OBJECT;
    }
}

int32_t Gun_GetRoundsPerBox(const LARA_GUN_TYPE gun_type)
{
    return M_ROUNDS_PER_BOX;
}

int32_t Gun_GetInitialRounds(const LARA_GUN_TYPE gun_type)
{
    return M_INITIAL_ROUNDS;
}

bool Gun_HasInfiniteAmmo(const LARA_GUN_TYPE gun_type)
{
    return false;
}

bool Inv_State_Has(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < state->count; i++) {
        if (state->entries[i].object_id == object_id) {
            return state->entries[i].qty > 0;
        }
    }
    return false;
}

void Inv_State_SetCount(
    INVENTORY_STATE *const state, const OBJECT_ID object_id, const int32_t qty)
{
    for (int32_t i = 0; i < state->count; i++) {
        if (state->entries[i].object_id == object_id) {
            state->entries[i].qty = qty;
            return;
        }
    }
    state->entries[state->count++] =
        (INVENTORY_ENTRY) { .object_id = object_id, .qty = qty };
}

void Inv_State_AddCount(
    INVENTORY_STATE *const state, const OBJECT_ID object_id, const int32_t qty)
{
    for (int32_t i = 0; i < state->count; i++) {
        if (state->entries[i].object_id == object_id) {
            state->entries[i].qty += qty;
            return;
        }
    }
    state->entries[state->count++] =
        (INVENTORY_ENTRY) { .object_id = object_id, .qty = qty };
}

void Inv_State_AddAmmo(
    INVENTORY_STATE *const state, const LARA_GUN_TYPE gun_type,
    const int32_t rounds)
{
    state->ammo[gun_type] += rounds;
}

bool Inv_HasItem(const OBJECT_ID object_id)
{
    return Inv_State_Has(&m_Inv, object_id);
}

bool Inv_AddItem(const OBJECT_ID object_id)
{
    Inv_State_AddCount(&m_Inv, object_id, 1);
    return true;
}

void Inv_AddAmmo(const LARA_GUN_TYPE gun_type, const int32_t rounds)
{
    m_Ammo[gun_type] += rounds;
}

void Overlay_AddDisplayPickup(const OBJECT_ID object_id)
{
}

RESUME_INFO *SG_Resume_GetEntry(const GF_LEVEL *const level)
{
    return &m_Resume;
}

void Stats_AddPickup(void)
{
    m_PickupCount++;
}

// The maximum the statistics screen shows for a reward, which is the sum of
// what the sequence declares.
TEST(secret_reward_count_sums_the_sequence)
{
    M_SetUp();
    CHECK_EQ_INT(GF_GetSecretRewardCount(&m_Level), 4);
}

// Only the secret rewards count towards it: an item the level hands Lara on
// arrival is not something she picks up.
TEST(secret_reward_count_ignores_regular_items)
{
    M_SetUp();
    m_GunReward.inv_type = GF_INV_REGULAR;
    CHECK_EQ_INT(GF_GetSecretRewardCount(&m_Level), 3);
    m_GunReward.inv_type = GF_INV_SECRET;
}

TEST(secret_reward_registers_every_declared_pickup)
{
    M_SetUp();
    GF_InventoryModifier_Scan(&m_Level);
    GF_InventoryModifier_Apply(&m_Level, GF_INV_SECRET);

    CHECK_EQ_INT(m_PickupCount, GF_GetSecretRewardCount(&m_Level));
    CHECK(Inv_HasItem(O_GRENADE_GUN_ITEM));
    CHECK_EQ_INT(m_Ammo[LGT_GRENADE], 2 * M_ROUNDS_PER_BOX);
}

// The bonus game hands Lara every weapon before she reaches the level, so the
// reward finds the launcher already in the backpack and gives ammunition in its
// place. It still stands for a pickup, or the level can never be completed to
// its advertised total (#4966).
TEST(secret_reward_registers_a_gun_lara_already_carries)
{
    M_SetUp();
    Inv_State_SetCount(&m_Inv, O_GRENADE_GUN_ITEM, 1);

    GF_InventoryModifier_Scan(&m_Level);
    GF_InventoryModifier_Apply(&m_Level, GF_INV_SECRET);

    CHECK_EQ_INT(m_PickupCount, GF_GetSecretRewardCount(&m_Level));
    CHECK_EQ_INT(m_Ammo[LGT_GRENADE], 2 * M_ROUNDS_PER_BOX + M_INITIAL_ROUNDS);
}

// A reward naming a gun and no ammunition for it, which is where the gun's own
// entry is the only thing there is to count.
TEST(secret_reward_registers_a_lone_gun_either_way)
{
    M_SetUp();
    m_Level.sequence.length = 1;

    GF_InventoryModifier_Scan(&m_Level);
    GF_InventoryModifier_Apply(&m_Level, GF_INV_SECRET);
    CHECK_EQ_INT(m_PickupCount, 1);

    M_SetUp();
    m_Level.sequence.length = 1;
    Inv_State_SetCount(&m_Inv, O_GRENADE_GUN_ITEM, 1);

    GF_InventoryModifier_Scan(&m_Level);
    GF_InventoryModifier_Apply(&m_Level, GF_INV_SECRET);
    CHECK_EQ_INT(m_PickupCount, 1);
}

// A pistols-only run takes the reward's gun and its ammunition away, and what
// is never handed over registers nothing. The maximum is the level scan's to
// adjust, and out of reach from here.
TEST(secret_reward_skips_guns_the_player_disabled)
{
    M_SetUp();
    g_ConfigStorage.gameplay.disable_extra_guns = true;

    GF_InventoryModifier_Scan(&m_Level);
    GF_InventoryModifier_Apply(&m_Level, GF_INV_SECRET);

    CHECK_EQ_INT(m_PickupCount, 1);
    CHECK(!Inv_HasItem(O_GRENADE_GUN_ITEM));
}
