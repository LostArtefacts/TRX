#include "game/collide.h"
#include "game/const.h"
#include "game/control.h"
#include "game/data.h"
#include "game/effects.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/shell.h"
#include "mod.h"
#include "util.h"

void TR1MLookLeftRight()
{
    Camera.type = CAM_LOOK;
    if (Input & IN_LEFT) {
        Input -= IN_LEFT;
        if (Lara.head_y_rot > -MAX_HEAD_ROTATION) {
            Lara.head_y_rot -= HEAD_TURN / 2;
        }
    } else if (Input & IN_RIGHT) {
        Input -= IN_RIGHT;
        if (Lara.head_y_rot < MAX_HEAD_ROTATION) {
            Lara.head_y_rot += HEAD_TURN / 2;
        }
    }
    if (Lara.gun_status != LGS_HANDSBUSY) {
        Lara.torso_y_rot = Lara.head_y_rot;
    }
}

void TR1MResetLook()
{
    if (Camera.type == CAM_LOOK) {
        return;
    }
    if (Lara.head_x_rot <= -HEAD_TURN / 2 || Lara.head_x_rot >= HEAD_TURN / 2) {
        Lara.head_x_rot = Lara.head_x_rot / -8 + Lara.head_x_rot;
    } else {
        Lara.head_x_rot = 0;
    }
    Lara.torso_x_rot = Lara.head_x_rot;

    if (Lara.head_y_rot <= -HEAD_TURN / 2 || Lara.head_y_rot >= HEAD_TURN / 2) {
        Lara.head_y_rot += Lara.head_y_rot / -8;
    } else {
        Lara.head_y_rot = 0;
    }
    Lara.torso_y_rot = Lara.head_y_rot;
}

