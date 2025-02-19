#include "game/creature.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define SHARK_HITPOINTS       30
#define SHARK_TOUCH_BITS      0b00110100'00000000 // = 0x3400
#define SHARK_RADIUS          (WALL_L / 3) // = 341
#define SHARK_BITE_DAMAGE     400
#define SHARK_SWIM_2_RANGE    SQUARE(WALL_L * 3) // = 9437184
#define SHARK_ATTACK_1_RANGE  SQUARE(WALL_L * 3 / 4) // = 589824
#define SHARK_ATTACK_2_RANGE  SQUARE(WALL_L * 4 / 3) // = 1863225
#define SHARK_SWIM_1_TURN     (DEG_1 / 2) // = 91
#define SHARK_SWIM_2_TURN     (DEG_1 * 2) // = 364
#define SHARK_ATTACK_1_CHANCE 0x800
// clang-format on

typedef enum {
    // clang-format off
    SHARK_STATE_STOP     = 0,
    SHARK_STATE_SWIM_1   = 1,
    SHARK_STATE_SWIM_2   = 2,
    SHARK_STATE_ATTACK_1 = 3,
    SHARK_STATE_ATTACK_2 = 4,
    SHARK_STATE_DEATH    = 5,
    SHARK_STATE_KILL     = 6,
    // clang-format on
} SHARK_STATE;

typedef enum {
    // clang-format off
    SHARK_ANIM_DEATH = 4,
    SHARK_ANIM_KILL  = 19,
    // clang-format on
} SHARK_ANIM;

static const BITE m_SharkBite = {
    .pos = { .x = 17, .y = -22, .z = 344 },
    .mesh_num = 12,
};

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->collision_func = Creature_Collision;

    obj->hit_points = SHARK_HITPOINTS;
    obj->radius = SHARK_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 200;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 9)->rot_y = true;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    const bool lara_alive = g_LaraItem->hit_points > 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != SHARK_STATE_DEATH) {
            Item_SwitchToAnim(item, SHARK_ANIM_DEATH, 0);
            item->current_anim_state = SHARK_STATE_DEATH;
        }
        Creature_Float(item_num);
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_ATTACK);

        int16_t head = 0;
        int16_t angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case SHARK_STATE_STOP:
            creature->flags = 0;
            creature->maximum_turn = 0;
            if (info.ahead != 0 && info.distance < SHARK_ATTACK_1_RANGE
                && info.zone_num == info.enemy_zone_num) {
                item->goal_anim_state = SHARK_STATE_ATTACK_1;
            } else {
                item->goal_anim_state = SHARK_STATE_SWIM_1;
            }
            break;

        case SHARK_STATE_SWIM_1:
            creature->maximum_turn = SHARK_SWIM_1_TURN;
            if (creature->mood == MOOD_BORED) {
            } else if (
                info.ahead != 0 && info.distance < SHARK_ATTACK_1_RANGE) {
                item->goal_anim_state = SHARK_STATE_STOP;
            } else if (
                creature->mood == MOOD_ESCAPE
                || info.distance > SHARK_SWIM_2_RANGE || info.ahead == 0) {
                item->goal_anim_state = SHARK_STATE_SWIM_2;
            }
            break;

        case SHARK_STATE_SWIM_2:
            creature->flags = 0;
            creature->maximum_turn = SHARK_SWIM_2_TURN;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = SHARK_STATE_SWIM_1;
            } else if (creature->mood == MOOD_ESCAPE) {
            } else if (
                info.ahead != 0 && info.distance < SHARK_ATTACK_2_RANGE
                && info.zone_num == info.enemy_zone_num) {
                if (Random_GetControl() < SHARK_ATTACK_1_CHANCE) {
                    item->goal_anim_state = SHARK_STATE_STOP;
                } else if (info.distance < SHARK_ATTACK_1_RANGE) {
                    item->goal_anim_state = SHARK_STATE_ATTACK_2;
                }
            }
            break;

        case SHARK_STATE_ATTACK_1:
        case SHARK_STATE_ATTACK_2:
            if (info.ahead != 0) {
                head = info.angle;
            }

            if (creature->flags == 0
                && (item->touch_bits & SHARK_TOUCH_BITS) != 0) {
                Lara_TakeDamage(SHARK_BITE_DAMAGE, true);
                Creature_Effect(item, &m_SharkBite, Spawn_Blood);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }

        if (lara_alive && g_LaraItem->hit_points <= 0) {
            Creature_Kill(
                item, SHARK_ANIM_KILL, SHARK_STATE_KILL, LA_EXTRA_SHARK_KILL);
        } else if (item->current_anim_state == SHARK_STATE_KILL) {
            Item_Animate(item);
        } else {
            Creature_Head(item, head);
            Creature_Animate(item_num, angle, 0);
            Creature_Underwater(item, SHARK_RADIUS);
        }
    }
}

REGISTER_OBJECT(O_SHARK, M_Setup)
