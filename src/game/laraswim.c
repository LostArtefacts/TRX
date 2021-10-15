#include "game/lara.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/objects/door.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "util.h"

#include <stddef.h>
#include <stdint.h>

static int32_t OpenDoorsCheatCooldown = 0;

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

    if (T1MConfig.enable_enhanced_look && item->hit_points > 0) {
        if (Input & IN_LOOK) {
            LookLeftRight();
        } else {
            ResetLook();
        }
    }

    LaraControlRoutines[item->current_anim_state](item, coll);

    if (item->pos.z_rot >= -(2 * LARA_LEAN_UNDO)
        && item->pos.z_rot <= 2 * LARA_LEAN_UNDO) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot < 0) {
        item->pos.z_rot += 2 * LARA_LEAN_UNDO / ANIM_SCALE;
    } else {
        item->pos.z_rot -= 2 * LARA_LEAN_UNDO / ANIM_SCALE;
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

    if (Lara.current_active && Lara.water_status != LWS_CHEAT) {
        LaraWaterCurrent(coll);
    }

    AnimateLara(item);

    if (ANIM_SCALE == 2) {
        LaraFloatPos.y -=
            (phd_sin_f(item->pos.x_rot) * LaraFallSpeedF) / (View2World * 4);
        LaraFloatPos.x += ((phd_sin_f(item->pos.y_rot) * LaraFallSpeedF)
                             / (View2World * 4) * phd_cos_f(item->pos.x_rot))
            / View2World;
        LaraFloatPos.z += ((phd_cos_f(item->pos.y_rot) * LaraFallSpeedF)
                             / (View2World * 4) * phd_cos_f(item->pos.x_rot))
            / View2World;

        item->pos.y = LaraFloatPos.y;
        item->pos.x = LaraFloatPos.x;
        item->pos.z = LaraFloatPos.z;
    } else {
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

        LaraFloatPos.y = item->pos.y;
        LaraFloatPos.x = item->pos.x;
        LaraFloatPos.z = item->pos.z;
    }

    if (Lara.water_status != LWS_CHEAT) {
        LaraBaddieCollision(item, coll);
    }

    if (Lara.water_status == LWS_CHEAT) {
        if (OpenDoorsCheatCooldown) {
            OpenDoorsCheatCooldown--;
        } else if (CHK_ANY(Input, IN_DRAW)) {
            OpenDoorsCheatCooldown = FRAMES_PER_SECOND;
            OpenNearestDoors(LaraItem);
        }
    }

    LaraCollisionRoutines[item->current_anim_state](item, coll);
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

    if (Input & IN_FORWARD) {
        item->pos.x_rot -= 2 * PHD_DEGREE / ANIM_SCALE;
    }
    if (Input & IN_BACK) {
        item->pos.x_rot += 2 * PHD_DEGREE / ANIM_SCALE;
    }
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_MED_TURN / ANIM_SCALE;
        item->pos.z_rot -= LARA_LEAN_RATE * 2 / ANIM_SCALE;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_MED_TURN / ANIM_SCALE;
        item->pos.z_rot += LARA_LEAN_RATE * 2 / ANIM_SCALE;
    }

    LaraFallSpeedF += 8.0;
    item->fall_speed = LaraFallSpeedF;
    if (Lara.water_status == LWS_CHEAT) {
        if (item->fall_speed > UW_MAXSPEED * 2) {
            item->fall_speed = UW_MAXSPEED * 2 / ANIM_SCALE;
            LaraFallSpeedF = UW_MAXSPEED * 2.0 / ANIM_SCALE;
        }
    } else if (item->fall_speed > UW_MAXSPEED) {
        item->fall_speed = UW_MAXSPEED;
        LaraFallSpeedF = UW_MAXSPEED;
    }

    if (!(Input & IN_JUMP)) {
        item->goal_anim_state = AS_GLIDE;
    }
}
void LaraAsGlide(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (Input & IN_FORWARD) {
        item->pos.x_rot -= 2 * PHD_DEGREE / ANIM_SCALE;
    } else if (Input & IN_BACK) {
        item->pos.x_rot += 2 * PHD_DEGREE / ANIM_SCALE;
    }
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_MED_TURN / ANIM_SCALE;
        item->pos.z_rot -= LARA_LEAN_RATE * 2 / ANIM_SCALE;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_MED_TURN / ANIM_SCALE;
        item->pos.z_rot += LARA_LEAN_RATE * 2 / ANIM_SCALE;
    }
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_SWIM;
    }

    LaraFallSpeedF -= WATER_FRICTION;
    if (LaraFallSpeedF < 0) {
        LaraFallSpeedF = 0;
    }

    if (LaraFallSpeedF <= (UW_MAXSPEED * 2.0) / 3.0) {
        item->goal_anim_state = AS_TREAD;
    }
    item->fall_speed = LaraFallSpeedF;
}

