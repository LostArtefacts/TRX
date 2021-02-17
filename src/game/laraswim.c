#include "game/collide.h"
#include "game/const.h"
#include "game/control.h"
#include "game/data.h"
#include "game/lara.h"
#include "game/shell.h"
#include "mod.h"
#include "util.h"

void __cdecl LaraUnderWater(ITEM_INFO* item, COLL_INFO* coll)
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

    if (TR1MConfig.enable_enhanced_look) {
        if (Input & IN_LOOK) {
            TR1MLookLeftRight();
        } else {
            TR1MResetLook();
        }
    }

    LaraControlRoutines[item->current_anim_state](item, coll);

    if (item->pos.z_rot >= -(2 * LARA_LEAN_UNDO)
        && item->pos.z_rot <= 2 * LARA_LEAN_UNDO) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot < 0) {
        item->pos.z_rot += 2 * LARA_LEAN_UNDO;
    } else {
        item->pos.z_rot -= 2 * LARA_LEAN_UNDO;
    }

    if (item->pos.x_rot < -100 * ONE_DEGREE) {
        item->pos.x_rot = -100 * ONE_DEGREE;
    } else if (item->pos.x_rot > 100 * ONE_DEGREE) {
        item->pos.x_rot = 100 * ONE_DEGREE;
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

    if (Lara.water_status != LWS_CHEAT) {
        LaraBaddieCollision(item, coll);
    }

    LaraCollisionRoutines[item->current_anim_state](item, coll);
    UpdateLaraRoom(item, 0);
    LaraGun();
    TestTriggers(coll->trigger, 0);
}

void __cdecl LaraAsSwim(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (Input & IN_FORWARD) {
        item->pos.x_rot -= 2 * ONE_DEGREE;
    }
    if (Input & IN_BACK) {
        item->pos.x_rot += 2 * ONE_DEGREE;
    }
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
    }

    item->fall_speed += 8;
    if (item->fall_speed > UW_MAXSPEED) {
        item->fall_speed = UW_MAXSPEED;
    }

    if (!(Input & IN_JUMP)) {
        item->goal_anim_state = AS_GLIDE;
    }
}

void __cdecl LaraAsGlide(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (Input & IN_FORWARD) {
        item->pos.x_rot -= 2 * ONE_DEGREE;
    } else if (Input & IN_BACK) {
        item->pos.x_rot += 2 * ONE_DEGREE;
    }
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
    }
    if (Input & IN_JUMP) {
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

void __cdecl LaraAsTread(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (Input & IN_FORWARD) {
        item->pos.x_rot -= 2 * ONE_DEGREE;
    } else if (Input & IN_BACK) {
        item->pos.x_rot += 2 * ONE_DEGREE;
    }
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
    }
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_SWIM;
    }

    item->fall_speed -= WATER_FRICTION;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }
}

void __cdecl LaraAsDive(ITEM_INFO* item, COLL_INFO* coll)
{
    if (Input & AS_RUN) {
        item->pos.x_rot -= ONE_DEGREE;
    }
}

void __cdecl LaraAsUWDeath(ITEM_INFO* item, COLL_INFO* coll)
{
    item->fall_speed -= 8;
    if (item->fall_speed <= 0) {
        item->fall_speed = 0;
    }

    if (item->pos.x_rot >= -2 * ONE_DEGREE
        && item->pos.x_rot <= 2 * ONE_DEGREE) {
        item->pos.x_rot = 0;
    } else if (item->pos.x_rot < 0) {
        item->pos.x_rot += 2 * ONE_DEGREE;
    } else {
        item->pos.x_rot -= 2 * ONE_DEGREE;
    }
}

void __cdecl LaraColSwim(ITEM_INFO* item, COLL_INFO* coll)
{
    LaraSwimCollision(item, coll);
}

void __cdecl LaraColUWDeath(ITEM_INFO* item, COLL_INFO* coll)
{
    item->hit_points = -1;
    Lara.air = -1;
    Lara.gun_status = LGS_HANDSBUSY;
    int wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT && wh < item->pos.y - 100) {
        item->pos.y -= 5;
    }
    LaraSwimCollision(item, coll);
}

void __cdecl LaraSwimCollision(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->pos.x_rot >= -PHD_ONE / 4 && item->pos.x_rot <= PHD_ONE / 4) {
        Lara.move_angle = coll->facing = item->pos.y_rot;
    } else {
        Lara.move_angle = coll->facing = item->pos.y_rot - PHD_ONE / 2;
    }
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + UW_HITE / 2, item->pos.z,
        item->room_number, UW_HITE);

    ShiftItem(item, coll);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->pos.x_rot > 35 * ONE_DEGREE) {
            item->pos.x_rot = item->pos.x_rot + UW_WALLDEFLECT;
            break;
        }
        if (item->pos.x_rot < -35 * ONE_DEGREE) {
            item->pos.x_rot = item->pos.x_rot - UW_WALLDEFLECT;
            break;
        }
        item->fall_speed = 0;
        break;

    case COLL_TOP:
        if (item->pos.x_rot >= -45 * ONE_DEGREE) {
            item->pos.x_rot -= UW_WALLDEFLECT;
        }
        break;

    case COLL_TOPFRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->pos.y_rot += 5 * ONE_DEGREE;
        break;

    case COLL_RIGHT:
        item->pos.y_rot -= 5 * ONE_DEGREE;
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

void TR1MInjectLaraSwim()
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
}
