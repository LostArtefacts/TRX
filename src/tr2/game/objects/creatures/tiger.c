#include "game/creature.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define TIGER_HITPOINTS      20
#define TIGER_TOUCH_BITS     0b00000111'11111101'11000000'00000000
#define TIGER_RADIUS         (WALL_L / 3) // = 341
#define TIGER_WALK_TURN      (DEG_1 * 3) // = 546
#define TIGER_RUN_TURN       (DEG_1 * 6) // = 1092
#define TIGER_ATTACK_1_RANGE SQUARE(WALL_L / 3) // = 116281
#define TIGER_ATTACK_2_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define TIGER_ATTACK_3_RANGE SQUARE(WALL_L) // = 1048576
#define TIGER_BITE_DAMAGE    100
#define TIGER_ROAR_CHANCE    96
// clang-format on

typedef enum {
    // clang-format off
    TIGER_STATE_EMPTY    = 0,
    TIGER_STATE_STOP     = 1,
    TIGER_STATE_WALK     = 2,
    TIGER_STATE_RUN      = 3,
    TIGER_STATE_WAIT     = 4,
    TIGER_STATE_ROAR     = 5,
    TIGER_STATE_ATTACK_1 = 6,
    TIGER_STATE_ATTACK_2 = 7,
    TIGER_STATE_ATTACK_3 = 8,
    TIGER_STATE_DEATH    = 9,
    // clang-format on
} TIGER_STATE;

typedef enum {
    TIGER_ANIM_DEATH = 11,
} TIGER_ANIM;

static const BITE m_TigerBite = {
    .pos = { .x = 19, .y = -13, .z = 3 },
    .mesh_num = 26,
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

    obj->hit_points = TIGER_HITPOINTS;
    obj->radius = TIGER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 200;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 21)->rot_y = true;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points > 0) {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_ATTACK);

        if (info.ahead != 0) {
            head = info.angle;
        }
        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case TIGER_STATE_STOP:
            creature->maximum_turn = 0;
            creature->flags = 0;

            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = TIGER_STATE_RUN;
            } else if (creature->mood == MOOD_BORED) {
                if (Random_GetControl() < TIGER_ROAR_CHANCE) {
                    item->goal_anim_state = TIGER_STATE_ROAR;
                }
                item->goal_anim_state = TIGER_STATE_WALK;
            } else if (
                info.ahead != 0 && info.distance < TIGER_ATTACK_1_RANGE) {
                item->goal_anim_state = TIGER_STATE_ATTACK_1;
            } else if (
                info.ahead != 0 && info.distance < TIGER_ATTACK_3_RANGE) {
                creature->maximum_turn = TIGER_WALK_TURN;
                item->goal_anim_state = TIGER_STATE_ATTACK_3;
            } else if (item->required_anim_state != TIGER_STATE_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (
                creature->mood != MOOD_ATTACK
                && Random_GetControl() < TIGER_ROAR_CHANCE) {
                item->goal_anim_state = TIGER_STATE_ROAR;
            } else {
                item->goal_anim_state = TIGER_STATE_RUN;
            }
            break;

        case TIGER_STATE_WALK:
            creature->maximum_turn = TIGER_WALK_TURN;
            if (Random_GetControl() < TIGER_ROAR_CHANCE) {
                item->goal_anim_state = TIGER_STATE_STOP;
                item->required_anim_state = TIGER_STATE_ROAR;
            }
            break;

        case TIGER_STATE_RUN:
            creature->maximum_turn = TIGER_RUN_TURN;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = TIGER_STATE_STOP;
            } else if (creature->flags != 0) {
                if (info.ahead != 0) {
                    item->goal_anim_state = TIGER_STATE_STOP;
                }
            } else if (
                info.ahead != 0 && info.distance < TIGER_ATTACK_2_RANGE) {
                if (g_LaraItem->speed == 0) {
                    item->goal_anim_state = TIGER_STATE_STOP;
                } else {
                    item->goal_anim_state = TIGER_STATE_ATTACK_2;
                }
            } else if (
                creature->mood != MOOD_ATTACK
                && Random_GetControl() < TIGER_ROAR_CHANCE) {
                item->required_anim_state = TIGER_STATE_ROAR;
                item->goal_anim_state = TIGER_STATE_STOP;
            }
            creature->flags = 0;
            break;

        case TIGER_STATE_ATTACK_1:
        case TIGER_STATE_ATTACK_2:
        case TIGER_STATE_ATTACK_3:
            if (creature->flags == 0
                && (item->touch_bits & TIGER_TOUCH_BITS) != 0) {
                Lara_TakeDamage(TIGER_BITE_DAMAGE, true);
                Creature_Effect(item, &m_TigerBite, Spawn_Blood);
                creature->flags = 1;
            }
            break;
            ;

        default:
            break;
        }
    } else if (item->current_anim_state != TIGER_STATE_DEATH) {
        Item_SwitchToAnim(item, TIGER_ANIM_DEATH, 0);
        item->current_anim_state = TIGER_STATE_DEATH;
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
}

REGISTER_OBJECT(O_TIGER, M_Setup)
