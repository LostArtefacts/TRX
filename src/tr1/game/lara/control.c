#include "game/lara/control.h"

#include "game/box.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/lara/cheat.h"
#include "game/lara/col.h"
#include "game/lara/common.h"
#include "game/lara/look.h"
#include "game/lara/state.h"
#include "game/lot.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>

#include <stdint.h>

static int32_t m_OpenDoorsCheatCooldown = 0;

static void (*m_LaraStateRoutines[])(ITEM *item, COLL_INFO *coll) = {
    // clang-format off
    [LS_WALK]         = Lara_State_Walk,
    [LS_RUN]          = Lara_State_Run,
    [LS_STOP]         = Lara_State_Stop,
    [LS_JUMP_FORWARD] = Lara_State_ForwardJump,
    [LS_POSE]         = Lara_State_Empty,
    [LS_FAST_BACK]    = Lara_State_FastBack,
    [LS_TURN_RIGHT]   = Lara_State_TurnR,
    [LS_TURN_LEFT]    = Lara_State_TurnL,
    [LS_DEATH]        = Lara_State_Death,
    [LS_FAST_FALL]    = Lara_State_FastFall,
    [LS_HANG]         = Lara_State_Hang,
    [LS_REACH]        = Lara_State_Reach,
    [LS_SPLAT]        = Lara_State_Empty,
    [LS_TREAD]        = Lara_State_Tread,
    [LS_LAND]         = Lara_State_Empty,
    [LS_COMPRESS]     = Lara_State_Compress,
    [LS_BACK]         = Lara_State_Back,
    [LS_SWIM]         = Lara_State_Swim,
    [LS_GLIDE]        = Lara_State_Glide,
    [LS_CLIMB_UP]     = Lara_State_Null,
    [LS_FAST_TURN]    = Lara_State_FastTurn,
    [LS_STEP_RIGHT]   = Lara_State_StepRight,
    [LS_STEP_LEFT]    = Lara_State_StepLeft,
    [LS_HIT]          = Lara_State_Empty,
    [LS_SLIDE]        = Lara_State_Slide,
    [LS_JUMP_BACK]    = Lara_State_BackJump,
    [LS_JUMP_RIGHT]   = Lara_State_RightJump,
    [LS_JUMP_LEFT]    = Lara_State_LeftJump,
    [LS_JUMP_UP]      = Lara_State_UpJump,
    [LS_FALL_BACK]    = Lara_State_FallBack,
    [LS_HANG_LEFT]    = Lara_State_HangLeft,
    [LS_HANG_RIGHT]   = Lara_State_HangRight,
    [LS_SLIDE_BACK]   = Lara_State_SlideBack,
    [LS_SURF_TREAD]   = Lara_State_SurfTread,
    [LS_SURF_SWIM]    = Lara_State_SurfSwim,
    [LS_DIVE]         = Lara_State_Dive,
    [LS_PUSH_BLOCK]   = Lara_State_PushBlock,
    [LS_PULL_BLOCK]   = Lara_State_PullBlock,
    [LS_PP_READY]     = Lara_State_PPReady,
    [LS_PICKUP]       = Lara_State_Pickup,
    [LS_SWITCH_ON]    = Lara_State_SwitchOn,
    [LS_SWITCH_OFF]   = Lara_State_SwitchOff,
    [LS_USE_KEY]      = Lara_State_UseKey,
    [LS_USE_PUZZLE]   = Lara_State_UsePuzzle,
    [LS_UW_DEATH]     = Lara_State_UWDeath,
    [LS_ROLL]         = Lara_State_Empty,
    [LS_SPECIAL]      = Lara_State_Special,
    [LS_SURF_BACK]    = Lara_State_SurfBack,
    [LS_SURF_LEFT]    = Lara_State_SurfLeft,
    [LS_SURF_RIGHT]   = Lara_State_SurfRight,
    [LS_USE_MIDAS]    = Lara_State_UseMidas,
    [LS_DIE_MIDAS]    = Lara_State_DieMidas,
    [LS_SWAN_DIVE]    = Lara_State_SwanDive,
    [LS_FAST_DIVE]    = Lara_State_FastDive,
    [LS_GYMNAST]      = Lara_State_Null,
    [LS_WATER_OUT]    = Lara_State_WaterOut,
    [LS_CONTROLLED]   = Lara_State_Controlled,
    [LS_TWIST]        = Lara_State_Empty,
    [LS_UW_ROLL]      = Lara_State_UWRoll,
    [LS_WADE]         = Lara_State_Wade,
    [LS_RESPONSIVE]   = Lara_State_Empty,
    // clang-format on
};