void __cdecl LaraAboveWater(ITEM_INFO* item, COLL_INFO* coll)
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

    if (TR1MConfig.enable_enhanced_look) {
        if (Input & IN_LOOK) {
            TR1MLookLeftRight();
        } else {
            TR1MResetLook();
        }
    }

    LaraControlRoutines[item->current_anim_state](item, coll);

    if (Camera.type != CAM_LOOK) {
        if (Lara.head_x_rot > -HEAD_TURN / 2
            && Lara.head_x_rot < HEAD_TURN / 2) {
            Lara.head_x_rot = 0;
        } else {
            Lara.head_x_rot -= Lara.head_x_rot / 8;
        }
        Lara.torso_x_rot = Lara.head_x_rot;

        if (Lara.head_y_rot > -HEAD_TURN / 2
            && Lara.head_y_rot < HEAD_TURN / 2) {
            Lara.head_y_rot = 0;
        } else {
            Lara.head_y_rot -= Lara.head_y_rot / 8;
        }
        Lara.torso_y_rot = Lara.head_y_rot;
    }

    if (item->pos.z_rot >= -LARA_LEAN_UNDO
        && item->pos.z_rot <= LARA_LEAN_UNDO) {
        item->pos.z_rot = 0;
    } else if (item->pos.z_rot < -LARA_LEAN_UNDO) {
        item->pos.z_rot += LARA_LEAN_UNDO;
    } else {
        item->pos.z_rot -= LARA_LEAN_UNDO;
    }

    if (Lara.turn_rate >= -LARA_TURN_UNDO && Lara.turn_rate <= LARA_TURN_UNDO) {
        Lara.turn_rate = 0;
    } else if (Lara.turn_rate < -LARA_TURN_UNDO) {
        Lara.turn_rate += LARA_TURN_UNDO;
    } else {
        Lara.turn_rate -= LARA_TURN_UNDO;
    }
    item->pos.y_rot += Lara.turn_rate;

    AnimateLara(item);
    LaraBaddieCollision(item, coll);
    LaraCollisionRoutines[item->current_anim_state](item, coll);
    UpdateLaraRoom(item, -LARA_HITE / 2);
    LaraGun();
    TestTriggers(coll->trigger, 0);
}

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
        Camera.type = CAM_LOOK;
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
    if (Camera.type == CAM_LOOK) {
        Camera.type = CAM_CHASE;
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
        if ((Input & IN_ACTION) && Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = AS_REACH;
        }
        if ((Input & IN_SLOW) && Lara.gun_status == LGS_ARMLESS) {
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
    if (item->hit_points <= 0
        || (TR1MConfig.enable_enhanced_look && (Input & IN_LOOK))) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    Lara.turn_rate += LARA_TURN_RATE;
    if (Lara.gun_status == LGS_READY) {
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
    if (item->hit_points <= 0
        || (TR1MConfig.enable_enhanced_look && (Input & IN_LOOK))) {
        item->goal_anim_state = AS_STOP;
        return;
    }

    Lara.turn_rate -= LARA_TURN_RATE;
    if (Lara.gun_status == LGS_READY) {
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
    if (item->hit_points <= 0
        || (TR1MConfig.enable_enhanced_look && (Input & IN_LOOK))) {
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
    if ((Input & IN_ACTION) && Lara.gun_status == LGS_ARMLESS) {
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

void __cdecl LaraAsRoll(ITEM_INFO* item, COLL_INFO* coll)
{
}

void __cdecl LaraAsRoll2(ITEM_INFO* item, COLL_INFO* coll)
{
}

void __cdecl LaraAsSpecial(ITEM_INFO* item, COLL_INFO* coll)
{
    Camera.flags = FOLLOW_CENTRE;
    Camera.target_angle = 170 * ONE_DEGREE;
    Camera.target_elevation = -25 * ONE_DEGREE;
}

void __cdecl LaraAsUseMidas(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    ItemSparkle(item, (1 << LM_HAND_L) | (1 << LM_HAND_R));
}

void __cdecl LaraAsDieMidas(ITEM_INFO* item, COLL_INFO* coll)
{
    item->gravity_status = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    int frm = item->frame_number - Anims[item->anim_number].frame_base;
    switch (frm) {
    case 5:
        Lara.mesh_effects |= (1 << LM_FOOT_L);
        Lara.mesh_effects |= (1 << LM_FOOT_R);
        Lara.mesh_ptrs[LM_FOOT_L] =
            Meshes[Objects[O_LARA_EXTRA].mesh_index + LM_FOOT_L];
        Lara.mesh_ptrs[LM_FOOT_R] =
            Meshes[Objects[O_LARA_EXTRA].mesh_index + LM_FOOT_R];
        break;

    case 70:
        Lara.mesh_effects |= (1 << LM_CALF_L);
        Lara.mesh_ptrs[LM_CALF_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_CALF_L];
        break;

    case 90:
        Lara.mesh_effects |= (1 << LM_THIGH_L);
        Lara.mesh_ptrs[LM_THIGH_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_THIGH_L];
        break;

    case 100:
        Lara.mesh_effects |= (1 << LM_CALF_R);
        Lara.mesh_ptrs[LM_CALF_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_CALF_R];
        break;

    case 120:
        Lara.mesh_effects |= (1 << LM_HIPS);
        Lara.mesh_effects |= (1 << LM_THIGH_R);
        Lara.mesh_ptrs[LM_HIPS] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HIPS];
        Lara.mesh_ptrs[LM_THIGH_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_THIGH_R];
        break;

    case 135:
        Lara.mesh_effects |= (1 << LM_TORSO);
        Lara.mesh_ptrs[LM_TORSO] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_TORSO];
        break;

    case 150:
        Lara.mesh_effects |= (1 << LM_UARM_L);
        Lara.mesh_ptrs[LM_UARM_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_UARM_L];
        break;

    case 163:
        Lara.mesh_effects |= (1 << LM_LARM_L);
        Lara.mesh_ptrs[LM_LARM_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_LARM_L];
        break;

    case 174:
        Lara.mesh_effects |= (1 << LM_HAND_L);
        Lara.mesh_ptrs[LM_HAND_L] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HAND_L];
        break;

    case 186:
        Lara.mesh_effects |= (1 << LM_UARM_R);
        Lara.mesh_ptrs[LM_UARM_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_UARM_R];
        break;

    case 195:
        Lara.mesh_effects |= (1 << LM_LARM_R);
        Lara.mesh_ptrs[LM_LARM_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_LARM_R];
        break;

    case 218:
        Lara.mesh_effects |= (1 << LM_HAND_R);
        Lara.mesh_ptrs[LM_HAND_R] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HAND_R];
        break;

    case 225:
        Lara.mesh_effects |= (1 << LM_HEAD);
        Lara.mesh_ptrs[LM_HEAD] =
            Meshes[(&Objects[O_LARA_EXTRA])->mesh_index + LM_HEAD];
        break;
    }

    ItemSparkle(item, Lara.mesh_effects);
}

void __cdecl LaraAsSwanDive(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = AS_FASTDIVE;
    }
}

void __cdecl LaraAsFastDive(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    item->speed = (item->speed * 95) / 100;
}

void __cdecl LaraAsGymnast(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void __cdecl LaraAsWaterOut(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Camera.flags = FOLLOW_CENTRE;
}

void __cdecl LaraColWalk(ITEM_INFO* item, COLL_INFO* coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->lava_is_pit = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }
    if (TestLaraVault(item, coll)) {
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        if (item->frame_number >= 29 && item->frame_number <= 47) {
            item->anim_number = AA_STOP_RIGHT;
            item->frame_number = AF_STOP_RIGHT;
        } else if (
            (item->frame_number >= 22 && item->frame_number <= 28)
            || (item->frame_number >= 48 && item->frame_number <= 57)) {
            item->anim_number = AA_STOP_LEFT;
            item->frame_number = AF_STOP_LEFT;
        } else {
            item->anim_number = AA_STOP;
            item->frame_number = AF_STOP;
        }
    }

    if (coll->mid_floor > STEPUP_HEIGHT) {
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN;
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (coll->mid_floor > STEP_L / 2) {
        if (item->frame_number >= 28 && item->frame_number <= 45) {
            item->anim_number = AA_WALKSTEPD_RIGHT;
            item->frame_number = AF_WALKSTEPD_RIGHT;
        } else {
            item->anim_number = AA_WALKSTEPD_LEFT;
            item->frame_number = AF_WALKSTEPD_LEFT;
        }
    }

    if (coll->mid_floor >= -STEPUP_HEIGHT && coll->mid_floor < -STEP_L / 2) {
        if (item->frame_number >= 27 && item->frame_number <= 44) {
            item->anim_number = AA_WALKSTEPUP_RIGHT;
            item->frame_number = AF_WALKSTEPUP_RIGHT;
        } else {
            item->anim_number = AA_WALKSTEPUP_LEFT;
            item->frame_number = AF_WALKSTEPUP_LEFT;
        }
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void __cdecl LaraColRun(ITEM_INFO* item, COLL_INFO* coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }
    if (TestLaraVault(item, coll)) {
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        item->pos.z_rot = 0;

        if (coll->front_type == HT_WALL
            && coll->front_floor < -(STEP_L * 5) / 2) {
            item->current_anim_state = AS_SPLAT;
            if (item->frame_number >= 0 && item->frame_number <= 9) {
                item->anim_number = AA_HITWALLLEFT;
                item->frame_number = AF_HITWALLLEFT;
                return;
            }
            if (item->frame_number >= 10 && item->frame_number <= 21) {
                item->anim_number = AA_HITWALLRIGHT;
                item->frame_number = AF_HITWALLRIGHT;
                return;
            }
        }
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP;
    }

    if (coll->mid_floor > STEPUP_HEIGHT) {
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN;
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (coll->mid_floor >= -STEPUP_HEIGHT && coll->mid_floor < -STEP_L / 2) {
        if (item->frame_number >= 3 && item->frame_number <= 14) {
            item->anim_number = AA_RUNSTEPUP_LEFT;
            item->frame_number = AF_RUNSTEPUP_LEFT;
        } else {
            item->anim_number = AA_RUNSTEPUP_RIGHT;
            item->frame_number = AF_RUNSTEPUP_RIGHT;
        }
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor >= 50) {
        item->pos.y += 50;
    } else {
        item->pos.y += coll->mid_floor;
    }
}

void __cdecl LaraColStop(ITEM_INFO* item, COLL_INFO* coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (coll->mid_floor > 100) {
        item->anim_number = AA_FALLDOWN;
        item->frame_number = AF_FALLDOWN;
        item->current_anim_state = AS_FORWARDJUMP;
        item->goal_anim_state = AS_FORWARDJUMP;
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (TestLaraSlide(item, coll)) {
        return;
    }

    ShiftItem(item, coll);
    item->pos.y += coll->mid_floor;
}

void __cdecl LaraColForwardJump(ITEM_INFO* item, COLL_INFO* coll)
{
    Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    GetLaraCollisionInfo(item, coll);
    LaraDeflectEdgeJump(item, coll);
    if (coll->mid_floor <= 0 && item->fall_speed > 0) {
        if (LaraLandedBad(item, coll)) {
            item->goal_anim_state = AS_DEATH;
        } else if (Input & IN_FORWARD && !(Input & IN_SLOW)) {
            item->goal_anim_state = AS_RUN;
        } else {
            item->goal_anim_state = AS_STOP;
        }
        item->pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->speed = 0;
        AnimateLara(item);
    }
}

void __cdecl LaraColFastBack(ITEM_INFO* item, COLL_INFO* coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;
    Lara.move_angle = item->pos.y_rot + -32768;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    GetLaraCollisionInfo(item, coll);

    if (LaraHitCeiling(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->anim_number = AA_FALLBACK;
        item->frame_number = AF_FALLBACK;
        item->current_anim_state = AS_FALLBACK;
        item->goal_anim_state = AS_FALLBACK;
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (LaraDeflectEdge(item, coll)) {
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP;
    }

    item->pos.y += coll->mid_floor;
}

void __cdecl GetLaraCollisionInfo(ITEM_INFO* item, COLL_INFO* coll)
{
    coll->facing = Lara.move_angle;
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_number,
        LARA_HITE);
}

int32_t __cdecl LaraHitCeiling(ITEM_INFO* item, COLL_INFO* coll)
{
    if (coll->coll_type == COLL_TOP || coll->coll_type == COLL_CLAMP) {
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        item->goal_anim_state = AS_STOP;
        item->current_anim_state = AS_STOP;
        item->anim_number = AA_STOP;
        item->frame_number = AF_STOP;
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->speed = 0;
        return 1;
    }
    return 0;
}

int32_t __cdecl LaraDeflectEdge(ITEM_INFO* item, COLL_INFO* coll)
{
    if (coll->coll_type == COLL_FRONT || coll->coll_type == COLL_TOPFRONT) {
        ShiftItem(item, coll);
        item->goal_anim_state = AS_STOP;
        item->current_anim_state = AS_STOP;
        item->gravity_status = 0;
        item->speed = 0;
        return 1;
    }

    if (coll->coll_type == COLL_LEFT) {
        ShiftItem(item, coll);
        item->pos.y_rot += LARA_DEF_ADD_EDGE;
    } else if (coll->coll_type == COLL_RIGHT) {
        ShiftItem(item, coll);
        item->pos.y_rot -= LARA_DEF_ADD_EDGE;
    }
    return 0;
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
    INJECT(0x00422480, LaraAboveWater);
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
    INJECT(0x004232B0, LaraAsSpecial);
    INJECT(0x004232D0, LaraAsUseMidas);
    INJECT(0x004232F0, LaraAsDieMidas);
    INJECT(0x00423720, LaraAsSwanDive);
    INJECT(0x00423750, LaraAsFastDive);
    INJECT(0x004237C0, LaraColWalk);
    INJECT(0x004239F0, LaraColRun);
    INJECT(0x00423C00, LaraColStop);
    INJECT(0x00423D00, LaraColForwardJump);
    INJECT(0x00423DD0, LaraColFastBack);
    INJECT(0x004237A0, LaraAsWaterOut);
}
