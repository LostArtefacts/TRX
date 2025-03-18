#include "game/creature.h"
#include "game/objects/common.h"
#include "game/objects/creatures/worker_common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define WORKER_1_HITPOINTS     25
#define WORKER_1_SHOOT_DAMAGE  150
#define WORKER_1_WALK_TURN     (DEG_1 * 3) // = 546
#define WORKER_1_RUN_TURN      (DEG_1 * 5) // = 910
#define WORKER_1_RUN_RANGE     SQUARE(WALL_L * 2) // = 4194304
#define WORKER_1_SHOOT_1_RANGE SQUARE(WALL_L * 3) // = 9437184
// clang-format on

typedef enum {
    // clang-format off
    WORKER_1_STATE_EMPTY   = 0,
    WORKER_1_STATE_WALK    = 1,
    WORKER_1_STATE_STOP    = 2,
    WORKER_1_STATE_WAIT    = 3,
    WORKER_1_STATE_SHOOT_1 = 4,
    WORKER_1_STATE_RUN     = 5,
    WORKER_1_STATE_SHOOT_2 = 6,
    WORKER_1_STATE_DEATH   = 7,
    WORKER_1_STATE_AIM_1   = 8,
    WORKER_1_STATE_AIM_2   = 9,
    WORKER_1_STATE_SHOOT_3 = 10,
    // clang-format on
} WORKER_1_STATE;

typedef enum {
    WORKER_1_ANIM_DEATH = 18,
} WORKER_1_ANIM;

static const BITE m_Worker1Gun = {
    .pos = { .x = 0, .y = 281, .z = 40 },
    .mesh_num = 9,
};

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = WORKER_1_HITPOINTS;
    obj->radius = WORKER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 4)->rot_y = true;
    Object_GetBone(obj, 13)->rot_y = true;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    int16_t head = 0;
    int16_t tilt = 0;
    int16_t neck = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != WORKER_1_STATE_DEATH) {
            Item_SwitchToAnim(item, WORKER_1_ANIM_DEATH, 0);
            item->current_anim_state = WORKER_1_STATE_DEATH;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_BORED);

        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case WORKER_1_STATE_STOP:
            if (info.ahead != 0) {
                neck = info.angle;
            }
            creature->flags = 0;
            creature->maximum_turn = 0;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = WORKER_1_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance >= WORKER_1_SHOOT_1_RANGE
                    && info.zone_num == info.enemy_zone_num) {
                    item->goal_anim_state = WORKER_1_STATE_WALK;
                } else if (Random_GetControl() < 0x4000) {
                    item->goal_anim_state = WORKER_1_STATE_AIM_1;
                } else {
                    item->goal_anim_state = WORKER_1_STATE_AIM_2;
                }
            } else if (creature->mood == MOOD_BORED && info.ahead != 0) {
                item->goal_anim_state = WORKER_1_STATE_WAIT;
            } else if (info.distance > WORKER_1_RUN_RANGE) {
                item->goal_anim_state = WORKER_1_STATE_RUN;
            } else {
                item->goal_anim_state = WORKER_1_STATE_WALK;
            }
            break;

        case WORKER_1_STATE_WAIT:
            if (info.ahead != 0) {
                neck = info.angle;
            }
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = WORKER_1_STATE_SHOOT_1;
            } else if (creature->mood != MOOD_BORED || info.ahead == 0) {
                item->goal_anim_state = WORKER_1_STATE_STOP;
            }
            break;

        case WORKER_1_STATE_WALK:
            if (info.ahead != 0) {
                neck = info.angle;
            }
            creature->flags = 0;
            creature->maximum_turn = WORKER_1_WALK_TURN;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = WORKER_1_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance < WORKER_1_SHOOT_1_RANGE
                    || info.zone_num != info.enemy_zone_num) {
                    item->goal_anim_state = WORKER_1_STATE_STOP;
                } else {
                    item->goal_anim_state = WORKER_1_STATE_SHOOT_2;
                }
            } else if (creature->mood == MOOD_BORED && info.ahead != 0) {
                item->goal_anim_state = WORKER_1_STATE_STOP;
            } else if (info.distance > WORKER_1_RUN_RANGE) {
                item->goal_anim_state = WORKER_1_STATE_RUN;
            }
            break;

        case WORKER_1_STATE_RUN:
            if (info.ahead != 0) {
                neck = info.angle;
            }
            creature->maximum_turn = WORKER_1_RUN_TURN;
            tilt = angle / 2;
            if (creature->mood == MOOD_ESCAPE) {
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = WORKER_1_STATE_WALK;
            } else if (
                creature->mood == MOOD_BORED || creature->mood == MOOD_STALK) {
                item->goal_anim_state = WORKER_1_STATE_WALK;
            }
            break;

        case WORKER_1_STATE_AIM_1:
            if (info.ahead != 0) {
                head = info.angle;
            }
            creature->flags = 0;
            break;

        case WORKER_1_STATE_AIM_2:
            if (info.ahead != 0) {
                head = info.angle;
            }
            creature->flags = 0;
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = WORKER_1_STATE_SHOOT_3;
            }
            break;

        case WORKER_1_STATE_SHOOT_1:
        case WORKER_1_STATE_SHOOT_3:
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (creature->flags == 0) {
                Creature_ShootAtLara(
                    item, &info, &m_Worker1Gun, head, WORKER_1_SHOOT_DAMAGE);
                creature->flags = 1;
            }
            if (item->goal_anim_state != WORKER_1_STATE_STOP
                && (creature->mood == MOOD_ESCAPE
                    || info.distance > WORKER_1_SHOOT_1_RANGE
                    || !Creature_CanTargetEnemy(item, &info))) {
                item->goal_anim_state = WORKER_1_STATE_STOP;
            }
            break;

        case WORKER_1_STATE_SHOOT_2:
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (creature->flags == 0) {
                Creature_ShootAtLara(
                    item, &info, &m_Worker1Gun, head, WORKER_1_SHOOT_DAMAGE);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Neck(item, neck);
    Creature_Animate(item_num, angle, 0);
}

REGISTER_OBJECT(O_WORKER_1, M_Setup)
