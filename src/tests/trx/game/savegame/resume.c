// The resume info: one entry per level, holding what Lara arrives with and
// what the level recorded. The module is linked for real, along with the
// savegame tables it reads its item names from; the game around it is faked to
// four levels - a gym, two normal levels and the CURRENT one a save mirrors
// into - plus a demo, which is the case worth having because every demo shares
// a single entry past the end of the main table.
//
// What is pinned here is what a playthrough depends on: an entry belongs to its
// level, a carry hands the loadout on without forgetting what the next level
// has already done, and a reset leaves no previous level behind. The wiring
// that decides when these are called is the game flow's, and out of reach from
// here.

#include <harness/harness.h>

#include <trx/config.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/rules.h>
#include <trx/game/savegame.h>
#include <trx/version.h>

#include <string.h>

#define M_GYM 0
#define M_FIRST 1
#define M_SECOND 2
#define M_CURRENT 3
#define M_PISTOL_ROUNDS 50

static WEAPON_INFO m_Weapons[MAX_WEAPONS] = {};

static GF_LEVEL m_MainLevels[] = {
    { .num = M_GYM, .type = GFL_GYM },
    { .num = M_FIRST, .type = GFL_NORMAL },
    { .num = M_SECOND, .type = GFL_NORMAL },
    { .num = M_CURRENT, .type = GFL_CURRENT },
};
static GF_LEVEL m_DemoLevels[] = {
    { .num = 0, .type = GFL_DEMO },
};
static GF_LEVEL m_TitleLevel = { .num = 0, .type = GFL_TITLE };

static const GF_LEVEL_TABLE m_Tables[GFLT_NUMBER_OF] = {
    [GFLT_MAIN] = { .count = 4, .levels = m_MainLevels },
    [GFLT_DEMOS] = { .count = 1, .levels = m_DemoLevels },
};

static INVENTORY_STATE m_LiveInv;
static LARA_INFO m_Lara;
static ITEM m_LaraItem;
static bool m_BonusFlag;

static void M_SetUp(void)
{
    g_ConfigStorage = (CONFIG) {};
    g_ConfigStorage.gameplay.start_lara_hitpoints = 1000;
    g_TRVersion = 1;
    m_BonusFlag = false;
    m_LiveInv = (INVENTORY_STATE) {};
    m_Lara = (LARA_INFO) {};
    m_LaraItem = (ITEM) {};

    SG_Resume_Shutdown();
    SG_Resume_Init();
}

int32_t g_TRVersion = 1;
WEAPON_INFO *Gun_Registry_Get(const LARA_GUN_TYPE gun_type)
{
    // The real registry stamps a weapon with its own type as it
    // seeds them, which nothing here does.
    m_Weapons[gun_type].gun_type = gun_type;
    return &m_Weapons[gun_type];
}

const GF_LEVEL_TABLE *GF_GetLevelTable(const GF_LEVEL_TABLE_TYPE type)
{
    return &m_Tables[type];
}

GF_LEVEL_TABLE_TYPE GF_GetLevelTableType(const GF_LEVEL_TYPE level_type)
{
    switch (level_type) {
    case GFL_DEMO:
        return GFLT_DEMOS;
    case GFL_TITLE:
        return GFLT_TITLE;
    case GFL_CUTSCENE:
        return GFLT_CUTSCENES;
    default:
        return GFLT_MAIN;
    }
}

const GF_LEVEL *GF_GetGymLevel(void)
{
    return &m_MainLevels[M_GYM];
}

const GF_LEVEL *GF_GetFirstLevel(void)
{
    return &m_MainLevels[M_FIRST];
}

bool Game_IsBonusFlagSet(const GAME_BONUS_FLAG flag)
{
    return m_BonusFlag;
}

INVENTORY_STATE *Inv_GetState(void)
{
    return &m_LiveInv;
}

// The entries are addressed by object id alone, which is all the resume info
// asks of them - no rings, no ammunition boxes, no options.
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

int32_t Inv_State_GetCount(
    const INVENTORY_STATE *const state, const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < state->count; i++) {
        if (state->entries[i].object_id == object_id) {
            return state->entries[i].qty;
        }
    }
    return 0;
}

int32_t Inv_GetItemCount(const OBJECT_ID object_id)
{
    return Inv_State_GetCount(&m_LiveInv, object_id);
}

bool Inv_HasItem(const OBJECT_ID object_id)
{
    return Inv_GetItemCount(object_id) > 0;
}

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &m_Lara;
}

ITEM *Lara_GetItem(void)
{
    return &m_LaraItem;
}

OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:
        return O_PISTOL_ITEM;
    case LGT_SHOTGUN:
        return O_SHOTGUN_ITEM;
    case LGT_UZIS:
        return O_UZI_ITEM;
    default:
        return NO_OBJECT;
    }
}

int32_t Gun_GetInitialRounds(const LARA_GUN_TYPE gun_type)
{
    return gun_type == LGT_PISTOLS ? M_PISTOL_ROUNDS : 0;
}

