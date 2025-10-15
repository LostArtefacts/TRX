#include "game/lara/control.h"

#include "config.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/gun.h"
#include "game/gym.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/level/settings.h"
#include "game/music.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/stats.h"

// clang-format off
#define M_MAX_COLL_ROOMS    20
#define M_COLL_DIST         CREATURE_TARGET_DIST // = 4096
#define M_MOVE_TIMEOUT      90
#define M_UW_DAMAGE         5
#define M_DIVE_TILT_MED     (45 * DEG_1)         // = 8190
#define M_DIVE_TILT_MAX     (85 * DEG_1)         // = 15470
#define M_DIVE_TILT_MAX_ALT (100 * DEG_1)        // = 18200
#define M_RADIUS_SURF       LARA_RADIUS          // = 100
#define M_RADIUS_UW         300
#define M_WADE_DEPTH        384
#define M_SWIM_DEPTH        730
#define M_LEAN_UNDO_SURF    (LARA_LEAN_UNDO * 2) // = 364
#define M_LEAN_UNDO_UW      M_LEAN_UNDO_SURF     // = 364
#define M_LEAN_MAX_UW       (LARA_LEAN_MAX * 2)  // = 4004
// clang-format on

static int32_t m_OpenDoorsCheatCooldown = 0;

#if TR_VERSION >= 2
extern bool Skidoo_Control(void);
#endif

static SECTOR *M_GetCurrentSector(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    int16_t room_num = lara_item->room_num;
    return Room_GetSector(
        lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z, &room_num);
}

static void M_Cheat(void)
{
    if (!g_Config.gameplay.enable_cheats) {
        return;
    }

    if (g_InputDB.level_skip_cheat) {
        Lara_Cheat_EndLevel();
    }

    if (g_InputDB.item_cheat) {
        Lara_Cheat_GiveAllItems();
    }

    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->water_status != LWS_CHEAT && g_InputDB.fly_cheat) {
        Lara_Cheat_EnterFlyMode();
    }
}

static void M_WaterCurrent(COLL_INFO *const coll)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    int16_t room_num = item->room_num;
    const ROOM *const room = Room_Get(item->room_num);
    item->box_num = Room_GetWorldSector(room, item->pos.x, item->pos.z)->box;

    XYZ_32 target;
    if (Box_CalculateTarget(&target, item, &lara_info->lot) == TARGET_NONE) {
        return;
    }

#define L_SHIFT(_axis)                                                         \
    do {                                                                       \
        target._axis -= item->pos._axis;                                       \
        if (target._axis > lara_info->current_active) {                        \
            item->pos._axis += lara_info->current_active;                      \
        } else if (target._axis < -lara_info->current_active) {                \
            item->pos._axis -= lara_info->current_active;                      \
        } else {                                                               \
            item->pos._axis += target._axis;                                   \
        }                                                                      \
    } while (0)

    L_SHIFT(x);
    L_SHIFT(y);
    L_SHIFT(z);
#undef L_SHIFT

    lara_info->current_active = 0;
    coll->facing =
        Math_Atan(item->pos.z - coll->old.z, item->pos.x - coll->old.x);
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + LARA_HEIGHT_UW / 2, item->pos.z,
        room_num, LARA_HEIGHT_UW);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->rot.x > 35 * DEG_1) {
            item->rot.x += LARA_UW_WALL_DEFLECT;
        } else if (item->rot.x < -35 * DEG_1) {
            item->rot.x -= LARA_UW_WALL_DEFLECT;
        } else {
            item->fall_speed = 0;
        }
        break;

    case COLL_TOP:
        item->rot.x -= LARA_UW_WALL_DEFLECT;
        break;

    case COLL_TOP_FRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->rot.y += 5 * DEG_1;
        break;

    case COLL_RIGHT:
        item->rot.y -= 5 * DEG_1;
        break;

    default:
        break;
    }

    if (coll->side_mid.floor < 0) {
        item->pos.y += coll->side_mid.floor;
        item->rot.x += LARA_UW_WALL_DEFLECT;
    }
    Lara_Col_Shift(coll);

    coll->old = item->pos;
}