void LaraAsTread(ITEM_INFO *item, COLL_INFO *coll)
{
    if (T1MConfig.enable_enhanced_look) {
        if (Input & IN_LOOK) {
            LookUpDown();
        }
    }

    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (Input & IN_FORWARD) {
        item->pos.x_rot -= 2 * PHD_DEGREE / ANIM_SCALE;
    } else if (Input & IN_BACK) {
        item->pos.x_rot += 2 * PHD_DEGREE / ANIM_SCALE;
    }
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_MED_TURN / ANIM_SCALE;
        item->pos.z_rot -= LARA_LEAN_RATE * 2 / ANIM_SCALE;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_MED_TURN / ANIM_SCALE;
        item->pos.z_rot += LARA_LEAN_RATE * 2 / ANIM_SCALE;
    }
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_SWIM;
    }

    LaraFallSpeedF -= WATER_FRICTION;

    if (LaraFallSpeedF < 0) {
        LaraFallSpeedF = 0;
    }
    item->fall_speed = LaraFallSpeedF;
}

void LaraAsDive(ITEM_INFO *item, COLL_INFO *coll)
{
    if (Input & AS_RUN) {
        item->pos.x_rot -= PHD_DEGREE / ANIM_SCALE;
    }
}

void LaraAsUWDeath(ITEM_INFO *item, COLL_INFO *coll)
{
    LaraFallSpeedF -= 8.0 / ANIM_SCALE;

    if (LaraFallSpeedF <= 0) {
        LaraFallSpeedF = 0;
    }
    item->fall_speed = LaraFallSpeedF;

    if (item->pos.x_rot >= -2 * PHD_DEGREE
        && item->pos.x_rot <= 2 * PHD_DEGREE) {
        item->pos.x_rot = 0;
    } else if (item->pos.x_rot < 0) {
        item->pos.x_rot += 2 * PHD_DEGREE / ANIM_SCALE;
    } else {
        item->pos.x_rot -= 2 * PHD_DEGREE / ANIM_SCALE;
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
    Lara.air = -1;
    Lara.gun_status = LGS_HANDSBUSY;
    int16_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT && wh < item->pos.y - 100) {
        item->pos.y -= 5;
        LaraFloatPos.y -= 5.0;
    }
    LaraSwimCollision(item, coll);
}

void LaraSwimCollision(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->pos.x_rot >= -PHD_90 && item->pos.x_rot <= PHD_90) {
        Lara.move_angle = coll->facing = item->pos.y_rot;
    } else {
        Lara.move_angle = coll->facing = item->pos.y_rot - PHD_180;
    }
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HITE / 2, item->pos.z,
        item->room_number, UW_HITE);

    ShiftItemLara(item, coll);

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
        LaraFallSpeedF = 0.0;
        break;

    case COLL_TOP:
        if (item->pos.x_rot >= -45 * PHD_DEGREE) {
            item->pos.x_rot -= UW_WALLDEFLECT;
        }
        break;

    case COLL_TOPFRONT:
        item->fall_speed = 0;
        LaraFallSpeedF = 0.0;
        break;

    case COLL_LEFT:
        item->pos.y_rot += 5 * PHD_DEGREE;
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= 5 * PHD_DEGREE;
        break;

    case COLL_CLAMP:
        item->fall_speed = 0;
        LaraFallSpeedF = 0.0;
        return;
        break;
    }

    if (coll->mid_floor < 0) {
        LaraFloatPos.y += coll->mid_floor;
        item->pos.y += coll->mid_floor;
        item->pos.x_rot += UW_WALLDEFLECT;
    }
}