bool Gun_IsRifleType(const LARA_GUN_TYPE gun_type)
{
    return gun_type == LGT_SHOTGUN;
}

LARA_GUN_TYPE Gun_GetHolsterChoice(const INVENTORY_STATE *const inv)
{
    return LGT_PISTOLS;
}

LARA_GUN_TYPE Gun_GetBackChoice(const INVENTORY_STATE *const inv)
{
    return LGT_UNARMED;
}

void Rules_Reset(void)
{
}

void CutSeq_SetPlayedMask(const uint64_t mask)
{
}

const char *EnumMap_ToString(const char *const enum_name, const int32_t value)
{
    return "?";
}

TEST(an_entry_belongs_to_its_level_and_the_demos_share_one)
{
    M_SetUp();
    RESUME_INFO *const first = SG_Resume_GetEntry(&m_MainLevels[M_FIRST]);
    RESUME_INFO *const second = SG_Resume_GetEntry(&m_MainLevels[M_SECOND]);
    CHECK_NOT_NULL(first);
    CHECK(first != second);

    // Past the end of the main table, so no level of the playthrough shares it.
    RESUME_INFO *const demo = SG_Resume_GetEntry(&m_DemoLevels[0]);
    CHECK_NOT_NULL(demo);
    CHECK(demo != first);
    CHECK(demo != second);

    // A title screen has nothing to resume.
    CHECK_NULL(SG_Resume_GetEntry(&m_TitleLevel));
    CHECK_NULL(SG_Resume_GetEntry(nullptr));
}

TEST(a_demo_arrives_armed_however_the_playthrough_stands)
{
    M_SetUp();
    const RESUME_INFO *const demo = SG_Resume_GetEntry(&m_DemoLevels[0]);
    CHECK(demo->flags.available);
    CHECK_EQ_INT(Inv_State_GetCount(&demo->inv, O_PISTOL_ITEM), 1);
    CHECK_EQ_INT(demo->inv.ammo[LGT_PISTOLS], M_PISTOL_ROUNDS);
    CHECK_EQ_INT(demo->prev_level, -1);
}

TEST(a_reset_entry_remembers_no_previous_level)
{
    M_SetUp();
    RESUME_INFO *const entry = SG_Resume_GetEntry(&m_MainLevels[M_SECOND]);
    entry->prev_level = M_FIRST;
    entry->lara_hitpoints = 12;
    entry->level_completed = true;
    entry->stats.counts[STATS_CAT_KILLS] = 7;

    SG_Resume_ResetEntry(&m_MainLevels[M_SECOND]);

    // -1 rather than 0, which is a level number the gym answers to.
    CHECK_EQ_INT(entry->prev_level, -1);
    CHECK_EQ_INT(entry->lara_hitpoints, 0);
    CHECK(!entry->level_completed);
    CHECK_EQ_INT(entry->stats.counts[STATS_CAT_KILLS], 0);
}

TEST(carrying_an_entry_hands_on_the_loadout_and_names_where_it_came_from)
{
    M_SetUp();
    RESUME_INFO *const src = SG_Resume_GetEntry(&m_MainLevels[M_FIRST]);
    RESUME_INFO *const dst = SG_Resume_GetEntry(&m_MainLevels[M_SECOND]);
    src->lara_hitpoints = 640;
    src->equipped_gun_type = LGT_SHOTGUN;
    Inv_State_SetCount(&src->inv, O_SHOTGUN_ITEM, 1);

    SG_Resume_CarryEntry(&m_MainLevels[M_FIRST], &m_MainLevels[M_SECOND]);

    CHECK_EQ_INT(dst->lara_hitpoints, 640);
    CHECK_EQ_INT(dst->equipped_gun_type, LGT_SHOTGUN);
    CHECK_EQ_INT(Inv_State_GetCount(&dst->inv, O_SHOTGUN_ITEM), 1);
    CHECK_EQ_INT(dst->prev_level, M_FIRST);
}

TEST(carrying_an_entry_keeps_what_the_next_level_has_already_done)
{
    M_SetUp();
    RESUME_INFO *const dst = SG_Resume_GetEntry(&m_MainLevels[M_SECOND]);
    dst->level_completed = true;

    SG_Resume_CarryEntry(&m_MainLevels[M_FIRST], &m_MainLevels[M_SECOND]);

    // The source has not been completed, and playing it again is no reason to
    // forget that the level after it has.
    CHECK(dst->level_completed);
}

TEST(a_fresh_playthrough_offers_the_gym_and_the_first_level_alone)
{
    M_SetUp();
    SG_Resume_GetEntry(&m_MainLevels[M_SECOND])->level_completed = true;

    SG_Resume_ResetAllEntries();

    CHECK(SG_Resume_GetEntry(&m_MainLevels[M_GYM])->flags.available);
    CHECK(SG_Resume_GetEntry(&m_MainLevels[M_FIRST])->flags.available);
    CHECK(!SG_Resume_GetEntry(&m_MainLevels[M_SECOND])->flags.available);
    CHECK(!SG_Resume_GetEntry(&m_MainLevels[M_SECOND])->level_completed);
    CHECK_EQ_INT(SG_Resume_CountCompletedLevels(), 0);
}