static void M_ObjectCollision(COLL_INFO *const coll)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->hit_direction = -1;
    lara_item->hit_status = false;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t nearby_rooms[M_MAX_COLL_ROOMS];
    const int32_t room_count = Room_GetAdjoiningRooms(
        lara_item->room_num, nearby_rooms, M_MAX_COLL_ROOMS);

    for (int32_t i = 0; i < room_count; i++) {
        int16_t item_num = Room_Get(nearby_rooms[i])->item_num;
        while (item_num != NO_ITEM) {
            const ITEM *const item = Item_Get(item_num);
            // The collision routine can destroy the item - need to store the
            // next item beforehand.
            const int16_t next_item_num = item->next_item;

            if (lara_info->water_status == LWS_CHEAT
                && !Object_IsType(item->object_id, g_PickupObjects)
                && !Object_IsType(item->object_id, g_SwitchObjects)) {
                goto loop_end;
            }
            if (!item->collidable || item->status == IS_INVISIBLE) {
                goto loop_end;
            }

            const OBJECT *const obj = Object_Get(item->object_id);
            if (obj->collision_func == nullptr
                || !Item_IsNearby(lara_item, item, M_COLL_DIST)) {
                goto loop_end;
            }

            obj->collision_func(item_num, lara_item, coll);

        loop_end:
            item_num = next_item_num;
        }
    }

    if (lara_info->hit_effect_count != 0 && lara_info->hit_effect != nullptr
        && coll->enable_hit) {
        const int32_t dx = lara_info->hit_effect->pos.x - lara_item->pos.x;
        const int32_t dz = lara_info->hit_effect->pos.z - lara_item->pos.z;
        Lara_TakeHit(lara_item, dx, dz);
        lara_info->hit_effect_count--;
    }

    if (lara_info->hit_direction == -1) {
        lara_info->hit_frame = 0;
    }
}

static void M_UpdateEnvironment(void)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
#if TR_VERSION == 1
    const bool wading_enabled = g_Config.gameplay.enable_wading;
#else
    const bool wading_enabled = true;
    if (Lara_Vehicle_IsMounted() || lara_info->extra_anim) {
        return;
    }
