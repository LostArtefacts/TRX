#include "game/lara/lara_control.h"

#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/gameflow.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara/lara.h"
#include "game/lara/lara_col.h"
#include "game/lara/lara_look.h"
#include "game/lara/lara_state.h"
#include "game/objects/door.h"
#include "game/sound.h"
#include "global/vars.h"
#include "math/math.h"

static int32_t m_OpenDoorsCheatCooldown = 0;

static void Lara_WaterCurrent(COLL_INFO *coll);

static void Lara_WaterCurrent(COLL_INFO *coll)
{
    PHD_VECTOR target;

    ITEM_INFO *item = g_LaraItem;
    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    FLOOR_INFO *floor =
        &r->floor
             [((item->pos.z - r->z) >> WALL_SHIFT)
              + ((item->pos.x - r->x) >> WALL_SHIFT) * r->x_size];
    item->box_number = floor->box;

    if (CalculateTarget(&target, item, &g_Lara.LOT) == TARGET_NONE) {
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
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HEIGHT / 2, item->pos.z,
        item->room_number, UW_HEIGHT);

    if (coll->coll_type == COLL_FRONT) {
        if (item->pos.x_rot > 35 * PHD_DEGREE) {
            item->pos.x_rot += UW_WALLDEFLECT;
        } else if (item->pos.x_rot < -35 * PHD_DEGREE) {
            item->pos.x_rot -= UW_WALLDEFLECT;
        } else {
            item->fall_speed = 0;
        }
    } else if (coll->coll_type == COLL_TOP) {
        item->pos.x_rot -= UW_WALLDEFLECT;
    } else if (coll->coll_type == COLL_TOPFRONT) {
        item->fall_speed = 0;
    } else if (coll->coll_type == COLL_LEFT) {
        item->pos.y_rot += 5 * PHD_DEGREE;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->pos.y_rot -= 5 * PHD_DEGREE;
    }

    if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
        item->pos.x_rot += UW_WALLDEFLECT;
    }
    Item_ShiftCol(item, coll);

    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
}

void Lara_HandleAboveWater(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = LARA_RAD;
    coll->trigger = NULL;

    coll->lava_is_pit = 0;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->enable_spaz = 1;
    coll->enable_baddie_push = 1;

    if (g_Config.enable_enhanced_look && item->hit_points > 0) {
        if (g_Input.look) {
            Lara_LookLeftRight();
        } else {
            Lara_ResetLook();
        }
    }

    g_LaraStateRoutines[item->current_anim_state](item, coll);

    if (g_Camera.type != CAM_LOOK) {
        if (g_Lara.head_x_rot > -HEAD_TURN / 2
            && g_Lara.head_x_rot < HEAD_TURN / 2) {
            g_Lara.head_x_rot = 0;
        } else {
            g_Lara.head_x_rot -= g_Lara.head_x_rot / 8;
        }
        g_Lara.torso_x_rot = g_Lara.head_x_rot;

        if (g_Lara.head_y_rot > -HEAD_TURN / 2
            && g_Lara.head_y_rot < HEAD_TURN / 2) {
            g_Lara.head_y_rot = 0;
        } else {
            g_Lara.head_y_rot -= g_Lara.head_y_rot / 8;
        }
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    }

    if (item->pos.z_rot >= -LARA_LEAN_UNDO
        && item->pos.z_rot <= LARA_LEAN_UNDO) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot < -LARA_LEAN_UNDO) {
        item->pos.z_rot += LARA_LEAN_UNDO;
    } else {
        item->pos.z_rot -= LARA_LEAN_UNDO;
    }

    if (g_Lara.turn_rate >= -LARA_TURN_UNDO
        && g_Lara.turn_rate <= LARA_TURN_UNDO) {
        g_Lara.turn_rate = 0;
    } else if (g_Lara.turn_rate < -LARA_TURN_UNDO) {
        g_Lara.turn_rate += LARA_TURN_UNDO;
    } else {
        g_Lara.turn_rate -= LARA_TURN_UNDO;
    }
    item->pos.y_rot += g_Lara.turn_rate;

    Lara_Animate(item);
    LaraBaddieCollision(item, coll);
    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, -LARA_HEIGHT / 2);
    Gun_Control();
    TestTriggers(coll->trigger, 0);
}

