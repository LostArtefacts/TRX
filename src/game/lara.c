#include "game/const.h"
#include "game/control.h"
#include "game/data.h"
#include "game/effects.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/shell.h"
#include "mod.h"
#include "util.h"

void __cdecl LaraAsWalk(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_SLOW_TURN) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_SLOW_TURN) {
            Lara.turn_rate = LARA_SLOW_TURN;
        }
    }

    if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else {
        item->goal_anim_state = AS_STOP;
    }
}

void __cdecl LaraAsRun(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_DEATH;
        return;
    }

    if (Input & IN_ROLL) {
        item->anim_number = AA_ROLL;
        item->frame_number = AF_ROLL;
        item->current_anim_state = AS_ROLL;
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_FAST_TURN) {
            Lara.turn_rate = -LARA_FAST_TURN;
        }
        item->pos.z_rot -= LARA_LEAN_RATE;
        if (item->pos.z_rot < -LARA_LEAN_MAX) {
            item->pos.z_rot = -LARA_LEAN_MAX;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_FAST_TURN) {
            Lara.turn_rate = LARA_FAST_TURN;
        }
        item->pos.z_rot += LARA_LEAN_RATE;
        if (item->pos.z_rot > LARA_LEAN_MAX) {
            item->pos.z_rot = LARA_LEAN_MAX;
        }
    }

    if ((Input & IN_JUMP) && !item->gravity_status) {
        item->goal_anim_state = AS_FORWARDJUMP;
    } else if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else {
        item->goal_anim_state = AS_STOP;
    }
}

void __cdecl LaraAsStop(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_DEATH;
        return;
    }

    if (Input & IN_ROLL) {
        item->anim_number = AA_ROLL;
        item->frame_number = AF_ROLL;
        item->current_anim_state = AS_ROLL;
        item->goal_anim_state = AS_STOP;
        return;
    }

    item->goal_anim_state = AS_STOP;
    if (Input & IN_LOOK) {
        Camera.type = LOOK_CAMERA;
        if ((Input & IN_LEFT) && Lara.head_y_rot > -MAX_HEAD_ROTATION) {
            Lara.head_y_rot -= HEAD_TURN / 2;
        } else if ((Input & IN_RIGHT) && Lara.head_y_rot < MAX_HEAD_ROTATION) {
            Lara.head_y_rot += HEAD_TURN / 2;
        }
        Lara.torso_y_rot = Lara.head_y_rot;

        if ((Input & IN_FORWARD) && Lara.head_x_rot > MIN_HEAD_TILT) {
            Lara.head_x_rot -= HEAD_TURN / 2;
        } else if ((Input & IN_BACK) && Lara.head_x_rot < MAX_HEAD_TILT) {
            Lara.head_x_rot += HEAD_TURN / 2;
        }
        Lara.torso_x_rot = Lara.head_x_rot;
        return;
    }
    if (Camera.type == LOOK_CAMERA) {
        Camera.type = CHASE_CAMERA;
    }

    if (Input & IN_STEPL) {
        item->goal_anim_state = AS_STEPLEFT;
    } else if (Input & IN_STEPR) {
        item->goal_anim_state = AS_STEPRIGHT;
    }

    if (Input & IN_LEFT) {
        item->goal_anim_state = AS_TURN_L;
    } else if (Input & IN_RIGHT) {
        item->goal_anim_state = AS_TURN_R;
    }

    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_COMPRESS;
    } else if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            LaraAsWalk(item, coll);
        } else {
            LaraAsRun(item, coll);
        }
    } else if (Input & IN_BACK) {
        if (Input & IN_SLOW) {
            LaraAsBack(item, coll);
        } else {
            item->goal_anim_state = AS_FASTBACK;
        }
    }
}