#endif

    const ROOM *const room = Room_Get(item->room_num);
    const bool room_submerged = (room->flags & RF_UNDERWATER) != 0;
    const int32_t water_depth = Lara_GetWaterDepth(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    const int32_t water_height_diff =
        water_height == NO_HEIGHT ? NO_HEIGHT : item->pos.y - water_height;
    lara_info->water_surface_dist = -water_height_diff;

    // Create splash if Lara lands in wading height water. TR3+ feature.
    if (wading_enabled) {
        const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
        if (item->pos.y + bounds->min.y <= water_height
            && item->pos.y + bounds->max.y >= water_height
            && item->fall_speed > 0 && water_depth < M_SWIM_DEPTH - STEP_L) {
            Spawn_Splash(item);
        }
    }

    switch (lara_info->water_status) {
    case LWS_ABOVE_WATER: {
        if (wading_enabled
            && (water_height_diff == NO_HEIGHT
                || water_height_diff < M_WADE_DEPTH)) {
            break;
        }

        if (wading_enabled && water_depth <= M_SWIM_DEPTH - STEP_L) {
            if (water_height_diff > M_WADE_DEPTH) {
                lara_info->water_status = LWS_WADE;
                if (!item->gravity) {
                    item->goal_anim_state = LS(LS_STOP);
                }
            }
        } else if (room_submerged) {
            lara_info->air = LARA_MAX_AIR;
            lara_info->water_status = LWS_UNDERWATER;
            item->gravity = false;
            item->pos.y += 100;
            Lara_UpdateRoomToHeight(0);
            Sound_StopEffect(SFX_LARA_FALL);
            if (item->current_anim_state == LS(LS_SWAN_DIVE)) {
                item->rot.x = -M_DIVE_TILT_MED;
                item->goal_anim_state = LS(LS_DIVE);
                Lara_Animate(item);
                item->fall_speed *= 2;
            } else if (item->current_anim_state == LS(LS_FAST_DIVE)) {
                item->rot.x = -M_DIVE_TILT_MAX;
                item->goal_anim_state = LS(LS_DIVE);
                Lara_Animate(item);
                item->fall_speed *= 2;
            } else {
                item->rot.x = -M_DIVE_TILT_MED;
                Item_SwitchToAnim(item, LA(LA_FREEFALL_TO_UNDERWATER), 0);
                item->current_anim_state = LS(LS_DIVE);
                item->goal_anim_state = LS(LS_SWIM);
                item->fall_speed = (item->fall_speed * 3) / 2;
            }
            lara_info->head_rot.x = 0;
            lara_info->head_rot.y = 0;
            lara_info->torso_rot.x = 0;
            lara_info->torso_rot.y = 0;
            Spawn_Splash(item);
        }

        break;
    }

    case LWS_UNDERWATER: {
        if (room_submerged) {
            break;
        }

        if (water_depth == NO_HEIGHT || ABS(water_height_diff) >= STEP_L) {
            lara_info->water_status = LWS_ABOVE_WATER;
            Item_SwitchToAnim(item, LA(LA_FALL_START), 0);
            item->current_anim_state = LS(LS_JUMP_FORWARD);
            item->goal_anim_state = LS(LS_JUMP_FORWARD);
            item->gravity = true;
            item->speed = item->fall_speed / 4;
            item->fall_speed = 0;
            item->rot.x = 0;
            item->rot.z = 0;
            lara_info->head_rot.x = 0;
            lara_info->head_rot.y = 0;
            lara_info->torso_rot.x = 0;
            lara_info->torso_rot.y = 0;
            if (TR_VERSION == 1) {
                lara_info->gun_status = LGS_ARMLESS;
            }
        } else {
            lara_info->water_status = LWS_SURFACE;
            Item_SwitchToAnim(item, LA(LA_UNDERWATER_TO_ONWATER), 0);
            item->current_anim_state = LS(LS_SURF_TREAD);
            item->goal_anim_state = LS(LS_SURF_TREAD);
            item->fall_speed = 0;
            item->pos.y += 1 - water_height_diff;
            item->rot.x = 0;
            item->rot.z = 0;
            lara_info->dive_timer = LARA_DIVE_WAIT + 1;
            lara_info->head_rot.x = 0;
            lara_info->head_rot.y = 0;
            lara_info->torso_rot.x = 0;
            lara_info->torso_rot.y = 0;
            Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
            Sound_Effect(SFX_LARA_BREATH, &item->pos, SPM_ALWAYS);
        }
        break;
    }

    case LWS_SURFACE: {
        if (room_submerged) {
            break;
        }

        if (wading_enabled && water_height_diff > M_WADE_DEPTH) {
            lara_info->water_status = LWS_WADE;
            Item_SwitchToAnim(item, LA(LA_STAND_IDLE), 0);
            item->current_anim_state = LS(LS_STOP);
            item->goal_anim_state = LS(LS_WADE);
            Item_Animate(item);
            item->fall_speed = 0;
        } else {
            lara_info->water_status = LWS_ABOVE_WATER;
            Item_SwitchToAnim(item, LA(LA_FALL_START), 0);
            item->current_anim_state = LS(LS_JUMP_FORWARD);
            item->goal_anim_state = LS(LS_JUMP_FORWARD);
            item->gravity = true;
            item->speed = item->fall_speed / 4;
            if (TR_VERSION == 1) {
                lara_info->gun_status = LGS_ARMLESS;
            }
        }
        item->rot.x = 0;
        item->rot.z = 0;
        lara_info->head_rot.x = 0;
        lara_info->head_rot.y = 0;
        lara_info->torso_rot.x = 0;
        lara_info->torso_rot.y = 0;
        break;
    }

    case LWS_WADE: {
        g_Camera.target_elevation = CAM_WADE_ELEVATION;

        if (water_height_diff < M_WADE_DEPTH) {
            lara_info->water_status = LWS_ABOVE_WATER;
            if (item->current_anim_state == LS(LS_WADE)) {
                item->goal_anim_state = LS(LS_RUN);
            }
        } else if (water_height_diff > M_SWIM_DEPTH) {
            lara_info->water_status = LWS_SURFACE;
            item->pos.y += 1 - water_height_diff;

            LARA_ANIMATION anim_idx;
            switch (LS_U(item->current_anim_state)) {
            case LS_WALK_BACK:
                item->goal_anim_state = LS(LS_SURF_BACK);
                anim_idx = LA(LA_ONWATER_IDLE_TO_SWIM_BACK);
                break;

            case LS_STEP_RIGHT:
                item->goal_anim_state = LS(LS_SURF_RIGHT);
                anim_idx = LA(LA_ONWATER_SWIM_RIGHT);
                break;

            case LS_STEP_LEFT:
                item->goal_anim_state = LS(LS_SURF_LEFT);
                anim_idx = LA(LA_ONWATER_SWIM_LEFT);
                break;

            default:
                item->goal_anim_state = LS(LS_SURF_SWIM);
                anim_idx = LA(LA_ONWATER_SWIM_FORWARD);
                break;
            }

            item->current_anim_state = item->goal_anim_state;
            Item_SwitchToAnim(item, anim_idx, 0);

            item->rot.z = 0;
            item->rot.x = 0;
            item->gravity = false;
            item->fall_speed = 0;
            lara_info->dive_timer = 0;
            lara_info->torso_rot.y = 0;
            lara_info->torso_rot.x = 0;
            lara_info->head_rot.y = 0;
            lara_info->head_rot.x = 0;
            Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
        }
        break;
    }

    default:
        break;
    }
}

