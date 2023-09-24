#include "game/objects/creatures/raptor.h"

#include "game/creature.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#include <stdbool.h>

#define RAPTOR_ATTACK_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_BITE_DAMAGE 100
#define RAPTOR_CHARGE_DAMAGE 100
#define RAPTOR_CLOSE_RANGE SQUARE(680) // = 462400
#define RAPTOR_DIE_ANIM 9
#define RAPTOR_LUNGE_DAMAGE 100
#define RAPTOR_LUNGE_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_ROAR_CHANCE 256
#define RAPTOR_RUN_TURN (4 * PHD_DEGREE) // = 728
#define RAPTOR_TOUCH 0xFF7C00
#define RAPTOR_WALK_TURN (1 * PHD_DEGREE) // = 182
#define RAPTOR_HITPOINTS 20
#define RAPTOR_RADIUS (WALL_L / 3) // = 341
#define RAPTOR_SMARTNESS 0x4000

typedef enum {
    RAPTOR_EMPTY = 0,
    RAPTOR_STOP = 1,
    RAPTOR_WALK = 2,
    RAPTOR_RUN = 3,
    RAPTOR_ATTACK1 = 4,
    RAPTOR_DEATH = 5,
    RAPTOR_WARNING = 6,
    RAPTOR_ATTACK2 = 7,
    RAPTOR_ATTACK3 = 8,
} RAPTOR_ANIM;

static BITE_INFO m_RaptorBite = { 0, 66, 318, 22 };

void Raptor_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Raptor_Control;
    obj->collision = Creature_Collision;
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
    g_AnimBones[obj->bone_index + 84] |= BEB_ROT_Y;
}

void Raptor_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *raptor = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != RAPTOR_DEATH) {
            item->current_anim_state = RAPTOR_DEATH;
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
        case RAPTOR_STOP:
            if (item->required_anim_state != RAPTOR_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            } else if (item->touch_bits & RAPTOR_TOUCH) {
                item->goal_anim_state = RAPTOR_ATTACK3;
            } else if (info.bite && info.distance < RAPTOR_CLOSE_RANGE) {
                item->goal_anim_state = RAPTOR_ATTACK3;
            } else if (info.bite && info.distance < RAPTOR_LUNGE_RANGE) {
                item->goal_anim_state = RAPTOR_ATTACK1;
            } else if (raptor->mood == MOOD_BORED) {
                item->goal_anim_state = RAPTOR_WALK;
            } else {
                item->goal_anim_state = RAPTOR_RUN;
            }
            break;

        case RAPTOR_WALK:
            raptor->maximum_turn = RAPTOR_WALK_TURN;
            if (raptor->mood != MOOD_BORED) {
                item->goal_anim_state = RAPTOR_STOP;
            } else if (info.ahead && Random_GetControl() < RAPTOR_ROAR_CHANCE) {
                item->required_anim_state = RAPTOR_WARNING;
                item->goal_anim_state = RAPTOR_STOP;
            }
            break;

        case RAPTOR_RUN:
            tilt = angle;
            raptor->maximum_turn = RAPTOR_RUN_TURN;
            if (item->touch_bits & RAPTOR_TOUCH) {
                item->goal_anim_state = RAPTOR_STOP;
            } else if (info.bite && info.distance < RAPTOR_ATTACK_RANGE) {
                if (item->goal_anim_state == RAPTOR_RUN) {
                    if (Random_GetControl() < 0x2000) {
                        item->goal_anim_state = RAPTOR_STOP;
                    } else {
                        item->goal_anim_state = RAPTOR_ATTACK2;
                    }
                }
            } else if (
                info.ahead && raptor->mood != MOOD_ESCAPE
                && Random_GetControl() < RAPTOR_ROAR_CHANCE) {
                item->required_anim_state = RAPTOR_WARNING;
                item->goal_anim_state = RAPTOR_STOP;
            } else if (raptor->mood == MOOD_BORED) {
                item->goal_anim_state = RAPTOR_STOP;
            }
            break;

        case RAPTOR_ATTACK1:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY && info.ahead
                && (item->touch_bits & RAPTOR_TOUCH)) {
                Creature_Effect(item, &m_RaptorBite, Effect_Blood);
                Lara_TakeDamage(RAPTOR_LUNGE_DAMAGE, true);
                item->required_anim_state = RAPTOR_STOP;
            }
            break;

        case RAPTOR_ATTACK2:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY && info.ahead
                && (item->touch_bits & RAPTOR_TOUCH)) {
                Creature_Effect(item, &m_RaptorBite, Effect_Blood);
                Lara_TakeDamage(RAPTOR_CHARGE_DAMAGE, true);
                item->required_anim_state = RAPTOR_RUN;
            }
            break;

        case RAPTOR_ATTACK3:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY
                && (item->touch_bits & RAPTOR_TOUCH)) {
                Creature_Effect(item, &m_RaptorBite, Effect_Blood);
                Lara_TakeDamage(RAPTOR_BITE_DAMAGE, true);
                item->required_anim_state = RAPTOR_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
}