void Lara_HandleSurface(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Camera.target_elevation = -22 * PHD_DEGREE;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -100;
    coll->bad_ceiling = 100;
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = SURF_RADIUS;
    coll->trigger = NULL;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    g_LaraStateRoutines[item->current_anim_state](item, coll);

    if (item->pos.z_rot >= -364 && item->pos.z_rot <= 364) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot >= 0) {
        item->pos.z_rot -= 364;
    } else {
        item->pos.z_rot += 364;
    }

    if (g_Camera.type != CAM_LOOK) {
        if (g_Lara.head_y_rot > -HEAD_TURN_SURF
            && g_Lara.head_y_rot < HEAD_TURN_SURF) {
            g_Lara.head_y_rot = 0;
        } else {
            g_Lara.head_y_rot -= g_Lara.head_y_rot / 8;
        }
        g_Lara.torso_y_rot = g_Lara.head_x_rot / 2;

        if (g_Lara.head_x_rot > -HEAD_TURN_SURF
            && g_Lara.head_x_rot < HEAD_TURN_SURF) {
            g_Lara.head_x_rot = 0;
        } else {
            g_Lara.head_x_rot -= g_Lara.head_x_rot / 8;
        }
        g_Lara.torso_x_rot = 0;
    }

    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        Lara_WaterCurrent(coll);
    }

    Lara_Animate(item);

    item->pos.x +=
        (Math_Sin(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.z +=
        (Math_Cos(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);

    LaraBaddieCollision(item, coll);

    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, 100);
    Gun_Control();
    TestTriggers(coll->trigger, 0);
}

void Lara_HandleUnderwater(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -UW_HEIGHT;
    coll->bad_ceiling = UW_HEIGHT;
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = UW_RADIUS;
    coll->trigger = NULL;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    if (g_Config.enable_enhanced_look && item->hit_points > 0) {
        if (g_Input.look) {
            Lara_LookLeftRight();
        } else {
            Lara_ResetLook();
        }
    }

    g_LaraStateRoutines[item->current_anim_state](item, coll);

    if (item->pos.z_rot >= -(2 * LARA_LEAN_UNDO)
        && item->pos.z_rot <= 2 * LARA_LEAN_UNDO) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot < 0) {
        item->pos.z_rot += 2 * LARA_LEAN_UNDO;
    } else {
        item->pos.z_rot -= 2 * LARA_LEAN_UNDO;
    }

    if (item->pos.x_rot < -100 * PHD_DEGREE) {
        item->pos.x_rot = -100 * PHD_DEGREE;
    } else if (item->pos.x_rot > 100 * PHD_DEGREE) {
        item->pos.x_rot = 100 * PHD_DEGREE;
    }

    if (item->pos.z_rot < -LARA_LEAN_MAX_UW) {
        item->pos.z_rot = -LARA_LEAN_MAX_UW;
    } else if (item->pos.z_rot > LARA_LEAN_MAX_UW) {
        item->pos.z_rot = LARA_LEAN_MAX_UW;
    }

    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        Lara_WaterCurrent(coll);
    }

    Lara_Animate(item);

    item->pos.y -=
        (Math_Sin(item->pos.x_rot) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.x +=
        (((Math_Sin(item->pos.y_rot) * item->fall_speed) >> (W2V_SHIFT + 2))
         * Math_Cos(item->pos.x_rot))
        >> W2V_SHIFT;
    item->pos.z +=
        (((Math_Cos(item->pos.y_rot) * item->fall_speed) >> (W2V_SHIFT + 2))
         * Math_Cos(item->pos.x_rot))
        >> W2V_SHIFT;

    if (g_Lara.water_status != LWS_CHEAT) {
        LaraBaddieCollision(item, coll);
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        if (m_OpenDoorsCheatCooldown) {
            m_OpenDoorsCheatCooldown--;
        } else if (g_Input.draw) {
            m_OpenDoorsCheatCooldown = FRAMES_PER_SECOND;
            Door_OpenNearest(g_LaraItem);
        }
    }

    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, 0);
    Gun_Control();
    TestTriggers(coll->trigger, 0);
}