static void M_HandleAboveWater(COLL_INFO *const coll)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    coll->old = item->pos;
    coll->old_anim_state = item->current_anim_state;
    coll->old_anim_num = item->anim_num;
    coll->old_frame_num = item->frame_num;
    coll->radius = LARA_RADIUS;

    coll->lava_is_pit = 0;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->enable_hit = 1;
    coll->enable_baddie_push = 1;

    Lara_Look_Update();

    const ITEM *const vehicle = Lara_Vehicle_GetItem();
#if TR_VERSION >= 2
    if (vehicle != nullptr) {
        if (vehicle->object_id == O_SKIDOO_FAST) {
            // TODO: make this Object_Get(O_SKIDOO_FAST)->control
            if (Skidoo_Control()) {
                return;
            }
        } else {
            Gun_Control();
            return;
        }
    }
#endif

    Lara_State_Update(item, coll);

    if (item->rot.z < -LARA_LEAN_UNDO) {
        item->rot.z += LARA_LEAN_UNDO;
    } else if (item->rot.z > LARA_LEAN_UNDO) {
        item->rot.z -= LARA_LEAN_UNDO;
    } else {
        item->rot.z = 0;
    }

    if (lara_info->turn_rate < -LARA_TURN_UNDO) {
        lara_info->turn_rate += LARA_TURN_UNDO;
    } else if (lara_info->turn_rate > LARA_TURN_UNDO) {
        lara_info->turn_rate -= LARA_TURN_UNDO;
    } else {
        lara_info->turn_rate = 0;
    }
    item->rot.y += lara_info->turn_rate;

    Lara_Animate(item);
    const SECTOR *const sector = M_GetCurrentSector();

    if ((TR_VERSION == 1 || !lara_info->extra_anim)
        && lara_info->water_status != LWS_CHEAT) {
        M_ObjectCollision(coll);
        if (!Lara_Vehicle_IsMounted()) {
            Lara_Col_Update(item, coll);
        }
    }

    Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