void __cdecl LaraAsForwardJump(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->goal_anim_state == AS_SWANDIVE
        || item->goal_anim_state == AS_REACH) {
        item->goal_anim_state = AS_FORWARDJUMP;
    }
    if (item->goal_anim_state != AS_DEATH && item->goal_anim_state != AS_STOP) {
        if ((Input & IN_ACTION) && Lara.gun_status == LG_ARMLESS) {
            item->goal_anim_state = AS_REACH;
        }
        if ((Input & IN_SLOW) && Lara.gun_status == LG_ARMLESS) {
            item->goal_anim_state = AS_SWANDIVE;
        }
        if (item->fall_speed > LARA_FASTFALL_SPEED) {
            item->goal_anim_state = AS_FASTFALL;
        }
    }
    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_JUMP_TURN) {
            Lara.turn_rate = -LARA_JUMP_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_JUMP_TURN) {
            Lara.turn_rate = LARA_JUMP_TURN;
        }
    }
}

void __cdecl LaraAsPose(ITEM_INFO* item, COLL_INFO* coll)
{
}

void __cdecl LaraAsFastBack(ITEM_INFO* item, COLL_INFO* coll)
{
    item->goal_anim_state = AS_STOP;
    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_MED_TURN) {
            Lara.turn_rate = -LARA_MED_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_MED_TURN) {
            Lara.turn_rate = LARA_MED_TURN;
        }
    }
}

void __cdecl LaraAsTurnR(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    Lara.turn_rate += LARA_TURN_RATE;
    if (Lara.gun_status == LG_READY) {
        item->goal_anim_state = AS_FASTTURN;
    } else if (Lara.turn_rate > LARA_SLOW_TURN) {
        if (Input & IN_SLOW) {
            Lara.turn_rate = LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = AS_FASTTURN;
        }
    }

    if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else if (!(Input & IN_RIGHT)) {
        item->goal_anim_state = AS_STOP;
    }
}

void __cdecl LaraAsTurnL(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    Lara.turn_rate -= LARA_TURN_RATE;
    if (Lara.gun_status == LG_READY) {
        item->goal_anim_state = AS_FASTTURN;
    } else if (Lara.turn_rate < -LARA_SLOW_TURN) {
        if (Input & IN_SLOW) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = AS_FASTTURN;
        }
    }

    if (Input & IN_FORWARD) {
        if (Input & IN_SLOW) {
            item->goal_anim_state = AS_WALK;
        } else {
            item->goal_anim_state = AS_RUN;
        }
    } else if (!(Input & IN_LEFT)) {
        item->goal_anim_state = AS_STOP;
    }
}

