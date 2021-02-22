#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/lara.h"
#include "game/misc.h"
#include "game/vars.h"
#include "util.h"

void LaraSurface(ITEM_INFO* item, COLL_INFO* coll)
{
    Camera.target_elevation = -22 * PHD_DEGREE;

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

    LaraControlRoutines[item->current_anim_state](item, coll);

    if (item->pos.z_rot >= -364 && item->pos.z_rot <= 364) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot >= 0) {
        item->pos.z_rot -= 364;
    } else {
        item->pos.z_rot += 364;
    }

    if (Camera.type != CAM_LOOK) {
        if (Lara.head_y_rot > -HEAD_TURN_SURF
            && Lara.head_y_rot < HEAD_TURN_SURF) {
            Lara.head_y_rot = 0;
        } else {
            Lara.head_y_rot -= Lara.head_y_rot / 8;
        }
        Lara.torso_y_rot = Lara.head_x_rot / 2;

        if (Lara.head_x_rot > -HEAD_TURN_SURF
            && Lara.head_x_rot < HEAD_TURN_SURF) {
            Lara.head_x_rot = 0;
        } else {
            Lara.head_x_rot -= Lara.head_x_rot / 8;
        }
        Lara.torso_x_rot = 0;
    }

    if (Lara.current_active && Lara.water_status != LWS_CHEAT) {
        LaraWaterCurrent(coll);
    }

    AnimateLara(item);

    item->pos.x +=
        (phd_sin(Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.z +=
        (phd_cos(Lara.move_angle) * item->fall_speed) >> (W2V_SHIFT + 2);

    LaraBaddieCollision(item, coll);

    LaraCollisionRoutines[item->current_anim_state](item, coll);
    UpdateLaraRoom(item, 100);
    LaraGun();
    TestTriggers(coll->trigger, 0);
}

void LaraAsSurfSwim(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    Lara.dive_count = 0;
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_SLOW_TURN;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_SLOW_TURN;
    }

    if (!(Input & IN_FORWARD)) {
        item->goal_anim_state = AS_SURFTREAD;
    }
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_SURFTREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
    }
}

void LaraAsSurfBack(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    Lara.dive_count = 0;
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_SLOW_TURN / 2;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_SLOW_TURN / 2;
    }

    if (!(Input & IN_BACK)) {
        item->goal_anim_state = AS_SURFTREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
    }
}

void LaraAsSurfLeft(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    Lara.dive_count = 0;
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_SLOW_TURN / 2;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_SLOW_TURN / 2;
    }

    if (!(Input & IN_STEPL)) {
        item->goal_anim_state = AS_SURFTREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
    }
}

void LaraAsSurfRight(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    Lara.dive_count = 0;
    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_SLOW_TURN / 2;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_SLOW_TURN / 2;
    }

    if (!(Input & IN_STEPR)) {
        item->goal_anim_state = AS_SURFTREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
    }
}

void LaraAsSurfTread(ITEM_INFO* item, COLL_INFO* coll)
{
    item->fall_speed -= 4;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }

    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_UWDEATH;
        return;
    }

    if (Input & IN_LOOK) {
        Camera.type = CAM_LOOK;
        if (Input & IN_LEFT && Lara.head_y_rot > -MAX_HEAD_ROTATION_SURF) {
            Lara.head_y_rot -= HEAD_TURN_SURF;
        } else if (
            (Input & IN_RIGHT) && Lara.head_y_rot < MAX_HEAD_ROTATION_SURF) {
            Lara.head_y_rot += HEAD_TURN_SURF;
        }
        Lara.torso_y_rot = Lara.head_y_rot / 2;

        if ((Input & IN_FORWARD) && Lara.head_x_rot > MIN_HEAD_TILT_SURF) {
            Lara.head_x_rot -= HEAD_TURN_SURF;
        } else if ((Input & IN_BACK) && Lara.head_x_rot < MAX_HEAD_TILT_SURF) {
            Lara.head_x_rot += HEAD_TURN_SURF;
        }
        Lara.torso_x_rot = 0;
        return;
    }
    if (Camera.type == CAM_LOOK) {
        Camera.type = CAM_CHASE;
    }

    if (Input & IN_LEFT) {
        item->pos.y_rot -= LARA_SLOW_TURN;
    } else if (Input & IN_RIGHT) {
        item->pos.y_rot += LARA_SLOW_TURN;
    }

    if (Input & IN_FORWARD) {
        item->goal_anim_state = AS_SURFSWIM;
    } else if (Input & IN_BACK) {
        item->goal_anim_state = AS_SURFBACK;
    }

    if (Input & IN_STEPL) {
        item->goal_anim_state = AS_SURFLEFT;
    } else if (Input & IN_STEPR) {
        item->goal_anim_state = AS_SURFRIGHT;
    }

    if (Input & IN_JUMP) {
        Lara.dive_count++;
        if (Lara.dive_count == DIVE_COUNT) {
            item->goal_anim_state = AS_SWIM;
            item->current_anim_state = AS_DIVE;
            item->anim_number = AA_SURFDIVE;
            item->frame_number = AF_SURFDIVE;
            item->pos.x_rot = -45 * PHD_DEGREE;
            item->fall_speed = 80;
            Lara.water_status = LWS_UNDERWATER;
        }
    } else {
        Lara.dive_count = 0;
    }
}