static void M_HandleUnderwater(COLL_INFO *const coll)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    coll->old = item->pos;
    coll->radius = M_RADIUS_UW;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_HEIGHT_UW;
    coll->bad_ceiling = LARA_HEIGHT_UW;

    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    Lara_Look_Update();
    Lara_State_Update(item, coll);

    if (item->rot.z > M_LEAN_UNDO_UW) {
        item->rot.z -= M_LEAN_UNDO_UW;
    } else if (item->rot.z < -M_LEAN_UNDO_UW) {
        item->rot.z += M_LEAN_UNDO_UW;
    } else {
        item->rot.z = 0;
    }

    if (g_Config.gameplay.enable_tr2_swimming) {
        CLAMP(item->rot.x, -M_DIVE_TILT_MAX, M_DIVE_TILT_MAX);
        CLAMP(item->rot.z, -M_LEAN_MAX_UW, M_LEAN_MAX_UW);

        if (lara_info->turn_rate < -LARA_TURN_UNDO) {
            lara_info->turn_rate += LARA_TURN_UNDO;
        } else if (lara_info->turn_rate > LARA_TURN_UNDO) {
            lara_info->turn_rate -= LARA_TURN_UNDO;
        } else {
            lara_info->turn_rate = 0;
        }
        item->rot.y += lara_info->turn_rate;
    } else {
        CLAMP(item->rot.x, -M_DIVE_TILT_MAX_ALT, M_DIVE_TILT_MAX_ALT);
        CLAMP(item->rot.z, -M_LEAN_MAX_UW, M_LEAN_MAX_UW);
    }

    if (lara_info->current_active && lara_info->water_status != LWS_CHEAT) {
        M_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&lara_info->lot);
    }

    Lara_Animate(item);
    item->pos.y -=
        (item->fall_speed * Math_Sin(item->rot.x)) >> (W2V_SHIFT + 2);
    item->pos.x +=
        (Math_Cos(item->rot.x)
         * ((item->fall_speed * Math_Sin(item->rot.y)) >> (W2V_SHIFT + 2)))
        >> W2V_SHIFT;
    item->pos.z +=
        (Math_Cos(item->rot.x)
         * ((item->fall_speed * Math_Cos(item->rot.y)) >> (W2V_SHIFT + 2)))
        >> W2V_SHIFT;

    const SECTOR *const sector = M_GetCurrentSector();
    if (TR_VERSION == 1 || !lara_info->extra_anim) {
        M_ObjectCollision(coll);
    }

    if (lara_info->water_status == LWS_CHEAT) {
        if (m_OpenDoorsCheatCooldown > 0) {
            m_OpenDoorsCheatCooldown--;
        } else if (g_Input.draw) {
            m_OpenDoorsCheatCooldown = LOGIC_FPS;
            Lara_Cheat_OpenNearestDoor();
        }
    }

    if (TR_VERSION == 1 || !lara_info->extra_anim) {
        Lara_Col_Update(item, coll);
    }

    Lara_UpdateRoomToHeight(0);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

static void M_HandleSurface(COLL_INFO *const coll)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    g_Camera.target_elevation = CAM_WADE_ELEVATION;

    coll->old = item->pos;
    coll->radius = M_RADIUS_SURF;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = TR_VERSION == 1 ? -100 : -STEP_L / 2;
    coll->bad_ceiling = 100;

    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    Lara_Look_Update();
    Lara_State_Update(item, coll);

    if (item->rot.z > M_LEAN_UNDO_SURF) {
        item->rot.z -= M_LEAN_UNDO_SURF;
    } else if (item->rot.z < -M_LEAN_UNDO_SURF) {
        item->rot.z += M_LEAN_UNDO_SURF;
    } else {
        item->rot.z = 0;
    }

    if (lara_info->current_active && lara_info->water_status != LWS_CHEAT) {
        M_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&lara_info->lot);
    }

    Lara_Animate(item);
    item->pos.x +=
        (item->fall_speed * Math_Sin(lara_info->move_angle)) >> (W2V_SHIFT + 2);
    item->pos.z +=
        (item->fall_speed * Math_Cos(lara_info->move_angle)) >> (W2V_SHIFT + 2);

    const SECTOR *const sector = M_GetCurrentSector();

    M_ObjectCollision(coll);
    if (!Lara_Vehicle_IsMounted()) {
        Lara_Col_Update(item, coll);
    }

    Lara_UpdateRoomToHeight(100);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

static void M_HandleExposure(void)
{
    if (!Level_HasColdWater()) {
        return;
    }

    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    switch (lara_info->water_status) {
    case LWS_ABOVE_WATER:
        lara_info->exposure_timer++;
        CLAMPG(lara_info->exposure_timer, LARA_MAX_EXPOSURE);
        break;
    case LWS_WADE:
        lara_info->exposure_timer--;
        break;
    case LWS_UNDERWATER:
    case LWS_SURFACE:
        lara_info->exposure_timer -= 2;
        break;
    case LWS_CHEAT:
        lara_info->exposure_timer = LARA_MAX_EXPOSURE;
        break;
    }

    if (lara_info->exposure_timer < 0) {
        lara_info->exposure_timer = -1;
        Lara_TakeDamage(10, false);
    }
}

