#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/items/anim.h>
#include <trx/game/lara.h>
#include <trx/game/lara/flare.h>
#include <trx/game/lara/misc.h>
#include <trx/game/lara/util.h>
#include <trx/game/objects/general/flare_item.h>
#include <trx/game/rooms/geometry.h>
#include <trx/utils.h>

// clang-format off
#define M_CAM_CRAWL_ELEVATION (-DEG_1 * 23)      // = -4186
#define M_CRAWL_TURN_RATE     ((DEG_1 * 2) + 45) // = 409
#define M_CRAWL_TURN_MAX      (DEG_1 * 3)        // = 546
#define M_CRAWL_TURN_SLOW     273
// clang-format on

static bool M_CanEnterCrawlFromCrouch(const ITEM *const item)
{
    return item->current_anim_state == LS(LS_CROUCH_IDLE)
        && Item_GetRelativeFrame(item) > 1;
}

static bool M_CanCrouchRoll(const ITEM *const item, const LARA_INFO *const lara)
{
    if (!g_Config.gameplay.enable_crouch_roll || g_Input.draw || !g_Input.sprint
        || lara->gun_status != LGS_ARMLESS) {
        return false;
    }

    if (!g_Input.crouch
        && (!lara->keep_crouched || lara->water_status == LWS_WADE)) {
        return false;
    }

    const int16_t height_far = Lara_FloorFront(item, item->rot.y, STEP_L * 2);
    const int16_t height_near = Lara_FloorFront(item, item->rot.y, STEP_L);
    if (height_far >= STEPUP_HEIGHT || height_near < -STEPUP_HEIGHT) {
        return false;
    }

    if (!Item_TestAnimEqual(item, LA(LA_CROUCH_IDLE))
        && !Item_TestAnimEqual(item, LA(LA_STAND_TO_CROUCH_END))) {
        return false;
    }

    if (lara->gun_type == LGT_FLARE
        && (lara->flare.age <= 0 || lara->flare.age >= Flare_GetMaxAge())) {
        return false;
    }

    return true;
}

static void M_CrouchIdle(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->is_crouched = true;

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        return;
    }

    if (Lara_Col_TestSlide(item, coll)) {
        return;
    }

    if (g_Input.look) {
        Lara_Look_UpDown();
    }

    if ((g_Input.forward || g_Input.back) && lara->gun_status == LGS_ARMLESS
        && M_CanEnterCrawlFromCrouch(item)) {
        lara->torso_rot.x = 0;
        lara->torso_rot.y = 0;
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        return;
    }

    if (M_CanCrouchRoll(item, lara)) {
        lara->torso_rot.x = 0;
        lara->torso_rot.y = 0;
        Item_SwitchToAnim(item, LA(LA_CROUCH_ROLL_FORWARD_START), 0);
        item->current_anim_state = LS(LS_CROUCH_ROLL);
        item->goal_anim_state = LS(LS_CROUCH_ROLL);
    }
}

static void M_CrouchRoll(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_elevation = -3640;
    item->goal_anim_state = LS(LS_CROUCH_IDLE);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->is_crouched = true;
}

static void M_CrawlIdle(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_DEATH);
        return;
    }

    if (g_Input.look) {
        Lara_Look_UpDown();
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->torso_rot.x = 0;
    lara->torso_rot.y = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    g_Camera.target_elevation = M_CAM_CRAWL_ELEVATION;

    if (Item_TestAnimEqual(item, LA(LA_CROUCH_TO_CRAWL_START))) {
        lara->gun_status = LGS_HANDS_BUSY;
    }

    if (Lara_Col_TestSlide(item, coll)) {
        return;
    }

    g_Camera.target_elevation = M_CAM_CRAWL_ELEVATION;
}

static void M_CrawlForward(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        return;
    }

    if (g_Input.look) {
        Lara_Look_UpDown();
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->torso_rot.x = 0;
    lara->torso_rot.y = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    g_Camera.target_elevation = M_CAM_CRAWL_ELEVATION;

    if (Lara_Col_TestSlide(item, coll)) {
        return;
    }

    if (!g_Input.forward || (!g_Input.crouch && !lara->keep_crouched)) {
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        return;
    }

    if (g_Input.left) {
        lara->turn_rate -= M_CRAWL_TURN_RATE;
        CLAMPL(lara->turn_rate, -M_CRAWL_TURN_MAX);
    } else if (g_Input.right) {
        lara->turn_rate += M_CRAWL_TURN_RATE;
        CLAMPG(lara->turn_rate, M_CRAWL_TURN_MAX);
    }
}

static void M_CrawlTurn(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->torso_rot.x = 0;
    lara->torso_rot.y = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    g_Camera.target_elevation = M_CAM_CRAWL_ELEVATION;

    if (Lara_Col_TestSlide(item, coll)) {
        return;
    }

    const bool left_turn = item->current_anim_state == LS(LS_CRAWL_TURN_LEFT);
    item->rot.y += left_turn ? -M_CRAWL_TURN_SLOW : M_CRAWL_TURN_SLOW;

    if (!(left_turn ? g_Input.left : g_Input.right)) {
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
    }
}

static void M_CrawlBack(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        return;
    }

    if (g_Input.look) {
        Lara_Look_UpDown();
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->torso_rot.x = 0;
    lara->torso_rot.y = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    g_Camera.target_elevation = M_CAM_CRAWL_ELEVATION;

    if (Lara_Col_TestSlide(item, coll)) {
        return;
    }

    if (g_Input.back) {
        if (g_Input.right) {
            lara->turn_rate -= M_CRAWL_TURN_RATE;
            CLAMPL(lara->turn_rate, -M_CRAWL_TURN_MAX);
        } else if (g_Input.left) {
            lara->turn_rate += M_CRAWL_TURN_RATE;
            CLAMPG(lara->turn_rate, M_CRAWL_TURN_MAX);
        }
    } else {
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
    }
}

// clang-format off
REGISTER_LARA_STATE(LS_CROUCH_IDLE,      M_CrouchIdle)
REGISTER_LARA_STATE(LS_CROUCH_ROLL,      M_CrouchRoll)
REGISTER_LARA_STATE(LS_CRAWL_IDLE,       M_CrawlIdle)
REGISTER_LARA_STATE(LS_CRAWL_FORWARD,    M_CrawlForward)
REGISTER_LARA_STATE(LS_CRAWL_TURN_LEFT,  M_CrawlTurn)
REGISTER_LARA_STATE(LS_CRAWL_TURN_RIGHT, M_CrawlTurn)
REGISTER_LARA_STATE(LS_CRAWL_BACK,       M_CrawlBack)
// clang-format on