static void M_WaterCurrent(COLL_INFO *coll);
static void M_BaddieCollision(ITEM *lara_item, COLL_INFO *coll);
static SECTOR *M_GetCurrentSector(const ITEM *lara_item);

static void M_WaterCurrent(COLL_INFO *coll)
{
    XYZ_32 target;

    ITEM *const item = g_LaraItem;
    const ROOM *const room = Room_Get(item->room_num);
    const SECTOR *const sector =
        Room_GetWorldSector(room, item->pos.x, item->pos.z);
    item->box_num = sector->box;

    if (Box_CalculateTarget(&target, item, &g_Lara.lot) == TARGET_NONE) {
        return;
    }

    target.x -= item->pos.x;
    if (target.x > g_Lara.current_active) {
        item->pos.x += g_Lara.current_active;
    } else if (target.x < -g_Lara.current_active) {
        item->pos.x -= g_Lara.current_active;
    } else {
        item->pos.x += target.x;
    }

    target.z -= item->pos.z;
    if (target.z > g_Lara.current_active) {
        item->pos.z += g_Lara.current_active;
    } else if (target.z < -g_Lara.current_active) {
        item->pos.z -= g_Lara.current_active;
    } else {
        item->pos.z += target.z;
    }

    target.y -= item->pos.y;
    if (target.y > g_Lara.current_active) {
        item->pos.y += g_Lara.current_active;
    } else if (target.y < -g_Lara.current_active) {
        item->pos.y -= g_Lara.current_active;
    } else {
        item->pos.y += target.y;
    }

    g_Lara.current_active = 0;

    coll->facing = (int16_t)Math_Atan(
        item->pos.z - coll->old.z, item->pos.x - coll->old.x);
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HEIGHT / 2, item->pos.z,
        item->room_num, UW_HEIGHT);

    if (coll->coll_type == COLL_FRONT) {
        if (item->rot.x > 35 * DEG_1) {
            item->rot.x += UW_WALLDEFLECT;
        } else if (item->rot.x < -35 * DEG_1) {
            item->rot.x -= UW_WALLDEFLECT;
        } else {
            item->fall_speed = 0;
        }
    } else if (coll->coll_type == COLL_TOP) {
        item->rot.x -= UW_WALLDEFLECT;
    } else if (coll->coll_type == COLL_TOP_FRONT) {
        item->fall_speed = 0;
    } else if (coll->coll_type == COLL_LEFT) {
        item->rot.y += 5 * DEG_1;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->rot.y -= 5 * DEG_1;
    }

    if (coll->side_mid.floor < 0) {
        item->pos.y += coll->side_mid.floor;
        item->rot.x += UW_WALLDEFLECT;
    }
    Lara_ShiftCol(coll);

    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
}