void LaraWaterCurrent(COLL_INFO *coll)
{
    PHD_VECTOR target; // [esp+Ch] [ebp-Ch]

    ITEM_INFO *item = LaraItem;
    ROOM_INFO *r = &RoomInfo[item->room_number];
    FLOOR_INFO *floor =
        &r->floor
             [((item->pos.z - r->z) >> WALL_SHIFT)
              + ((item->pos.x - r->x) >> WALL_SHIFT) * r->x_size];
    item->box_number = floor->box;

    if (CalculateTarget(&target, item, &Lara.LOT) == TARGET_NONE) {
        return;
    }

    target.x -= item->pos.x;
    if (target.x > Lara.current_active) {
        LaraFloatPos.x += Lara.current_active;
        item->pos.x = LaraFloatPos.x;
    } else if (target.x < -Lara.current_active) {
        LaraFloatPos.x -= Lara.current_active;
        item->pos.x = LaraFloatPos.x;
    } else {
        item->pos.x += target.x;
        LaraFloatPos.x += target.x;
    }

    target.z -= item->pos.z;
    if (target.z > Lara.current_active) {
        LaraFloatPos.z += Lara.current_active;
        item->pos.z = LaraFloatPos.z;
    } else if (target.z < -Lara.current_active) {
        LaraFloatPos.z -= Lara.current_active;
        item->pos.z = LaraFloatPos.z;
    } else {
        item->pos.z += target.z;
        LaraFloatPos.z += target.z;
    }

    target.y -= item->pos.y;
    if (target.y > Lara.current_active) {
        LaraFloatPos.y += Lara.current_active;
        item->pos.y = LaraFloatPos.y;
    } else if (target.y < -Lara.current_active) {
        LaraFloatPos.y -= Lara.current_active;
        item->pos.y = LaraFloatPos.y;
    } else {
        item->pos.y += target.y;
        LaraFloatPos.y += target.y;
    }

    Lara.current_active = 0;

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
            LaraFallSpeedF = 0.0;
        }
    } else if (coll->coll_type == COLL_TOP) {
        item->pos.x_rot -= UW_WALLDEFLECT;
    } else if (coll->coll_type == COLL_TOPFRONT) {
        item->fall_speed = 0;
        LaraFallSpeedF = 0.0;
    } else if (coll->coll_type == COLL_LEFT) {
        item->pos.y_rot += 5 * PHD_DEGREE;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->pos.y_rot -= 5 * PHD_DEGREE;
    }

    if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
        LaraFloatPos.y += coll->mid_floor;
        item->pos.x_rot += UW_WALLDEFLECT;
    }
    ShiftItemLara(item, coll);

    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
}

void T1MInjectGameLaraSwim()
{
    INJECT(0x00428F10, LaraUnderWater);

    INJECT(0x004290C0, LaraAsSwim);
    INJECT(0x00429140, LaraAsGlide);
    INJECT(0x004291D0, LaraAsTread);
    INJECT(0x00429250, LaraAsDive);
    INJECT(0x00429270, LaraAsUWDeath);

    INJECT(0x004292C0, LaraColSwim);
    INJECT(0x004292E0, LaraColUWDeath);

    INJECT(0x00429340, LaraSwimCollision);
    INJECT(0x00429440, LaraWaterCurrent);
}