void __cdecl LaraAsDeath(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void __cdecl LaraAsFastFall(ITEM_INFO* item, COLL_INFO* coll)
{
    item->speed = (item->speed * 95) / 100;
    if (item->fall_speed >= DAMAGE_START + DAMAGE_LENGTH) {
        SoundEffect(30, &item->pos, 0);
    }
}

void __cdecl LaraAsHang(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = CAM_A_HANG;
    Camera.target_elevation = CAM_E_HANG;
    if (Input & (IN_LEFT | IN_STEPL)) {
        item->goal_anim_state = AS_HANGLEFT;
    } else if (Input & (IN_RIGHT | IN_STEPR)) {
        item->goal_anim_state = AS_HANGRIGHT;
    }
}

void __cdecl LaraAsReach(ITEM_INFO* item, COLL_INFO* coll)
{
    Camera.target_angle = 85 * ONE_DEGREE;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void __cdecl LaraAsSplat(ITEM_INFO* item, COLL_INFO* coll)
{
}

void __cdecl LaraAsLand(ITEM_INFO* item, COLL_INFO* coll)
{
}

void __cdecl LaraAsCompress(ITEM_INFO* item, COLL_INFO* coll)
{
    if ((Input & IN_FORWARD)
        && LaraFloorFront(item, item->pos.y_rot, 256) >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_FORWARDJUMP;
        Lara.move_angle = item->pos.y_rot;
    } else if (
        (Input & IN_LEFT)
        && LaraFloorFront(item, item->pos.y_rot - 0x4000, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_LEFTJUMP;
        Lara.move_angle = item->pos.y_rot - 0x4000;
    } else if (
        (Input & IN_RIGHT)
        && LaraFloorFront(item, item->pos.y_rot + 0x4000, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_RIGHTJUMP;
        Lara.move_angle = item->pos.y_rot + 0x4000;
    } else if (
        (Input & IN_BACK)
        && LaraFloorFront(item, item->pos.y_rot - 0x8000, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = AS_BACKJUMP;
        Lara.move_angle = item->pos.y_rot - 0x8000;
    }

    if (item->fall_speed > LARA_FASTFALL_SPEED)
        item->goal_anim_state = AS_FASTFALL;
}

void __cdecl LaraAsBack(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if ((Input & IN_BACK) && (Input & IN_SLOW)) {
        item->goal_anim_state = AS_BACK;
    } else {
        item->goal_anim_state = AS_STOP;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_SLOW_TURN) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_SLOW_TURN) {
            Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void __cdecl LaraAsFastTurn(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (Lara.turn_rate >= 0) {
        Lara.turn_rate = LARA_FAST_TURN;
        if (!(Input & IN_RIGHT)) {
            item->goal_anim_state = AS_STOP;
        }
    } else {
        Lara.turn_rate = -LARA_FAST_TURN;
        if (!(Input & IN_LEFT)) {
            item->goal_anim_state = AS_STOP;
        }
    }
}

void __cdecl LaraAsStepRight(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (!(Input & IN_STEPR)) {
        item->goal_anim_state = AS_STOP;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_SLOW_TURN) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_SLOW_TURN) {
            Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void __cdecl LaraAsStepLeft(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    if (!(Input & IN_STEPL)) {
        item->goal_anim_state = AS_STOP;
    }

    if (Input & IN_LEFT) {
        Lara.turn_rate -= LARA_TURN_RATE;
        if (Lara.turn_rate < -LARA_SLOW_TURN) {
            Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (Input & IN_RIGHT) {
        Lara.turn_rate += LARA_TURN_RATE;
        if (Lara.turn_rate > LARA_SLOW_TURN) {
            Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void __cdecl LaraAsSlide(ITEM_INFO* item, COLL_INFO* coll)
{
    Camera.flags = NO_CHUNKY;
    Camera.target_elevation = -45 * ONE_DEGREE;
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_FORWARDJUMP;
    }
}

void __cdecl LaraAsBackJump(ITEM_INFO* item, COLL_INFO* coll)
{
    Camera.target_angle = ONE_DEGREE * 135;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void __cdecl LaraAsRightJump(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void __cdecl LaraAsLeftJump(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void __cdecl LaraAsUpJump(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
}

void __cdecl LaraAsFallBack(ITEM_INFO* item, COLL_INFO* coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTFALL;
    }
    if ((Input & IN_ACTION) && Lara.gun_status == LG_ARMLESS) {
        item->goal_anim_state = AS_REACH;
    }
}

void __cdecl LaraAsHangLeft(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = CAM_A_HANG;
    Camera.target_elevation = CAM_E_HANG;
    if (!(Input & IN_LEFT) && !(Input & IN_STEPL)) {
        item->goal_anim_state = AS_HANG;
    }
}

void __cdecl LaraAsHangRight(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = CAM_A_HANG;
    Camera.target_elevation = CAM_E_HANG;
    if (!(Input & IN_RIGHT) && !(Input & IN_STEPR)) {
        item->goal_anim_state = AS_HANG;
    }
}

void __cdecl LaraAsSlideBack(ITEM_INFO* item, COLL_INFO* coll)
{
    if (Input & IN_JUMP) {
        item->goal_anim_state = AS_BACKJUMP;
    }
}

void __cdecl LaraAsPushBlock(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.flags = FOLLOW_CENTRE;
    Camera.target_angle = 35 * ONE_DEGREE;
    Camera.target_elevation = -25 * ONE_DEGREE;
}

void __cdecl LaraAsPullBlock(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.flags = FOLLOW_CENTRE;
    Camera.target_angle = 35 * ONE_DEGREE;
    Camera.target_elevation = -25 * ONE_DEGREE;
}

void __cdecl LaraAsPPReady(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = 75 * ONE_DEGREE;
    if (!(Input & IN_ACTION)) {
        item->goal_anim_state = AS_STOP;
    }
}

void __cdecl LaraAsPickup(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = -130 * ONE_DEGREE;
    Camera.target_elevation = -15 * ONE_DEGREE;
    Camera.target_distance = WALL_L;
}

void __cdecl LaraAsSwitchOn(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = 80 * ONE_DEGREE;
    Camera.target_elevation = -25 * ONE_DEGREE;
    Camera.target_distance = WALL_L;
}

void __cdecl LaraAsSwitchOff(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = 80 * ONE_DEGREE;
    Camera.target_elevation = -25 * ONE_DEGREE;
    Camera.target_distance = WALL_L;
}

void __cdecl LaraAsUseKey(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = -80 * ONE_DEGREE;
    Camera.target_elevation = -25 * ONE_DEGREE;
    Camera.target_distance = WALL_L;
}

void __cdecl LaraAsUsePuzzle(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.target_angle = -80 * ONE_DEGREE;
    Camera.target_elevation = -25 * ONE_DEGREE;
    Camera.target_distance = WALL_L;
}

int16_t __cdecl LaraFloorFront(ITEM_INFO* item, PHD_ANGLE ang, int32_t dist)
{
    int32_t x = item->pos.x + ((phd_sin(ang) * dist) >> W2V_SHIFT);
    int32_t y = item->pos.y - LARA_HITE;
    int32_t z = item->pos.z + ((phd_cos(ang) * dist) >> W2V_SHIFT);
    int16_t room = item->room_number;
    FLOOR_INFO* floor = GetFloor(x, y, z, &room);
    int32_t height = GetHeight(floor, x, y, z);
    if (height != NO_HEIGHT)
        height -= item->pos.y;
    return height;
}

void TR1MInjectLara()
{
    INJECT(0x004225F0, LaraAsWalk);
    INJECT(0x00422670, LaraAsRun);
    INJECT(0x00422760, LaraAsStop);
    INJECT(0x00422970, LaraAsForwardJump);
    INJECT(0x00422A30, LaraAsFastBack);
    INJECT(0x00422A90, LaraAsTurnR);
    INJECT(0x00422B10, LaraAsTurnL);
    INJECT(0x00422B90, LaraAsFastFall);
    INJECT(0x00422BD0, LaraAsHang);
    INJECT(0x00422C20, LaraAsReach);
    INJECT(0x00422C40, LaraAsCompress);
    INJECT(0x00422EB0, LaraAsBack);
    INJECT(0x00422F30, LaraAsFastTurn);
    INJECT(0x00422F80, LaraAsStepRight);
    INJECT(0x00423000, LaraAsStepLeft);
    INJECT(0x00423080, LaraAsSlide);
    INJECT(0x004230B0, LaraAsBackJump);
    INJECT(0x004230D0, LaraAsRightJump);
    INJECT(0x004230F0, LaraAsFallBack);
    INJECT(0x00423120, LaraAsHangLeft);
    INJECT(0x00423160, LaraAsHangRight);
    INJECT(0x004231A0, LaraAsSlideBack);
    INJECT(0x004231C0, LaraAsPushBlock);
    INJECT(0x004231F0, LaraAsPPReady);
    INJECT(0x00423220, LaraAsPickup);
    INJECT(0x00423250, LaraAsSwitchOn);
    INJECT(0x00423280, LaraAsUseKey);
}