void Lara_CheatGetStuff(void)
{
    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        return;
    }

    // play pistols drawing sound
    Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);

    if (g_Objects[O_GUN_OPTION].loaded && !Inv_RequestItem(O_GUN_ITEM)) {
        Inv_AddItem(O_GUN_ITEM);
    }

    if (g_Objects[O_SHOTGUN_OPTION].loaded) {
        if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
            Inv_AddItem(O_SHOTGUN_ITEM);
        }
        g_Lara.shotgun.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 300;
    }

    if (g_Objects[O_MAGNUM_OPTION].loaded) {
        if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
            Inv_AddItem(O_MAGNUM_ITEM);
        }
        g_Lara.magnums.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 1000;
    }

    if (g_Objects[O_UZI_OPTION].loaded) {
        if (!Inv_RequestItem(O_UZI_ITEM)) {
            Inv_AddItem(O_UZI_ITEM);
        }
        g_Lara.uzis.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 2000;
    }

    for (int i = 0; i < 10; i++) {
        if (g_Objects[O_MEDI_OPTION].loaded
            && Inv_RequestItem(O_MEDI_ITEM) < 240) {
            Inv_AddItem(O_MEDI_ITEM);
        }
        if (g_Objects[O_BIGMEDI_OPTION].loaded
            && Inv_RequestItem(O_BIGMEDI_ITEM) < 240) {
            Inv_AddItem(O_BIGMEDI_ITEM);
        }
    }

    if (g_Objects[O_KEY_OPTION1].loaded && !Inv_RequestItem(O_KEY_ITEM1)) {
        Inv_AddItem(O_KEY_ITEM1);
    }
    if (g_Objects[O_KEY_OPTION2].loaded && !Inv_RequestItem(O_KEY_ITEM2)) {
        Inv_AddItem(O_KEY_ITEM2);
    }
    if (g_Objects[O_KEY_OPTION3].loaded && !Inv_RequestItem(O_KEY_ITEM3)) {
        Inv_AddItem(O_KEY_ITEM3);
    }
    if (g_Objects[O_KEY_OPTION4].loaded && !Inv_RequestItem(O_KEY_ITEM4)) {
        Inv_AddItem(O_KEY_ITEM4);
    }
    if (g_Objects[O_PUZZLE_OPTION1].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM1)) {
        Inv_AddItem(O_PUZZLE_ITEM1);
    }
    if (g_Objects[O_PUZZLE_OPTION2].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM2)) {
        Inv_AddItem(O_PUZZLE_ITEM2);
    }
    if (g_Objects[O_PUZZLE_OPTION3].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM3)) {
        Inv_AddItem(O_PUZZLE_ITEM3);
    }
    if (g_Objects[O_PUZZLE_OPTION4].loaded
        && !Inv_RequestItem(O_PUZZLE_ITEM4)) {
        Inv_AddItem(O_PUZZLE_ITEM4);
    }
    if (g_Objects[O_PICKUP_OPTION1].loaded
        && !Inv_RequestItem(O_PICKUP_ITEM1)) {
        Inv_AddItem(O_PICKUP_ITEM1);
    }
    if (g_Objects[O_PICKUP_OPTION2].loaded
        && !Inv_RequestItem(O_PICKUP_ITEM2)) {
        Inv_AddItem(O_PICKUP_ITEM2);
    }
}