static void M_BaddieCollision(ITEM *lara_item, COLL_INFO *coll)
{
    lara_item->hit_status = 0;
    g_Lara.hit_direction = -1;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t roomies[12];
    const int32_t roomies_count =
        Room_GetAdjoiningRooms(lara_item->room_num, roomies, 12);

    for (int32_t i = 0; i < roomies_count; i++) {
        int16_t item_num = Room_Get(roomies[i])->item_num;
        while (item_num != NO_ITEM) {
            const ITEM *const item = Item_Get(item_num);
            if (item->collidable && item->status != IS_INVISIBLE) {
                const OBJECT *const obj = Object_Get(item->object_id);
                if (obj->collision_func != nullptr) {
                    int32_t x = lara_item->pos.x - item->pos.x;
                    int32_t y = lara_item->pos.y - item->pos.y;
                    int32_t z = lara_item->pos.z - item->pos.z;
                    if (x > -CREATURE_TARGET_DIST && x < CREATURE_TARGET_DIST
                        && y > -CREATURE_TARGET_DIST && y < CREATURE_TARGET_DIST
                        && z > -CREATURE_TARGET_DIST
                        && z < CREATURE_TARGET_DIST) {
                        obj->collision_func(item_num, lara_item, coll);
                    }
                }
            }
            item_num = item->next_item;
        }
    }

    if (g_Lara.hit_effect_count && g_Lara.hit_effect && coll->enable_hit) {
        int32_t x = g_Lara.hit_effect->pos.x - lara_item->pos.x;
        int32_t z = g_Lara.hit_effect->pos.z - lara_item->pos.z;
        PHD_ANGLE hitang = lara_item->rot.y - (DEG_180 + Math_Atan(z, x));
        g_Lara.hit_direction = (hitang + DEG_45) / DEG_90;
        if (!g_Lara.hit_frame) {
            Sound_Effect(SFX_LARA_BODYSL, &lara_item->pos, SPM_NORMAL);
        }

        g_Lara.hit_frame++;
        if (g_Lara.hit_frame > 34) {
            g_Lara.hit_frame = 34;
        }

        g_Lara.hit_effect_count--;
    }

    if (g_Lara.hit_direction == -1) {
        g_Lara.hit_frame = 0;
    }
}

static SECTOR *M_GetCurrentSector(const ITEM *const lara_item)
{
    int16_t room_num = lara_item->room_num;
    return Room_GetSector(
        lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z, &room_num);
}