TEST(the_first_level_starts_with_her_pistols_and_nothing_else)
{
    M_SetUp();
    RESUME_INFO *const entry = SG_Resume_GetEntry(&m_MainLevels[M_FIRST]);
    Inv_State_SetCount(&entry->inv, O_SHOTGUN_ITEM, 1);

    SG_Resume_ApplyRulesToEntry(&m_MainLevels[M_FIRST]);

    CHECK_EQ_INT(Inv_State_GetCount(&entry->inv, O_PISTOL_ITEM), 1);
    CHECK_EQ_INT(entry->inv.ammo[LGT_PISTOLS], M_PISTOL_ROUNDS);
    CHECK_EQ_INT(Inv_State_GetCount(&entry->inv, O_SHOTGUN_ITEM), 0);
    CHECK_EQ_INT(entry->equipped_gun_type, LGT_PISTOLS);
    CHECK_EQ_INT(entry->lara_hitpoints, 1000);
}

TEST(the_gym_is_a_house_tour_so_she_arrives_with_nothing)
{
    M_SetUp();
    RESUME_INFO *const entry = SG_Resume_GetEntry(&m_MainLevels[M_GYM]);
    Inv_State_SetCount(&entry->inv, O_PISTOL_ITEM, 1);

    SG_Resume_ApplyRulesToEntry(&m_MainLevels[M_GYM]);

    CHECK_EQ_INT(Inv_State_GetCount(&entry->inv, O_PISTOL_ITEM), 0);
    CHECK_EQ_INT(entry->equipped_gun_type, LGT_UNARMED);
    CHECK_EQ_INT(entry->gun_status, LGS_ARMLESS);
    CHECK(entry->flags.costume);
}

TEST(healing_between_levels_spares_the_level_she_is_arriving_in)
{
    M_SetUp();
    g_ConfigStorage.gameplay.disable_healing_between_levels = true;
    RESUME_INFO *const entry = SG_Resume_GetEntry(&m_MainLevels[M_SECOND]);
    entry->lara_hitpoints = 320;

    SG_Resume_ApplyRulesToEntry(&m_MainLevels[M_SECOND]);
    CHECK_EQ_INT(entry->lara_hitpoints, 320);

    // The first level is where a new game begins, so it is filled up no matter
    // what the setting says.
    SG_Resume_ApplyRulesToEntry(&m_MainLevels[M_FIRST]);
    CHECK_EQ_INT(
        SG_Resume_GetEntry(&m_MainLevels[M_FIRST])->lara_hitpoints, 1000);
}

TEST(storing_the_game_takes_what_lara_is_carrying_into_the_entry)
{
    M_SetUp();
    m_LaraItem.hit_points = 480;
    m_Lara.last_gun_type = LGT_UZIS;
    m_Lara.holsters_gun_type = LGT_UZIS;
    m_Lara.gun_status = LGS_READY;
    Inv_State_SetCount(&m_LiveInv, O_UZI_ITEM, 1);
    Inv_State_SetCount(&m_LiveInv, O_SMALL_MEDIPACK_ITEM, 3);
    m_LiveInv.ammo[LGT_UZIS] = 150;

    SG_Resume_StoreGameToEntry(&m_MainLevels[M_SECOND]);

    const RESUME_INFO *const entry =
        SG_Resume_GetEntry(&m_MainLevels[M_SECOND]);
    CHECK(entry->flags.available);
    CHECK_EQ_INT(entry->lara_hitpoints, 480);
    CHECK_EQ_INT(entry->equipped_gun_type, LGT_UZIS);
    CHECK_EQ_INT(entry->gun_status, LGS_READY);
    CHECK_EQ_INT(Inv_State_GetCount(&entry->inv, O_UZI_ITEM), 1);
    CHECK_EQ_INT(Inv_State_GetCount(&entry->inv, O_SMALL_MEDIPACK_ITEM), 3);
    CHECK_EQ_INT(entry->inv.ammo[LGT_UZIS], 150);
}

TEST(mirroring_puts_the_level_into_the_entry_a_save_writes_from)
{
    M_SetUp();
    SG_Resume_GetEntry(&m_MainLevels[M_SECOND])->lara_hitpoints = 250;

    SG_Resume_MirrorCurrentEntry(&m_MainLevels[M_SECOND]);

    CHECK_EQ_INT(
        SG_Resume_GetEntry(&m_MainLevels[M_CURRENT])->lara_hitpoints, 250);
}

TEST(completed_levels_are_counted)
{
    M_SetUp();
    CHECK_EQ_INT(SG_Resume_CountCompletedLevels(), 0);

    SG_Resume_GetEntry(&m_MainLevels[M_GYM])->level_completed = true;
    SG_Resume_GetEntry(&m_MainLevels[M_SECOND])->level_completed = true;
    CHECK_EQ_INT(SG_Resume_CountCompletedLevels(), 2);
}
