#include "game/objects/creatures/dog.h"

#include "game/creature.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define DOG_HITPOINTS       10
#define DOG_TOUCH_BITS      0b00000001'00111111'01110000'00000000
#define DOG_RADIUS          (WALL_L / 3) // = 341
#define DOG_WALK_TURN       (3 * DEG_1) // = 546
#define DOG_RUN_TURN        (6 * DEG_1) // = 1092
#define DOG_ATTACK_1_RANGE  SQUARE(WALL_L / 3) // = 116281
#define DOG_ATTACK_2_RANGE  SQUARE(WALL_L * 3 / 4) // = 589824
#define DOG_ATTACK_3_RANGE  SQUARE(WALL_L * 2 / 3) // = 465124
#define DOG_LEAP_DAMAGE     200
#define DOG_BITE_DAMAGE     100
#define DOG_LUNGE_DAMAGE    100
#define DOG_BARK_CHANCE     0x300
#define DOG_CROUCH_CHANCE   (DOG_BARK_CHANCE + 0x300) // = 0x600
#define DOG_STAND_CHANCE    (DOG_CROUCH_CHANCE + 0x500) // = 0xB00
#define DOG_WALK_CHANCE     (DOG_STAND_CHANCE + 0x2000) // = 0x2600
#define DOG_UNCROUCH_CHANCE 0x100
#define DOG_UNSTAND_CHANCE  0x200
#define DOG_UNBARK_CHANCE   0x500
// clang-format on

typedef enum {
    // clang-format off
    DOG_STATE_EMPTY    = 0,
    DOG_STATE_WALK     = 1,
    DOG_STATE_RUN      = 2,
    DOG_STATE_STOP     = 3,
    DOG_STATE_BARK     = 4,
    DOG_STATE_CROUCH   = 5,
    DOG_STATE_STAND    = 6,
    DOG_STATE_ATTACK_1 = 7,
    DOG_STATE_ATTACK_2 = 8,
    DOG_STATE_ATTACK_3 = 9,
    DOG_STATE_DEATH    = 10,
    // clang-format on
} DOG_STATE;

typedef enum {
    DOG_ANIM_DEATH = 13,
} DOG_ANIM;

static const BITE m_DogBite = {
    .pos = { .x = 0, .y = 30, .z = 141 },
    .mesh_num = 20,
};

void Dog_Setup(void)
{
    OBJECT *const obj = Object_Get(O_DOG);
    if (!obj->loaded) {
        return;
    }

    obj->control_func = Dog_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = DOG_HITPOINTS;
    obj->radius = DOG_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 300;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 19)->rot_y = true;
}

void Dog_Control(const int16_t item_num)
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
        Creature_Mood(item, &info, MOOD_BORED);

        if (info.ahead) {
            head = info.angle;
        }
        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case DOG_STATE_WALK:
            creature->maximum_turn = DOG_WALK_TURN;
            if (creature->mood == MOOD_BORED) {
                const int16_t random = Random_GetControl();
                if (random < DOG_BARK_CHANCE) {
                    item->required_anim_state = DOG_STATE_BARK;
                    item->goal_anim_state = DOG_STATE_STOP;
                } else if (random < DOG_CROUCH_CHANCE) {
                    item->required_anim_state = DOG_STATE_CROUCH;
                    item->goal_anim_state = DOG_STATE_STOP;
                } else if (random < DOG_STAND_CHANCE) {
                    item->goal_anim_state = DOG_STATE_STOP;
                }
            } else {
                item->goal_anim_state = DOG_STATE_RUN;
            }
            break;

        case DOG_STATE_RUN:
            tilt = angle;
            creature->maximum_turn = DOG_RUN_TURN;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = DOG_STATE_STOP;
            } else if (info.distance < DOG_ATTACK_2_RANGE) {
                item->goal_anim_state = DOG_STATE_ATTACK_2;
            }
            break;

        case DOG_STATE_STOP:
            creature->maximum_turn = 0;
            creature->flags = 0;
            if (creature->mood != MOOD_BORED) {
                if (creature->mood == MOOD_ESCAPE) {
                    item->goal_anim_state = DOG_STATE_RUN;
                } else if (info.distance < DOG_ATTACK_1_RANGE && info.ahead) {
                    item->goal_anim_state = DOG_STATE_ATTACK_1;
                } else {
                    item->goal_anim_state = DOG_STATE_RUN;
                }
            } else if (item->required_anim_state != DOG_STATE_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else {
                const int16_t random = Random_GetControl();
                if (random < DOG_BARK_CHANCE) {
                    item->goal_anim_state = DOG_STATE_BARK;
                } else if (random < DOG_CROUCH_CHANCE) {
                    item->goal_anim_state = DOG_STATE_CROUCH;
                } else if (random < DOG_WALK_CHANCE) {
                    item->goal_anim_state = DOG_STATE_WALK;
                }
            }
            break;

        case DOG_STATE_BARK:
            if (creature->mood || Random_GetControl() < DOG_UNBARK_CHANCE) {
                item->goal_anim_state = DOG_STATE_STOP;
            }
            break;

        case DOG_STATE_CROUCH:
            if (creature->mood || Random_GetControl() < DOG_UNCROUCH_CHANCE) {
                item->goal_anim_state = DOG_STATE_STOP;
            }
            break;

        case DOG_STATE_STAND:
            if (creature->mood || Random_GetControl() < DOG_UNSTAND_CHANCE) {
                item->goal_anim_state = DOG_STATE_STOP;
            }
            break;

        case DOG_STATE_ATTACK_1:
            creature->maximum_turn = 0;
            if (creature->flags != 1 && info.ahead != 0
                && (item->touch_bits & DOG_TOUCH_BITS) != 0) {
                Creature_Effect(item, &m_DogBite, Spawn_Blood);
                Lara_TakeDamage(DOG_BITE_DAMAGE, true);
                creature->flags = 1;
            }
            if (info.distance > DOG_ATTACK_1_RANGE
                && info.distance < DOG_ATTACK_3_RANGE) {
                item->goal_anim_state = DOG_STATE_ATTACK_3;
            } else {
                item->goal_anim_state = DOG_STATE_STOP;
            }
            break;

        case DOG_STATE_ATTACK_2:
            if (creature->flags != 2
                && (item->touch_bits & DOG_TOUCH_BITS) != 0) {
                Creature_Effect(item, &m_DogBite, Spawn_Blood);
                Lara_TakeDamage(DOG_LUNGE_DAMAGE, true);
                creature->flags = 2;
            }
            if (info.distance < DOG_ATTACK_1_RANGE) {
                item->goal_anim_state = DOG_STATE_ATTACK_1;
            } else if (info.distance < DOG_ATTACK_3_RANGE) {
                item->goal_anim_state = DOG_STATE_ATTACK_3;
            }
            break;

        case DOG_STATE_ATTACK_3:
            creature->maximum_turn = DOG_RUN_TURN;
            if (creature->flags != 3
                && (item->touch_bits & DOG_TOUCH_BITS) != 0) {
                Creature_Effect(item, &m_DogBite, Spawn_Blood);
                Lara_TakeDamage(DOG_LUNGE_DAMAGE, true);
                creature->flags = 3;
            }
            if (info.distance < DOG_ATTACK_1_RANGE) {
                item->goal_anim_state = DOG_STATE_ATTACK_1;
            }
            break;

        default:
            break;
        }
    } else if (item->current_anim_state != DOG_STATE_DEATH) {
        Item_SwitchToAnim(item, DOG_ANIM_DEATH, 0);
        item->current_anim_state = DOG_STATE_DEATH;
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
}