void Lara_HandleAboveWater(ITEM *item, COLL_INFO *coll)
{
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->old_anim_state = item->current_anim_state;
    coll->old_anim_num = item->anim_num;
    coll->old_frame_num = item->frame_num;
    coll->radius = LARA_RAD;

    coll->lava_is_pit = 0;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->enable_hit = 1;
    coll->enable_baddie_push = 1;

    if (g_Config.gameplay.enable_enhanced_look && item->hit_points > 0) {
        if (g_Input.look) {
            Lara_LookLeftRight();
        } else {
            Lara_ResetLook();
        }
    }

    m_LaraStateRoutines[item->current_anim_state](item, coll);

    if (g_Camera.type != CAM_LOOK) {
        if (g_Lara.head_rot.x > -HEAD_TURN / 2
            && g_Lara.head_rot.x < HEAD_TURN / 2) {
            g_Lara.head_rot.x = 0;
        } else {
            g_Lara.head_rot.x -= g_Lara.head_rot.x / 8;
        }
        g_Lara.torso_rot.x = g_Lara.head_rot.x;

        if (g_Lara.head_rot.y > -HEAD_TURN / 2
            && g_Lara.head_rot.y < HEAD_TURN / 2) {
            g_Lara.head_rot.y = 0;
        } else {
            g_Lara.head_rot.y -= g_Lara.head_rot.y / 8;
        }
        g_Lara.torso_rot.y = g_Lara.head_rot.y;
    }

    if (item->rot.z >= -LARA_LEAN_UNDO && item->rot.z <= LARA_LEAN_UNDO) {
        item->rot.z = 0;
    } else if (item->rot.z < -LARA_LEAN_UNDO) {
        item->rot.z += LARA_LEAN_UNDO;
    } else {
        item->rot.z -= LARA_LEAN_UNDO;
    }

    if (g_Lara.turn_rate >= -LARA_TURN_UNDO
        && g_Lara.turn_rate <= LARA_TURN_UNDO) {
        g_Lara.turn_rate = 0;
    } else if (g_Lara.turn_rate < -LARA_TURN_UNDO) {
        g_Lara.turn_rate += LARA_TURN_UNDO;
    } else {
        g_Lara.turn_rate -= LARA_TURN_UNDO;
    }
    item->rot.y += g_Lara.turn_rate;

    Lara_Animate(item);
    const SECTOR *const sector = M_GetCurrentSector(item);

    M_BaddieCollision(item, coll);
    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

void Lara_HandleSurface(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_elevation = CAM_WADE_ELEVATION;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -100;
    coll->bad_ceiling = 100;
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = SURF_RADIUS;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    m_LaraStateRoutines[item->current_anim_state](item, coll);

    if (item->rot.z >= -364 && item->rot.z <= 364) {
        item->rot.z = 0;
    } else if (item->rot.z >= 0) {
        item->rot.z -= 364;
    } else {
        item->rot.z += 364;
    }

    if (g_Camera.type != CAM_LOOK) {
        if (g_Lara.head_rot.y > -HEAD_TURN_SURF
            && g_Lara.head_rot.y < HEAD_TURN_SURF) {
            g_Lara.head_rot.y = 0;
        } else {
            g_Lara.head_rot.y -= g_Lara.head_rot.y / 8;
        }
        g_Lara.torso_rot.y = g_Lara.head_rot.x / 2;

        if (g_Lara.head_rot.x > -HEAD_TURN_SURF
            && g_Lara.head_rot.x < HEAD_TURN_SURF) {
            g_Lara.head_rot.x = 0;
        } else {
            g_Lara.head_rot.x -= g_Lara.head_rot.x / 8;
        }
        g_Lara.torso_rot.x = 0;
    }

    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        M_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.lot);
    }

    Lara_Animate(item);

    item->pos.x +=
        (Math_Sin(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.z +=
        (Math_Cos(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);

    const SECTOR *const sector = M_GetCurrentSector(item);

    M_BaddieCollision(item, coll);
    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Lara_UpdateRoomToHeight(100);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}

void Lara_HandleUnderwater(ITEM *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -UW_HEIGHT;
    coll->bad_ceiling = UW_HEIGHT;
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = UW_RADIUS;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    if (g_Config.gameplay.enable_enhanced_look && item->hit_points > 0) {
        if (g_Input.look) {
            Lara_LookLeftRight();
        } else {
            Lara_ResetLook();
        }
    }

    m_LaraStateRoutines[item->current_anim_state](item, coll);

    if (item->rot.z >= -(2 * LARA_LEAN_UNDO)
        && item->rot.z <= 2 * LARA_LEAN_UNDO) {
        item->rot.z = 0;
    } else if (item->rot.z < 0) {
        item->rot.z += 2 * LARA_LEAN_UNDO;
    } else {
        item->rot.z -= 2 * LARA_LEAN_UNDO;
    }

    if (g_Config.gameplay.enable_tr2_swimming) {
        CLAMP(item->rot.x, -85 * DEG_1, 85 * DEG_1);
        CLAMP(item->rot.z, -LARA_LEAN_MAX_UW, LARA_LEAN_MAX_UW);

        if (g_Lara.turn_rate < -LARA_TURN_UNDO) {
            g_Lara.turn_rate += LARA_TURN_UNDO;
        } else if (g_Lara.turn_rate > LARA_TURN_UNDO) {
            g_Lara.turn_rate -= LARA_TURN_UNDO;
        } else {
            g_Lara.turn_rate = 0;
        }
        item->rot.y += g_Lara.turn_rate;
    } else {
        CLAMP(item->rot.x, -100 * DEG_1, 100 * DEG_1);
        CLAMP(item->rot.z, -LARA_LEAN_MAX_UW, LARA_LEAN_MAX_UW);
    }

    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        M_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.lot);
    }

    Lara_Animate(item);

    item->pos.y -=
        (Math_Sin(item->rot.x) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.x +=
        (((Math_Sin(item->rot.y) * item->fall_speed) >> (W2V_SHIFT + 2))
         * Math_Cos(item->rot.x))
        >> W2V_SHIFT;
    item->pos.z +=
        (((Math_Cos(item->rot.y) * item->fall_speed) >> (W2V_SHIFT + 2))
         * Math_Cos(item->rot.x))
        >> W2V_SHIFT;

    const SECTOR *const sector = M_GetCurrentSector(item);

    if (g_Lara.water_status != LWS_CHEAT) {
        M_BaddieCollision(item, coll);
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        if (m_OpenDoorsCheatCooldown) {
            m_OpenDoorsCheatCooldown--;
        } else if (g_Input.draw) {
            m_OpenDoorsCheatCooldown = LOGIC_FPS;
            Lara_Cheat_OpenNearestDoor();
        }
    }

    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Lara_UpdateRoomToHeight(0);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}
