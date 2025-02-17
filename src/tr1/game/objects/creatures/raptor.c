#include "game/objects/creatures/raptor.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define RAPTOR_ATTACK_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_BITE_DAMAGE 100
#define RAPTOR_CHARGE_DAMAGE 100
#define RAPTOR_CLOSE_RANGE SQUARE(680) // = 462400
#define RAPTOR_DIE_ANIM 9
#define RAPTOR_LUNGE_DAMAGE 100
#define RAPTOR_LUNGE_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_ROAR_CHANCE 256
#define RAPTOR_RUN_TURN (4 * DEG_1) // = 728
#define RAPTOR_TOUCH 0xFF7C00
#define RAPTOR_WALK_TURN (1 * DEG_1) // = 182
#define RAPTOR_HITPOINTS 20
#define RAPTOR_RADIUS (WALL_L / 3) // = 341
#define RAPTOR_SMARTNESS 0x4000

typedef enum {
    RAPTOR_STATE_EMPTY = 0,
    RAPTOR_STATE_STOP = 1,
    RAPTOR_STATE_WALK = 2,
    RAPTOR_STATE_RUN = 3,
    RAPTOR_STATE_ATTACK_1 = 4,
    RAPTOR_STATE_DEATH = 5,
    RAPTOR_STATE_WARNING = 6,
    RAPTOR_STATE_ATTACK_2 = 7,
    RAPTOR_STATE_ATTACK_3 = 8,
} RAPTOR_STATE;

static BITE m_RaptorBite = { 0, 66, 318, 22 };

void Raptor_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = Creature_Initialise;
    obj->control_func = Raptor_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = RAPTOR_HITPOINTS;
    obj->pivot_length = 400;
    obj->radius = RAPTOR_RADIUS;
    obj->smartness = RAPTOR_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;

    Object_GetBone(obj, 21)->rot_y = true;
}

void Raptor_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *raptor = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != RAPTOR_STATE_DEATH) {
            item->current_anim_state = RAPTOR_STATE_DEATH;
            Item_SwitchToAnim(
                item, RAPTOR_DIE_ANIM + (Random_GetControl() / 16200), 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, raptor->maximum_turn);

        switch (item->current_anim_state) {
        case RAPTOR_STATE_STOP:
            if (item->required_anim_state != RAPTOR_STATE_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (item->touch_bits & RAPTOR_TOUCH) {
                item->goal_anim_state = RAPTOR_STATE_ATTACK_3;
            } else if (info.bite && info.distance < RAPTOR_CLOSE_RANGE) {
                item->goal_anim_state = RAPTOR_STATE_ATTACK_3;
            } else if (info.bite && info.distance < RAPTOR_LUNGE_RANGE) {
                item->goal_anim_state = RAPTOR_STATE_ATTACK_1;
            } else if (raptor->mood == MOOD_BORED) {
                item->goal_anim_state = RAPTOR_STATE_WALK;
            } else {
                item->goal_anim_state = RAPTOR_STATE_RUN;
            }
            break;

        case RAPTOR_STATE_WALK:
            raptor->maximum_turn = RAPTOR_WALK_TURN;
            if (raptor->mood != MOOD_BORED) {
                item->goal_anim_state = RAPTOR_STATE_STOP;
            } else if (info.ahead && Random_GetControl() < RAPTOR_ROAR_CHANCE) {
                item->required_anim_state = RAPTOR_STATE_WARNING;
                item->goal_anim_state = RAPTOR_STATE_STOP;
            }
            break;

        case RAPTOR_STATE_RUN:
            tilt = angle;
            raptor->maximum_turn = RAPTOR_RUN_TURN;
            if (item->touch_bits & RAPTOR_TOUCH) {
                item->goal_anim_state = RAPTOR_STATE_STOP;
            } else if (info.bite && info.distance < RAPTOR_ATTACK_RANGE) {
                if (item->goal_anim_state == RAPTOR_STATE_RUN) {
                    if (Random_GetControl() < 0x2000) {
                        item->goal_anim_state = RAPTOR_STATE_STOP;
                    } else {
                        item->goal_anim_state = RAPTOR_STATE_ATTACK_2;
                    }
                }
            } else if (
                info.ahead && raptor->mood != MOOD_ESCAPE
                && Random_GetControl() < RAPTOR_ROAR_CHANCE) {
                item->required_anim_state = RAPTOR_STATE_WARNING;
                item->goal_anim_state = RAPTOR_STATE_STOP;
            } else if (raptor->mood == MOOD_BORED) {
                item->goal_anim_state = RAPTOR_STATE_STOP;
            }
            break;

        case RAPTOR_STATE_ATTACK_1:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_STATE_EMPTY && info.ahead
                && (item->touch_bits & RAPTOR_TOUCH)) {
                Creature_Effect(item, &m_RaptorBite, Spawn_Blood);
                Lara_TakeDamage(RAPTOR_LUNGE_DAMAGE, true);
                item->required_anim_state = RAPTOR_STATE_STOP;
            }
            break;

        case RAPTOR_STATE_ATTACK_2:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_STATE_EMPTY && info.ahead
                && (item->touch_bits & RAPTOR_TOUCH)) {
                Creature_Effect(item, &m_RaptorBite, Spawn_Blood);
                Lara_TakeDamage(RAPTOR_CHARGE_DAMAGE, true);
                item->required_anim_state = RAPTOR_STATE_RUN;
            }
            break;

        case RAPTOR_STATE_ATTACK_3:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_STATE_EMPTY
                && (item->touch_bits & RAPTOR_TOUCH)) {
                Creature_Effect(item, &m_RaptorBite, Spawn_Blood);
                Lara_TakeDamage(RAPTOR_BITE_DAMAGE, true);
                item->required_anim_state = RAPTOR_STATE_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
}
