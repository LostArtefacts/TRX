#include "game/lara/lara_control.h"

#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/gameflow.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_col.h"
#include "game/lara/lara_look.h"
#include "game/lara/lara_state.h"
#include "game/lot.h"
#include "game/objects/general/door.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_BADDIE_COLLISION 12

static int32_t m_OpenDoorsCheatCooldown = 0;

static void Lara_WaterCurrent(COLL_INFO *coll);
static void Lara_BaddieCollision(ITEM_INFO *lara_item, COLL_INFO *coll);

static void Lara_WaterCurrent(COLL_INFO *coll)
{
    XYZ_32 target;

    ITEM_INFO *item = g_LaraItem;
    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    FLOOR_INFO *floor =
        &r->floor
             [((item->pos.z - r->z) >> WALL_SHIFT)
              + ((item->pos.x - r->x) >> WALL_SHIFT) * r->x_size];
    item->box_number = floor->box;

    if (Box_CalculateTarget(&target, item, &g_Lara.LOT) == TARGET_NONE) {
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
        item->room_number, UW_HEIGHT);

    if (coll->coll_type == COLL_FRONT) {
        if (item->rot.x > 35 * PHD_DEGREE) {
            item->rot.x += UW_WALLDEFLECT;
        } else if (item->rot.x < -35 * PHD_DEGREE) {
            item->rot.x -= UW_WALLDEFLECT;
        } else {
            item->fall_speed = 0;
        }
    } else if (coll->coll_type == COLL_TOP) {
        item->rot.x -= UW_WALLDEFLECT;
    } else if (coll->coll_type == COLL_TOPFRONT) {
        item->fall_speed = 0;
    } else if (coll->coll_type == COLL_LEFT) {
        item->rot.y += 5 * PHD_DEGREE;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->rot.y -= 5 * PHD_DEGREE;
    }

    if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
        item->rot.x += UW_WALLDEFLECT;
    }
    Item_ShiftCol(item, coll);

    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
}

static void Lara_BaddieCollision(ITEM_INFO *lara_item, COLL_INFO *coll)
{
    lara_item->hit_status = 0;
    g_Lara.hit_direction = -1;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t numroom = 0;
    int16_t roomies[MAX_BADDIE_COLLISION];

    roomies[numroom++] = lara_item->room_number;

    DOOR_INFOS *door = g_RoomInfo[lara_item->room_number].doors;
    if (door) {
        for (int i = 0; i < door->count; i++) {
            if (numroom >= MAX_BADDIE_COLLISION) {
                break;
            }
            roomies[numroom++] = door->door[i].room_num;
        }
    }

    for (int i = 0; i < numroom; i++) {
        int16_t item_num = g_RoomInfo[roomies[i]].item_number;
        while (item_num != NO_ITEM) {
            ITEM_INFO *item = &g_Items[item_num];
            if (item->collidable && item->status != IS_INVISIBLE) {
                OBJECT_INFO *object = &g_Objects[item->object_number];
                if (object->collision) {
                    int32_t x = lara_item->pos.x - item->pos.x;
                    int32_t y = lara_item->pos.y - item->pos.y;
                    int32_t z = lara_item->pos.z - item->pos.z;
                    if (x > -TARGET_DIST && x < TARGET_DIST && y > -TARGET_DIST
                        && y < TARGET_DIST && z > -TARGET_DIST
                        && z < TARGET_DIST) {
                        object->collision(item_num, lara_item, coll);
                    }
                }
            }
            item_num = item->next_item;
        }
    }

    if (g_Lara.spaz_effect_count && g_Lara.spaz_effect && coll->enable_spaz) {
        int32_t x = g_Lara.spaz_effect->pos.x - lara_item->pos.x;
        int32_t z = g_Lara.spaz_effect->pos.z - lara_item->pos.z;
        PHD_ANGLE hitang = lara_item->rot.y - (PHD_180 + Math_Atan(z, x));
        g_Lara.hit_direction = (hitang + PHD_45) / PHD_90;
        if (!g_Lara.hit_frame) {
            Sound_Effect(SFX_LARA_BODYSL, &lara_item->pos, SPM_NORMAL);
        }

        g_Lara.hit_frame++;
        if (g_Lara.hit_frame > 34) {
            g_Lara.hit_frame = 34;
        }

        g_Lara.spaz_effect_count--;
    }

    if (g_Lara.hit_direction == -1) {
        g_Lara.hit_frame = 0;
    }

    g_InvChosen = -1;
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
    Lara_BaddieCollision(item, coll);
    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, -LARA_HEIGHT / 2);
    Gun_Control();
    Room_TestTriggers(coll->trigger, false);
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
        Lara_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.LOT);
    }

    Lara_Animate(item);

    item->pos.x +=
        (Math_Sin(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.z +=
        (Math_Cos(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);

    Lara_BaddieCollision(item, coll);

    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, 100);
    Gun_Control();
    Room_TestTriggers(coll->trigger, false);
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

    if (item->rot.z >= -(2 * LARA_LEAN_UNDO)
        && item->rot.z <= 2 * LARA_LEAN_UNDO) {
        item->rot.z = 0;
    } else if (item->rot.z < 0) {
        item->rot.z += 2 * LARA_LEAN_UNDO;
    } else {
        item->rot.z -= 2 * LARA_LEAN_UNDO;
    }

    if (item->rot.x < -100 * PHD_DEGREE) {
        item->rot.x = -100 * PHD_DEGREE;
    } else if (item->rot.x > 100 * PHD_DEGREE) {
        item->rot.x = 100 * PHD_DEGREE;
    }

    if (item->rot.z < -LARA_LEAN_MAX_UW) {
        item->rot.z = -LARA_LEAN_MAX_UW;
    } else if (item->rot.z > LARA_LEAN_MAX_UW) {
        item->rot.z = LARA_LEAN_MAX_UW;
    }

    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        Lara_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.LOT);
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

    if (g_Lara.water_status != LWS_CHEAT) {
        Lara_BaddieCollision(item, coll);
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        if (m_OpenDoorsCheatCooldown) {
            m_OpenDoorsCheatCooldown--;
        } else if (g_Input.draw) {
            m_OpenDoorsCheatCooldown = LOGIC_FPS;
            Door_OpenNearest(g_LaraItem);
        }
    }

    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, 0);
    Gun_Control();
    Room_TestTriggers(coll->trigger, false);
}

