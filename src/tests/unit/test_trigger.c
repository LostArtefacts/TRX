// Exercises the real Item_Trigger against the floordata item-activation rules.
// The primitive is linked for real; its leaf dependencies are faked: an item
// and object registry, a recording Item_Activate, and g_TRVersion set per case.
// The matrix pins the behavior M_Handle used to carry inline, across TR1/2/3.

#include "harness.h"

#include <trx/game/const.h>
#include <trx/game/items/utils.h>
#include <trx/game/lua/events.h>
#include <trx/game/objects/types.h>
#include <trx/version.h>

#define M_ITEM 0
#define M_OBJ_PLAIN 0
#define M_OBJ_FUNC 1

// A code-bit mask counted the level-editor way, 1..31.
#define M_MASK(bits) ((int16_t)(bits))
#define M_MASK_ALL ((int16_t)TRIGGER_MASK_ALL)

static ITEM m_Items[4];
static OBJECT m_Objects[4];

static int16_t m_Activated;
static int32_t m_ActivateCount;

static bool m_FuncCalled;
static ITEM_TRIGGER_KIND m_FuncSawKind;
static bool m_FuncReturn;

ITEM *Item_Get(const int16_t num)
{
    return &m_Items[num];
}

const OBJECT *Object_Get(const OBJECT_ID id)
{
    return &m_Objects[id];
}

void Item_Activate(const int16_t item_num, const bool force)
{
    m_Activated = item_num;
    m_ActivateCount++;
}

// Item_Trigger fires on_trigger through LUA_FireEventEx. Record that fire so
// the matrix can check the notification, argument for argument, against the
// real LUA_EVENT_ARG layout.
static int32_t m_NotifyCount;
static ITEM_TRIGGER_KIND m_NotifyKind;
static int32_t m_NotifyMask;
static bool m_NotifyOneShot;

bool Game_IsSettingUpItems(void)
{
    return false;
}

bool LUA_FireEventEx(
    const LUA_EVENT_TYPE ev, const LUA_EVENT_ARG *const args,
    const int32_t arg_count)
{
    if (ev != LUA_EVENT_TRIGGER) {
        return false;
    }
    m_NotifyCount++;
    m_NotifyKind = args[1].value.i32;
    m_NotifyMask = args[2].value.i32;
    m_NotifyOneShot = args[4].value.b;
    return false;
}

// Mirrors the objects that read the trigger (falling_block reads the kind):
// with the old convention this would dereference a null trigger. It must not
// now.
static bool M_RecordingFunc(ITEM *const item, const ITEM_TRIGGER *const trigger)
{
    m_FuncCalled = true;
    m_FuncSawKind = trigger->kind;
    return m_FuncReturn;
}

static ITEM *M_Reset(const int32_t version)
{
    g_TRVersion = version;
    memset(m_Items, 0, sizeof(m_Items));
    memset(m_Objects, 0, sizeof(m_Objects));
    m_Objects[M_OBJ_FUNC].trigger_func = M_RecordingFunc;
    m_Items[M_ITEM].object_id = M_OBJ_PLAIN;
    m_Activated = -1;
    m_ActivateCount = 0;
    m_FuncCalled = false;
    m_FuncSawKind = ITEM_TRIGGER_NORMAL;
    m_FuncReturn = true;
    m_NotifyCount = 0;
    m_NotifyKind = ITEM_TRIGGER_NORMAL;
    m_NotifyMask = 0;
    m_NotifyOneShot = false;
    return &m_Items[M_ITEM];
}

static void M_Fire(const ITEM_TRIGGER trigger)
{
    Item_Trigger(M_ITEM, &trigger);
}

TEST(notify_carries_the_trigger_fundamentals)
{
    M_Reset(1);
    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_SWITCH, .mask = M_MASK(5), .one_shot = true });
    CHECK_EQ_INT(m_NotifyCount, 1);
    CHECK_EQ_INT(m_NotifyKind, ITEM_TRIGGER_SWITCH);
    CHECK_EQ_INT(m_NotifyMask, M_MASK(5));
    CHECK(m_NotifyOneShot);
}

TEST(spent_trigger_does_not_notify)
{
    ITEM *const item = M_Reset(1);
    item->trigger.spent = true;
    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL });
    CHECK_EQ_INT(m_NotifyCount, 0);
}

TEST(forward_full_mask_activates)
{
    ITEM *const item = M_Reset(1);
    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL });
    CHECK_EQ_INT(item->trigger.mask, TRIGGER_MASK_ALL);
    CHECK_EQ_INT(m_ActivateCount, 1);
    CHECK_EQ_INT(m_Activated, M_ITEM);
}

TEST(forward_partial_mask_waits_then_accumulates)
{
    ITEM *const item = M_Reset(1);

    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK(3) });
    CHECK(item->trigger.mask != TRIGGER_MASK_ALL);
    CHECK_EQ_INT(m_ActivateCount, 0);

    // The remaining bits arrive on a second trigger; only now does it run.
    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK(28) });
    CHECK_EQ_INT(item->trigger.mask, TRIGGER_MASK_ALL);
    CHECK_EQ_INT(m_ActivateCount, 1);
}

TEST(timer_seconds_scale_to_frames)
{
    ITEM *const item = M_Reset(1);

    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL, .timer = 5.0f });
    CHECK_EQ_INT(item->timer, 150);
}

TEST(timer_of_one_is_a_single_frame_sentinel)
{
    ITEM *const item = M_Reset(1);
    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL, .timer = 1.0f });
    CHECK_EQ_INT(item->timer, 1);
}