void LaraColSurfSwim(ITEM_INFO* item, COLL_INFO* coll)
{
    Lara.move_angle = item->pos.y_rot;
    LaraSurfaceCollision(item, coll);
}

void LaraColSurfTread(ITEM_INFO* item, COLL_INFO* coll)
{
    Lara.move_angle = item->pos.y_rot;
    LaraSurfaceCollision(item, coll);
}

void LaraColSurfBack(ITEM_INFO* item, COLL_INFO* coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_ONE / 2;
    LaraSurfaceCollision(item, coll);
}

void LaraColSurfLeft(ITEM_INFO* item, COLL_INFO* coll)
{
    Lara.move_angle = item->pos.y_rot - PHD_ONE / 4;
    LaraSurfaceCollision(item, coll);
}

void LaraColSurfRight(ITEM_INFO* item, COLL_INFO* coll)
{
    Lara.move_angle = item->pos.y_rot + PHD_ONE / 4;
    LaraSurfaceCollision(item, coll);
}

void LaraSurfaceCollision(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->facing = Lara.move_angle;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y + SURF_HITE, item->pos.z,
        item->room_number, SURF_HITE);

    ShiftItem(item, coll);

    if ((coll->coll_type
         & (COLL_FRONT | COLL_LEFT | COLL_RIGHT | COLL_TOP | COLL_TOPFRONT
            | COLL_CLAMP))
        || coll->mid_floor < 0) {
        item->fall_speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
    } else if (coll->coll_type == COLL_LEFT) {
        item->pos.y_rot += 5 * PHD_DEGREE;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->pos.y_rot -= 5 * PHD_DEGREE;
    }

    int wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh - item->pos.y <= -100) {
        item->goal_anim_state = AS_SWIM;
        item->current_anim_state = AS_DIVE;
        item->anim_number = AA_SURFDIVE;
        item->frame_number = AF_SURFDIVE;
        item->pos.x_rot = -45 * PHD_DEGREE;
        item->fall_speed = 80;
        Lara.water_status = LWS_UNDERWATER;
        return;
    }

    LaraTestWaterClimbOut(item, coll);
}

int32_t LaraTestWaterClimbOut(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->pos.y_rot != Lara.move_angle) {
        return 0;
    }

    if (coll->coll_type != COLL_FRONT || !(Input & IN_ACTION)
        || ABS(coll->left_floor - coll->right_floor) >= SLOPE_DIF) {
        return 0;
    }

    if (coll->front_ceiling > 0 || coll->mid_ceiling > -STEPUP_HEIGHT) {
        return 0;
    }

    int hdif = coll->front_floor + 700;
    if (hdif < -512 || hdif > 100) {
        return 0;
    }

    PHD_ANGLE angle = item->pos.y_rot;
    if (angle >= -HANG_ANGLE && angle <= HANG_ANGLE) {
        angle = 0;
    } else if (
        angle >= PHD_ONE / 4 - HANG_ANGLE
        && angle <= PHD_ONE / 4 + HANG_ANGLE) {
        angle = PHD_ONE / 4;
    } else if (
        angle >= ((PHD_ONE / 2) - 1) - HANG_ANGLE
        || angle <= -((PHD_ONE / 2) - 1) + HANG_ANGLE) {
        angle = -PHD_ONE / 2;
    } else if (
        angle >= -PHD_ONE / 4 - HANG_ANGLE
        && angle <= -PHD_ONE / 4 + HANG_ANGLE) {
        angle = -PHD_ONE / 4;
    }
    if (angle & ((PHD_ONE / 4) - 1)) {
        return 0;
    }

    item->pos.y += hdif - 5;
    UpdateLaraRoom(item, -LARA_HITE / 2);

    switch (angle) {
    case 0:
        item->pos.z = (item->pos.z & -WALL_L) + WALL_L + LARA_RAD;
        break;
    case PHD_ONE / 4:
        item->pos.x = (item->pos.x & -WALL_L) + WALL_L + LARA_RAD;
        break;
    case -PHD_ONE / 2:
        item->pos.z = (item->pos.z & -WALL_L) - LARA_RAD;
        break;
    case -PHD_ONE / 4:
        item->pos.x = (item->pos.x & -WALL_L) - LARA_RAD;
        break;
    }

    item->anim_number = AA_SURFCLIMB;
    item->frame_number = AF_SURFCLIMB;
    item->current_anim_state = AS_WATEROUT;
    item->goal_anim_state = AS_STOP;
    item->pos.x_rot = 0;
    item->pos.y_rot = angle;
    item->pos.z_rot = 0;
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    Lara.gun_status = LGS_HANDSBUSY;
    Lara.water_status = LWS_ABOVEWATER;
    return 1;
}

void T1MInjectGameLaraSurf()
{
    INJECT(0x004286E0, LaraSurface);

    INJECT(0x004288A0, LaraAsSurfSwim);
    INJECT(0x00428910, LaraAsSurfBack);
    INJECT(0x00428970, LaraAsSurfLeft);
    INJECT(0x004289D0, LaraAsSurfRight);
    INJECT(0x00428A30, LaraAsSurfTread);

    INJECT(0x00428BB0, LaraColSurfTread);
    INJECT(0x00428BD0, LaraColSurfBack);
    INJECT(0x00428C00, LaraColSurfLeft);
    INJECT(0x00428C30, LaraColSurfRight);

    INJECT(0x00428C60, LaraSurfaceCollision);
    INJECT(0x00428D50, LaraTestWaterClimbOut);
}
