#include "game/creature.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define MOUSE_HITPOINTS     4
#define MOUSE_TOUCH_BITS    0b01111111 // = 0x7F
#define MOUSE_RADIUS        (WALL_L / 10) // = 102
#define MOUSE_RUN_TURN      (DEG_1 * 6) // = 1092
#define MOUSE_ATTACK_RANGE  SQUARE(WALL_L / 3) // = 116281
#define MOUSE_BITE_DAMAGE   20
#define MOUSE_WAIT_1_CHANCE 0x500 // = 1280
#define MOUSE_WAIT_2_CHANCE (MOUSE_WAIT_1_CHANCE + 0x500) // = 2560
// clang-format on

typedef enum {
    // clang-format off
    MOUSE_STATE_EMPTY  = 0,
    MOUSE_STATE_RUN    = 1,
    MOUSE_STATE_STOP   = 2,
    MOUSE_STATE_WAIT_1 = 3,
    MOUSE_STATE_WAIT_2 = 4,
    MOUSE_STATE_ATTACK = 5,
    MOUSE_STATE_DEATH  = 6,
    // clang-format on
} MOUSE_STATE;

typedef enum {
    MOUSE_ANIM_DEATH = 9,
} MOUSE_ANIM;

static const BITE m_MouseBite = {
    .pos = { .x = 0, .y = 0, .z = 57 },
    .mesh_num = 2,
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

    obj->hit_points = MOUSE_HITPOINTS;
    obj->radius = MOUSE_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 3)->rot_y = true;
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

    if (item->hit_points <= 0) {
        if (item->current_anim_state != MOUSE_STATE_DEATH) {
            Item_SwitchToAnim(item, MOUSE_ANIM_DEATH, 0);
            item->current_anim_state = MOUSE_STATE_DEATH;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_BORED);

        if (info.ahead) {
            head = info.angle;
        }
        angle = Creature_Turn(item, MOUSE_RUN_TURN);

        switch (item->current_anim_state) {
        case MOUSE_STATE_RUN:
            creature->maximum_turn = MOUSE_RUN_TURN;
            if (creature->mood == MOOD_BORED || creature->mood == MOOD_STALK) {
                const int32_t random = Random_GetControl();
                if (random < MOUSE_WAIT_1_CHANCE) {
                    item->required_anim_state = MOUSE_STATE_WAIT_1;
                    item->goal_anim_state = MOUSE_STATE_STOP;
                } else if (random < MOUSE_WAIT_2_CHANCE) {
                    item->goal_anim_state = MOUSE_STATE_STOP;
                }
            } else if (info.ahead && info.distance < MOUSE_ATTACK_RANGE) {
                item->goal_anim_state = MOUSE_STATE_STOP;
            }
            break;

        case MOUSE_STATE_STOP:
            creature->maximum_turn = 0;
            if (item->required_anim_state != MOUSE_STATE_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            }
            break;

        case MOUSE_STATE_WAIT_1:
            if (Random_GetControl() < MOUSE_WAIT_1_CHANCE) {
                item->goal_anim_state = MOUSE_STATE_STOP;
            }
            break;

        case MOUSE_STATE_WAIT_2:
            if (creature->mood == MOOD_BORED || creature->mood == MOOD_STALK) {
                const int32_t random = Random_GetControl();
                if (random < MOUSE_WAIT_1_CHANCE) {
                    item->required_anim_state = MOUSE_STATE_WAIT_1;
                } else if (random > MOUSE_WAIT_2_CHANCE) {
                    item->required_anim_state = MOUSE_STATE_RUN;
                }
            } else if (info.distance < MOUSE_ATTACK_RANGE) {
                item->required_anim_state = MOUSE_STATE_ATTACK;
            } else {
                item->required_anim_state = MOUSE_STATE_RUN;
            }

            if (item->required_anim_state != MOUSE_STATE_EMPTY) {
                item->goal_anim_state = MOUSE_STATE_STOP;
            }
            break;

        case MOUSE_STATE_ATTACK:
            if (item->required_anim_state == MOUSE_STATE_EMPTY
                && (item->touch_bits & MOUSE_TOUCH_BITS) != 0) {
                Lara_TakeDamage(MOUSE_BITE_DAMAGE, true);
                Creature_Effect(item, &m_MouseBite, Spawn_Blood);
                item->required_anim_state = MOUSE_STATE_STOP;
            }
            break;

        default:
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}

REGISTER_OBJECT(O_MOUSE, M_Setup)
