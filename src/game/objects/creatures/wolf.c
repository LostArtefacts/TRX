#include "game/objects/creatures/wolf.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/creature.h"
#include "game/effects/blood.h"
#include "game/lot.h"
#include "game/random.h"
#include "global/vars.h"

#define WOLF_SLEEP_FRAME 96
#define WOLF_BITE_DAMAGE 100
#define WOLF_POUNCE_DAMAGE 50
#define WOLF_DIE_ANIM 20
#define WOLF_WALK_TURN (2 * PHD_DEGREE) // = 364
#define WOLF_RUN_TURN (5 * PHD_DEGREE) // = 910
#define WOLF_STALK_TURN (2 * PHD_DEGREE) // = 364
#define WOLF_ATTACK_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define WOLF_STALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define WOLF_BITE_RANGE SQUARE(345) // = 119025
#define WOLF_WAKE_CHANCE 32
#define WOLF_SLEEP_CHANCE 32
#define WOLF_HOWL_CHANCE 384
#define WOLF_TOUCH 0x774F
#define WOLF_HITPOINTS 6
#define WOLF_RADIUS (WALL_L / 3) // = 341
#define WOLF_SMARTNESS 0x2000

typedef enum {
    WOLF_EMPTY = 0,
    WOLF_STOP = 1,
    WOLF_WALK = 2,
    WOLF_RUN = 3,
    WOLF_JUMP = 4,
    WOLF_STALK = 5,
    WOLF_ATTACK = 6,
    WOLF_HOWL = 7,
    WOLF_SLEEP = 8,
    WOLF_CROUCH = 9,
    WOLF_FASTTURN = 10,
    WOLF_DEATH = 11,
    WOLF_BITE = 12,
} WOLF_ANIM;

static BITE_INFO m_WolfJawBite = { 0, -14, 174, 6 };

void Wolf_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Wolf_Initialise;
    obj->control = Wolf_Control;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = WOLF_HITPOINTS;
    obj->pivot_length = 375;
    obj->radius = WOLF_RADIUS;
    obj->smartness = WOLF_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index + 8] |= BEB_ROT_Y;
}

void Wolf_Initialise(int16_t item_num)
{
    g_Items[item_num].frame_number = WOLF_SLEEP_FRAME;
    Creature_Initialise(item_num);
}

void Wolf_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *wolf = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != WOLF_DEATH) {
            item->current_anim_state = WOLF_DEATH;
            item->anim_number = g_Objects[O_WOLF].anim_index + WOLF_DIE_ANIM
                + (int16_t)(Random_GetControl() / 11000);
            item->frame_number = g_Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, wolf->maximum_turn);

        switch (item->current_anim_state) {
        case WOLF_SLEEP:
            head = 0;
            if (wolf->mood == MOOD_ESCAPE
                || info.zone_number == info.enemy_zone) {
                item->required_anim_state = WOLF_CROUCH;
                item->goal_anim_state = WOLF_STOP;
            } else if (Random_GetControl() < WOLF_WAKE_CHANCE) {
                item->required_anim_state = WOLF_WALK;
                item->goal_anim_state = WOLF_STOP;
            }
            break;

        case WOLF_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else {
                item->goal_anim_state = WOLF_WALK;
            }
            break;

        case WOLF_WALK:
            wolf->maximum_turn = WOLF_WALK_TURN;
            if (wolf->mood != MOOD_BORED) {
                item->goal_anim_state = WOLF_STALK;
                item->required_anim_state = WOLF_EMPTY;
            } else if (Random_GetControl() < WOLF_SLEEP_CHANCE) {
                item->required_anim_state = WOLF_SLEEP;
                item->goal_anim_state = WOLF_STOP;
            }
            break;

        case WOLF_CROUCH:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (wolf->mood == MOOD_ESCAPE) {
                item->goal_anim_state = WOLF_RUN;
            } else if (info.distance < WOLF_BITE_RANGE && info.bite) {
                item->goal_anim_state = WOLF_BITE;
            } else if (wolf->mood == MOOD_STALK) {
                item->goal_anim_state = WOLF_STALK;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = WOLF_STOP;
            } else {
                item->goal_anim_state = WOLF_RUN;
            }
            break;

        case WOLF_STALK:
            wolf->maximum_turn = WOLF_STALK_TURN;
            if (wolf->mood == MOOD_ESCAPE) {
                item->goal_anim_state = WOLF_RUN;
            } else if (info.distance < WOLF_BITE_RANGE && info.bite) {
                item->goal_anim_state = WOLF_BITE;
            } else if (info.distance > WOLF_STALK_RANGE) {
                item->goal_anim_state = WOLF_RUN;
            } else if (wolf->mood == MOOD_ATTACK) {
                if (!info.ahead || info.distance > WOLF_ATTACK_RANGE
                    || (info.enemy_facing < FRONT_ARC
                        && info.enemy_facing > -FRONT_ARC)) {
                    item->goal_anim_state = WOLF_RUN;
                }
            } else if (Random_GetControl() < WOLF_HOWL_CHANCE) {
                item->required_anim_state = WOLF_HOWL;
                item->goal_anim_state = WOLF_CROUCH;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = WOLF_CROUCH;
            }
            break;

        case WOLF_RUN:
            wolf->maximum_turn = WOLF_RUN_TURN;
            tilt = angle;
            if (info.ahead && info.distance < WOLF_ATTACK_RANGE) {
                if (info.distance > (WOLF_ATTACK_RANGE / 2)
                    && (info.enemy_facing > FRONT_ARC
                        || info.enemy_facing < -FRONT_ARC)) {
                    item->required_anim_state = WOLF_STALK;
                    item->goal_anim_state = WOLF_CROUCH;
                } else {
                    item->goal_anim_state = WOLF_ATTACK;
                    item->required_anim_state = WOLF_EMPTY;
                }
            } else if (
                wolf->mood == MOOD_STALK && info.distance < WOLF_STALK_RANGE) {
                item->required_anim_state = WOLF_STALK;
                item->goal_anim_state = WOLF_CROUCH;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = WOLF_CROUCH;
            }
            break;

        case WOLF_ATTACK:
            tilt = angle;
            if (item->required_anim_state == WOLF_EMPTY
                && (item->touch_bits & WOLF_TOUCH)) {
                Creature_Effect(item, &m_WolfJawBite, Effect_Blood);
                g_LaraItem->hit_points -= WOLF_POUNCE_DAMAGE;
                g_LaraItem->hit_status = 1;
                item->required_anim_state = WOLF_RUN;
            }
            item->goal_anim_state = WOLF_RUN;
            break;

        case WOLF_BITE:
            if (item->required_anim_state == WOLF_EMPTY
                && (item->touch_bits & WOLF_TOUCH) && info.ahead) {
                Creature_Effect(item, &m_WolfJawBite, Effect_Blood);
                g_LaraItem->hit_points -= WOLF_BITE_DAMAGE;
                g_LaraItem->hit_status = 1;
                item->required_anim_state = WOLF_CROUCH;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    CreatureAnimation(item_num, angle, tilt);
}
