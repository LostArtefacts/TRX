#include "game/lara.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/input.h"
#include "game/objects/door.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stddef.h>
#include <stdint.h>

static int32_t m_OpenDoorsCheatCooldown = 0;

void LaraUnderWater(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -UW_HITE;
    coll->bad_ceiling = UW_HITE;
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
            LookLeftRight();
        } else {
            ResetLook();
        }
    }

    g_LaraControlRoutines[item->current_anim_state](item, coll);

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
        LaraWaterCurrent(coll);
    }

    AnimateLara(item);

    item->pos.y -=
        (phd_sin(item->pos.x_rot) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.x +=
        (((phd_sin(item->pos.y_rot) * item->fall_speed) >> (W2V_SHIFT + 2))
         * phd_cos(item->pos.x_rot))
        >> W2V_SHIFT;
    item->pos.z +=
        (((phd_cos(item->pos.y_rot) * item->fall_speed) >> (W2V_SHIFT + 2))
         * phd_cos(item->pos.x_rot))
        >> W2V_SHIFT;

    if (g_Lara.water_status != LWS_CHEAT) {
        LaraBaddieCollision(item, coll);
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        if (m_OpenDoorsCheatCooldown) {
            m_OpenDoorsCheatCooldown--;
        } else if (g_Input.draw) {
            m_OpenDoorsCheatCooldown = FRAMES_PER_SECOND;
            OpenNearestDoors(g_LaraItem);
        }
    }

    g_LaraCollisionRoutines[item->current_anim_state](item, coll);
    UpdateLaraRoom(item, 0);
    LaraGun();
    TestTriggers(coll->trigger, 0);
}

void LaraAsSwim(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (g_Input.forward) {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    }
    if (g_Input.back) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    }
    if (g_Input.left) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
    }

    item->fall_speed += 8;
    if (g_Lara.water_status == LWS_CHEAT) {
        if (item->fall_speed > UW_MAXSPEED * 2) {
            item->fall_speed = UW_MAXSPEED * 2;
        }
    } else if (item->fall_speed > UW_MAXSPEED) {
        item->fall_speed = UW_MAXSPEED;
    }

    if (!g_Input.jump) {
        item->goal_anim_state = AS_GLIDE;
    }
}

void LaraAsGlide(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (g_Input.forward) {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    } else if (g_Input.back) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    }
    if (g_Input.left) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
    }
    if (g_Input.jump) {
        item->goal_anim_state = AS_SWIM;
    }

    item->fall_speed -= WATER_FRICTION;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }

    if (item->fall_speed <= (UW_MAXSPEED * 2) / 3) {
        item->goal_anim_state = AS_TREAD;
    }
}

void LaraAsTread(ITEM_INFO *item, COLL_INFO *coll)
{
    if (g_Config.enable_enhanced_look) {
        if (g_Input.look) {
            LookUpDown();
        }
    }

    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (g_Input.forward) {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    } else if (g_Input.back) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    }
    if (g_Input.left) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
    }
    if (g_Input.jump) {
        item->goal_anim_state = AS_SWIM;
    }

    item->fall_speed -= WATER_FRICTION;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }
}

void LaraAsDive(ITEM_INFO *item, COLL_INFO *coll)
{
    if (g_Input.forward) {
        item->pos.x_rot -= PHD_DEGREE;
    }
}

void LaraAsUWDeath(ITEM_INFO *item, COLL_INFO *coll)
{
    item->fall_speed -= 8;
    if (item->fall_speed <= 0) {
        item->fall_speed = 0;
    }

    if (item->pos.x_rot >= -2 * PHD_DEGREE
        && item->pos.x_rot <= 2 * PHD_DEGREE) {
        item->pos.x_rot = 0;
    } else if (item->pos.x_rot < 0) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    } else {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    }
}

void LaraColSwim(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraSwimCollision(item, coll);
}

void LaraColGlide(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraSwimCollision(item, coll);
}

void LaraColTread(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraSwimCollision(item, coll);
}

void LaraColDive(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraSwimCollision(item, coll);
}

void LaraColUWDeath(ITEM_INFO *item, COLL_INFO *coll)
{
    item->hit_points = -1;
    g_Lara.air = -1;
    g_Lara.gun_status = LGS_HANDSBUSY;
    int16_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT && wh < item->pos.y - 100) {
        item->pos.y -= 5;
    }
    LaraSwimCollision(item, coll);
}

void LaraSwimCollision(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->pos.x_rot >= -PHD_90 && item->pos.x_rot <= PHD_90) {
        g_Lara.move_angle = coll->facing = item->pos.y_rot;
    } else {
        g_Lara.move_angle = coll->facing = item->pos.y_rot - PHD_180;
    }
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HITE / 2, item->pos.z,
        item->room_number, UW_HITE);

    ShiftItem(item, coll);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->pos.x_rot > 35 * PHD_DEGREE) {
            item->pos.x_rot = item->pos.x_rot + UW_WALLDEFLECT;
            break;
        }
        if (item->pos.x_rot < -35 * PHD_DEGREE) {
            item->pos.x_rot = item->pos.x_rot - UW_WALLDEFLECT;
            break;
        }
        item->fall_speed = 0;
        break;

    case COLL_TOP:
        if (item->pos.x_rot >= -45 * PHD_DEGREE) {
            item->pos.x_rot -= UW_WALLDEFLECT;
        }
        break;

    case COLL_TOPFRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->pos.y_rot += 5 * PHD_DEGREE;
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= 5 * PHD_DEGREE;
        break;

    case COLL_CLAMP:
        item->fall_speed = 0;
        return;
        break;
    }

    if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
        item->pos.x_rot += UW_WALLDEFLECT;
    }
}

void LaraWaterCurrent(COLL_INFO *coll)
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

    coll->facing =
        (int16_t)phd_atan(item->pos.z - coll->old.z, item->pos.x - coll->old.x);
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HITE / 2, item->pos.z,
        item->room_number, UW_HITE);

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
    ShiftItem(item, coll);

    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
}
