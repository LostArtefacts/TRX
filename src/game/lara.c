#include "game/const.h"
#include "game/data.h"
#include "game/lara.h"
#include "game/lot.h"
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

void TR1MInjectLara()
{
    INJECT(0x004225F0, LaraAsWalk);
    INJECT(0x00422670, LaraAsRun);
    INJECT(0x00422760, LaraAsStop);
}
