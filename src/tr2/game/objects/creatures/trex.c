#include "game/creature.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define TREX_HITPOINTS      100
#define TREX_TOUCH_BITS     0b00110000'00000000
#define TREX_RADIUS         (WALL_L / 3) // = 341
#define TREX_RUN_TURN       (DEG_1 * 4) // = 728
#define TREX_WALK_TURN      (DEG_1 * 2) // = 364
#define TREX_FRONT_ARC      DEG_90
#define TREX_RUN_RANGE      SQUARE(WALL_L * 5) // = 26214400
#define TREX_ATTACK_RANGE   SQUARE(WALL_L * 4) // = 16777216
#define TREX_BITE_RANGE     SQUARE(1500) // = 2250000
#define TREX_TOUCH_DAMAGE   1
#define TREX_TRAMPLE_DAMAGE 10
#define TREX_BITE_DAMAGE    10000
#define TREX_ROAR_CHANCE    0x200
// clang-format on

typedef enum {
    TREX_STATE_EMPTY = 0,
    TREX_STATE_STOP = 1,
    TREX_STATE_WALK = 2,
    TREX_STATE_RUN = 3,
    TREX_STATE_ATTACK_1 = 4,
    TREX_STATE_DEATH = 5,
    TREX_STATE_ROAR = 6,
    TREX_STATE_ATTACK_2 = 7,
    TREX_STATE_KILL = 8,
} TREX_STATE;

typedef enum {
    TREX_ANIM_KILL = 11,
} TREX_ANIM;

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = TREX_HITPOINTS;
    obj->radius = TREX_RADIUS;
    obj->shadow_size = 64;
    obj->pivot_length = 1800;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 10)->rot_y = true;
    Object_GetBone(obj, 11)->rot_y = true;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    int16_t tilt = 0;
    int16_t angle = 0;
    int16_t head = 0;

    if (item->hit_points > 0) {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_ATTACK);

        if (info.ahead) {
            head = info.angle;
        }
        angle = Creature_Turn(item, creature->maximum_turn);

        if (item->touch_bits != 0) {
            g_LaraItem->hit_points -= item->current_anim_state == TREX_STATE_RUN
                ? TREX_TRAMPLE_DAMAGE
                : TREX_TOUCH_DAMAGE;
        }

        creature->flags = creature->mood != MOOD_ESCAPE && !info.ahead
            && info.enemy_facing > -TREX_FRONT_ARC
            && info.enemy_facing < TREX_FRONT_ARC;

        if (creature->flags == 0 && info.distance > TREX_BITE_RANGE
            && info.distance < TREX_ATTACK_RANGE && info.bite) {
            creature->flags = 1;
        }

        switch (item->current_anim_state) {
        case TREX_STATE_STOP:
            if (item->required_anim_state != TREX_STATE_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.distance < TREX_BITE_RANGE && info.bite) {
                item->goal_anim_state = TREX_STATE_ATTACK_2;
            } else if (creature->mood == MOOD_BORED || creature->flags != 0) {
                item->goal_anim_state = TREX_STATE_WALK;
            } else {
                item->goal_anim_state = TREX_STATE_RUN;
            }
            break;

        case TREX_STATE_WALK:
            creature->maximum_turn = TREX_WALK_TURN;
            if (creature->mood != MOOD_BORED && creature->flags == 0) {
                item->goal_anim_state = TREX_STATE_STOP;
            } else if (info.ahead && Random_GetControl() < TREX_ROAR_CHANCE) {
                item->required_anim_state = TREX_STATE_ROAR;
                item->goal_anim_state = TREX_STATE_STOP;
            }
            break;

        case TREX_STATE_RUN:
            creature->maximum_turn = TREX_RUN_TURN;
            if (info.distance < TREX_RUN_RANGE && info.bite) {
                item->goal_anim_state = TREX_STATE_STOP;
            } else if (creature->flags != 0) {
                item->goal_anim_state = TREX_STATE_STOP;
            } else if (
                creature->mood != MOOD_ESCAPE && info.ahead
                && Random_GetControl() < TREX_ROAR_CHANCE) {
                item->required_anim_state = TREX_STATE_ROAR;
                item->goal_anim_state = TREX_STATE_STOP;
            } else if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = TREX_STATE_STOP;
            }
            break;

        case TREX_STATE_ATTACK_2:
            if ((item->touch_bits & TREX_TOUCH_BITS) != 0) {
                Lara_TakeDamage(TREX_BITE_DAMAGE, true);
                Creature_Kill(
                    item, TREX_ANIM_KILL, TREX_STATE_KILL, LA_EXTRA_TREX_KILL);
                return;
            }
            item->required_anim_state = TREX_STATE_WALK;
            break;

        default:
            break;
        }
    } else if (item->current_anim_state == TREX_STATE_STOP) {
        item->goal_anim_state = TREX_STATE_DEATH;
    } else {
        item->goal_anim_state = TREX_STATE_STOP;
    }

    Creature_Head(item, head / 2);
    creature->neck_rotation = creature->head_rotation;

    Creature_Animate(item_num, angle, tilt);
}

REGISTER_OBJECT(O_DINO, M_Setup)
