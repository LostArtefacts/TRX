#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/data.h"
#include "game/lara.h"

void __cdecl LaraSurface(ITEM_INFO* item, COLL_INFO* coll)
{
    Camera.target_elevation = -22 * ONE_DEGREE;

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

void __cdecl LaraAsSurfSwim(ITEM_INFO* item, COLL_INFO* coll)
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

void Tomb1MInjectGameLaraSurf()
{
    INJECT(0x004286E0, LaraSurface);
    INJECT(0x004288A0, LaraAsSurfSwim);
}