static void M_HandleEnvironment(void)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    COLL_INFO coll = {};

    if (item->current_anim_state != LS(LS_SPRINT)) {
        lara_info->sprint_timer++;
        CLAMPG(lara_info->sprint_timer, LARA_MAX_SPRINT);
    }
    if (item->current_anim_state != LS(LS_STOP)
        && item->current_anim_state != LS(LS_POSE)) {
        lara_info->idle_timer = 0;
    }

    switch (lara_info->water_status) {
    case LWS_ABOVE_WATER:
    case LWS_WADE:
        lara_info->air = LARA_MAX_AIR;
        M_HandleAboveWater(&coll);
        break;

    case LWS_UNDERWATER:
        if (item->hit_points >= 0) {
            lara_info->air--;
            if (lara_info->air < 0) {
                lara_info->air = -1;
                Lara_TakeDamage(M_UW_DAMAGE, false);
            }
        }
        M_HandleUnderwater(&coll);
        break;

    case LWS_SURFACE:
        if (item->hit_points >= 0) {
            lara_info->air += 10;
            CLAMPG(lara_info->air, LARA_MAX_AIR);
        }
        M_HandleSurface(&coll);
        break;

    case LWS_CHEAT:
        item->hit_points = LARA_MAX_HITPOINTS;
        lara_info->death_timer = 0;
        M_HandleUnderwater(&coll);
        if (g_InputDB.slow && !g_Input.look && !g_Input.fly_cheat) {
            Lara_Cheat_ExitFlyMode();
        }
        break;

    default:
        break;
    }

    M_HandleExposure();
}

void Lara_Control_Initialise(
    const GF_LEVEL_TYPE level_type, const LARA_EXTRA_STATE start_state)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    if ((level_type == GFL_NORMAL || level_type == GFL_BONUS)
        && start_state != LS_EXTRA_BREATH) {
        lara_info->water_status = LWS_ABOVE_WATER;
        lara_info->gun_status = LGS_HANDS_BUSY;
        lara_item->goal_anim_state = start_state;
        lara_item->current_anim_state = LS_EXTRA_BREATH;
        Item_SwitchToObjAnim(lara_item, LS_EXTRA_BREATH, 0, O_LARA_EXTRA);
        Lara_Animate(lara_item);
        lara_info->extra_anim = true;
        Camera_InvokeCinematic(lara_item, 0, 0);
    } else if ((Room_Get(lara_item->room_num)->flags & RF_UNDERWATER) != 0) {
        lara_info->water_status = LWS_UNDERWATER;
        lara_item->fall_speed = 0;
        lara_item->goal_anim_state = LS(LS_TREAD);
        lara_item->current_anim_state = LS(LS_TREAD);
        Item_SwitchToAnim(lara_item, LA(LA_UNDERWATER_IDLE), 0);
    } else {
        lara_info->water_status = LWS_ABOVE_WATER;
        lara_item->goal_anim_state = LS(LS_STOP);
        lara_item->current_anim_state = LS(LS_STOP);
        Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);
    }
}

void Lara_Control(void)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    if (item->hit_points > 0 && g_Config.debug.enable_invulnerability) {
        item->hit_points = LARA_MAX_HITPOINTS;
    }

    M_Cheat();

    if (TR_VERSION == 1 && lara_info->interact_target.is_moving
        && lara_info->interact_target.move_count++ > M_MOVE_TIMEOUT) {
        lara_info->interact_target.is_moving = false;
        lara_info->gun_status = LGS_ARMLESS;
    }

    M_UpdateEnvironment();

    if (item->hit_points <= 0) {
        item->hit_points = -1;
        if (Game_IsInGym()) {
            Gym_SetInventoryOpenEnabled(true);
        }
        if (lara_info->death_timer == 0) {
            Music_Stop();
            Stats_AddDeath();
        }
        lara_info->death_timer++;
        lara_info->target = nullptr;

        if ((item->flags & IF_ONE_SHOT) != 0) {
            lara_info->death_timer++;
            return;
        }
    } else if (Room_IsAbyssHeight(item->pos.y)) {
        item->hit_points = -1;
        lara_info->death_timer = 9 * LOGIC_FPS;
    }

    Camera_MoveManual();
    M_HandleEnvironment();

    Stats_AddDistanceTravelled(item->pos, lara_info->last_pos);
    lara_info->last_pos = item->pos;
}
