#include "game/lara/control.h"

#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/lara/col.h"
#include "game/lara/common.h"
#include "game/lara/look.h"
#include "game/lara/state.h"
#include "game/lot.h"
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

static void M_WaterCurrent(COLL_INFO *coll);
static void M_BaddieCollision(ITEM *lara_item, COLL_INFO *coll);

static void M_WaterCurrent(COLL_INFO *coll)
{
    XYZ_32 target;

    ITEM *const item = g_LaraItem;
    const ROOM *const r = &g_RoomInfo[item->room_num];
    const SECTOR *const sector =
        &r->sectors
             [((item->pos.z - r->pos.z) >> WALL_SHIFT)
              + ((item->pos.x - r->pos.x) >> WALL_SHIFT) * r->size.z];
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

static void M_BaddieCollision(ITEM *lara_item, COLL_INFO *coll)
{
    lara_item->hit_status = 0;
    g_Lara.hit_direction = -1;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t numroom = 0;
    int16_t roomies[MAX_BADDIE_COLLISION];

    roomies[numroom++] = lara_item->room_num;

    PORTALS *portals = g_RoomInfo[lara_item->room_num].portals;
    if (portals != NULL) {
        for (int i = 0; i < portals->count; i++) {
            if (numroom >= MAX_BADDIE_COLLISION) {
                break;
            }
            roomies[numroom++] = portals->portal[i].room_num;
        }
    }

    for (int i = 0; i < numroom; i++) {
        int16_t item_num = g_RoomInfo[roomies[i]].item_num;
        while (item_num != NO_ITEM) {
            ITEM *item = &g_Items[item_num];
            if (item->collidable && item->status != IS_INVISIBLE) {
                OBJECT *object = &g_Objects[item->object_id];
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
}

void Lara_HandleAboveWater(ITEM *item, COLL_INFO *coll)
{
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = LARA_RAD;

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
    M_BaddieCollision(item, coll);
    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, -LARA_HEIGHT / 2);
    Gun_Control();
    Room_TestTriggers(item);
}

void Lara_HandleSurface(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_elevation = -22 * PHD_DEGREE;

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
        M_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.lot);
    }

    Lara_Animate(item);

    item->pos.x +=
        (Math_Sin(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.z +=
        (Math_Cos(g_Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);

    M_BaddieCollision(item, coll);

    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    Item_UpdateRoom(item, 100);
    Gun_Control();
    Room_TestTriggers(item);
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

    if (g_Config.enable_tr2_swimming) {
        CLAMP(item->rot.x, -85 * PHD_DEGREE, 85 * PHD_DEGREE);
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
        CLAMP(item->rot.x, -100 * PHD_DEGREE, 100 * PHD_DEGREE);
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
    Item_UpdateRoom(item, 0);
    Gun_Control();
    Room_TestTriggers(item);
}