void Lara_CheatGetStuff(void)
{
    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        return;
    }

    // play pistols drawing sound
    Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);

    Inv_AddItem(O_GUN_ITEM);

    if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
        Inv_AddItem(O_SHOTGUN_ITEM);
    }
    g_Lara.shotgun.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 300;

    if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
        Inv_AddItem(O_MAGNUM_ITEM);
    }
    g_Lara.magnums.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 1000;

    if (!Inv_RequestItem(O_UZI_ITEM)) {
        Inv_AddItem(O_UZI_ITEM);
    }
    g_Lara.uzis.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 2000;

    for (int i = 0; i < 10; i++) {
        if (Inv_RequestItem(O_MEDI_ITEM) < 240) {
            Inv_AddItem(O_MEDI_ITEM);
        }
        if (Inv_RequestItem(O_BIGMEDI_ITEM) < 240) {
            Inv_AddItem(O_BIGMEDI_ITEM);
        }
    }

    if (!Inv_RequestItem(O_KEY_ITEM1)) {
        Inv_AddItem(O_KEY_ITEM1);
    }
    if (!Inv_RequestItem(O_KEY_ITEM2)) {
        Inv_AddItem(O_KEY_ITEM2);
    }
    if (!Inv_RequestItem(O_KEY_ITEM3)) {
        Inv_AddItem(O_KEY_ITEM3);
    }
    if (!Inv_RequestItem(O_KEY_ITEM4)) {
        Inv_AddItem(O_KEY_ITEM4);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM1)) {
        Inv_AddItem(O_PUZZLE_ITEM1);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM2)) {
        Inv_AddItem(O_PUZZLE_ITEM2);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM3)) {
        Inv_AddItem(O_PUZZLE_ITEM3);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM4)) {
        Inv_AddItem(O_PUZZLE_ITEM4);
    }
    if (!Inv_RequestItem(O_PICKUP_ITEM1)) {
        Inv_AddItem(O_PICKUP_ITEM1);
    }
    if (!Inv_RequestItem(O_PICKUP_ITEM2)) {
        Inv_AddItem(O_PICKUP_ITEM2);
    }
}