TEST(timer_zero_stays_zero)
{
    ITEM *const item = M_Reset(1);
    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL, .timer = 0.0f });
    CHECK_EQ_INT(item->timer, 0);
}

TEST(timer_fraction_rounds)
{
    ITEM *const item = M_Reset(1);
    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL, .timer = 0.1f });
    CHECK_EQ_INT(item->timer, 3);
}

TEST(switch_xor_toggles_both_ways)
{
    ITEM *const item = M_Reset(1);

    // Off to on: sets the bits and runs.
    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_SWITCH, .mask = M_MASK_ALL });
    CHECK_EQ_INT(item->trigger.mask, TRIGGER_MASK_ALL);
    CHECK_EQ_INT(m_ActivateCount, 1);

    // On to off: clears the bits again and does not re-run.
    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_SWITCH, .mask = M_MASK_ALL });
    CHECK_EQ_INT(item->trigger.mask, 0);
    CHECK_EQ_INT(m_ActivateCount, 1);
}

TEST(antitrigger_tr1_clears_only_its_mask)
{
    ITEM *const item = M_Reset(1);
    item->trigger.mask = TRIGGER_MASK_ALL;
    item->trigger.reversed = true;

    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_ANTI, .mask = M_MASK(3) });
    // Only bits 1 and 2 cleared; the rest and the reverse flag stand.
    CHECK_EQ_INT(item->trigger.mask, TRIGGER_MASK_ALL & ~M_MASK(3));
    CHECK(item->trigger.reversed);
    CHECK_EQ_INT(m_ActivateCount, 0);
}

TEST(antitrigger_tr3_clears_all_bits_and_reverse)
{
    ITEM *const item = M_Reset(3);
    item->trigger.mask = TRIGGER_MASK_ALL;
    item->trigger.reversed = true;

    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_ANTI, .mask = M_MASK(3) });
    CHECK_EQ_INT(item->trigger.mask, 0);
    CHECK(!item->trigger.reversed);
    CHECK_EQ_INT(m_ActivateCount, 0);
}

TEST(one_shot_forward_latches_and_spends)
{
    ITEM *const item = M_Reset(1);

    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL, .one_shot = true });
    CHECK(item->trigger.spent);
    CHECK_EQ_INT(m_ActivateCount, 1);

    // Spent: a second trigger is ignored outright.
    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL, .one_shot = true });
    CHECK_EQ_INT(m_ActivateCount, 1);
}

TEST(one_shot_switch_tr3_latches_in_its_own_bit)
{
    ITEM *const item = M_Reset(3);

    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_SWITCH, .mask = M_MASK_ALL, .one_shot = true });
    CHECK(item->trigger.switch_spent);
    CHECK_EQ_INT(m_ActivateCount, 1);

    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_SWITCH, .mask = M_MASK_ALL, .one_shot = true });
    CHECK_EQ_INT(m_ActivateCount, 1);
}

TEST(one_shot_anti_tr3_latches_in_its_own_bit)
{
    ITEM *const item = M_Reset(3);
    item->trigger.mask = TRIGGER_MASK_ALL;

    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_ANTI, .mask = M_MASK_ALL, .one_shot = true });
    CHECK(item->trigger.anti_spent);
}

TEST(one_shot_anti_tr1_latches_in_the_general_bit)
{
    ITEM *const item = M_Reset(1);
    item->trigger.mask = TRIGGER_MASK_ALL;

    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_ANTI, .mask = M_MASK_ALL, .one_shot = true });
    CHECK(item->trigger.spent);
}

TEST(heavy_switch_spends_on_general_bit_not_the_switch_bit)
{
    // A heavy switch toggles like a switch, but TR3 tracks its spent state in
    // the general one-shot bit, not the switch bit. A stray switch-bit must not
    // stop it.
    ITEM *item = M_Reset(3);
    item->trigger.switch_spent = true;
    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_HEAVY_SWITCH,
                            .mask = M_MASK_ALL });
    CHECK_EQ_INT(item->trigger.mask, TRIGGER_MASK_ALL);
    CHECK_EQ_INT(m_ActivateCount, 1);

    item = M_Reset(3);
    item->trigger.spent = true;
    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_HEAVY_SWITCH,
                            .mask = M_MASK_ALL });
    CHECK_EQ_INT(m_ActivateCount, 0);
}

TEST(trigger_func_veto_stops_default_handling)
{
    ITEM *const item = M_Reset(1);
    item->object_id = M_OBJ_FUNC;
    m_FuncReturn = false;

    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL });
    CHECK(m_FuncCalled);
    CHECK_EQ_INT(item->trigger.mask, 0);
    CHECK_EQ_INT(m_ActivateCount, 0);
}

TEST(trigger_func_sees_the_real_kind)
{
    ITEM *const item = M_Reset(1);
    item->object_id = M_OBJ_FUNC;

    M_Fire((ITEM_TRIGGER) { .kind = ITEM_TRIGGER_HEAVY, .mask = M_MASK_ALL });
    CHECK(m_FuncCalled);
    CHECK_EQ_INT(m_FuncSawKind, ITEM_TRIGGER_HEAVY);
    // A heavy trigger is a forward trigger: default handling still runs.
    CHECK_EQ_INT(m_ActivateCount, 1);
}

TEST(spent_one_shot_skips_the_trigger_func)
{
    ITEM *const item = M_Reset(1);
    item->object_id = M_OBJ_FUNC;
    item->trigger.spent = true;

    M_Fire((ITEM_TRIGGER) {
        .kind = ITEM_TRIGGER_NORMAL, .mask = M_MASK_ALL, .one_shot = true });
    CHECK(!m_FuncCalled);
    CHECK_EQ_INT(m_ActivateCount, 0);
}
