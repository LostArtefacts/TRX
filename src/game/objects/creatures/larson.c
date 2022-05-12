#include "game/objects/creatures/larson.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/creature.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/random.h"
#include "global/vars.h"

#define LARSON_POSE_CHANCE 0x60 // = 96
#define LARSON_SHOT_DAMAGE 50
#define LARSON_WALK_TURN (PHD_DEGREE * 3) // = 546
#define LARSON_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define LARSON_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define LARSON_DIE_ANIM 15
#define LARSON_HITPOINTS 50
#define LARSON_RADIUS (WALL_L / 10) // = 102
#define LARSON_SMARTNESS 0x7FFF

typedef enum {
    LARSON_EMPTY = 0,
    LARSON_STOP = 1,
    LARSON_WALK = 2,
    LARSON_RUN = 3,
    LARSON_AIM = 4,
    LARSON_DEATH = 5,
    LARSON_POSE = 6,
    LARSON_SHOOT = 7,
} LARSON_ANIM;

static BITE_INFO m_LarsonGun = { -60, 170, 0, 14 };

void Larson_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Larson_Control;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = LARSON_HITPOINTS;
    obj->radius = LARSON_RADIUS;
    obj->smartness = LARSON_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index + 24] |= BEB_ROT_Y;
}

void Larson_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *person = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != LARSON_DEATH) {
            item->current_anim_state = LARSON_DEATH;
            item->anim_number =
                g_Objects[O_LARSON].anim_index + LARSON_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, person->maximum_turn);

        switch (item->current_anim_state) {
        case LARSON_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (person->mood == MOOD_BORED) {
                item->goal_anim_state = Random_GetControl() < LARSON_POSE_CHANCE
                    ? LARSON_POSE
                    : LARSON_WALK;
            } else if (person->mood == MOOD_ESCAPE) {
                item->goal_anim_state = LARSON_RUN;
            } else {
                item->goal_anim_state = LARSON_WALK;
            }
            break;

        case LARSON_POSE:
            if (person->mood != MOOD_BORED) {
                item->goal_anim_state = LARSON_STOP;
            } else if (Random_GetControl() < LARSON_POSE_CHANCE) {
                item->required_anim_state = LARSON_WALK;
                item->goal_anim_state = LARSON_STOP;
            }
            break;

        case LARSON_WALK:
            person->maximum_turn = LARSON_WALK_TURN;
            if (person->mood == MOOD_BORED
                && Random_GetControl() < LARSON_POSE_CHANCE) {
                item->required_anim_state = LARSON_POSE;
                item->goal_anim_state = LARSON_STOP;
            } else if (person->mood == MOOD_ESCAPE) {
                item->required_anim_state = LARSON_RUN;
                item->goal_anim_state = LARSON_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = LARSON_AIM;
                item->goal_anim_state = LARSON_STOP;
            } else if (!info.ahead || info.distance > LARSON_WALK_RANGE) {
                item->required_anim_state = LARSON_RUN;
                item->goal_anim_state = LARSON_STOP;
            }
            break;

        case LARSON_RUN:
            person->maximum_turn = LARSON_RUN_TURN;
            tilt = angle / 2;
            if (person->mood == MOOD_BORED
                && Random_GetControl() < LARSON_POSE_CHANCE) {
                item->required_anim_state = LARSON_POSE;
                item->goal_anim_state = LARSON_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = LARSON_AIM;
                item->goal_anim_state = LARSON_STOP;
            } else if (info.ahead && info.distance < LARSON_WALK_RANGE) {
                item->required_anim_state = LARSON_WALK;
                item->goal_anim_state = LARSON_STOP;
            }
            break;

        case LARSON_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = LARSON_SHOOT;
            } else {
                item->goal_anim_state = LARSON_STOP;
            }
            break;

        case LARSON_SHOOT:
            if (!item->required_anim_state) {
                if (ShotLara(item, info.distance, &m_LarsonGun, head)) {
                    g_LaraItem->hit_points -= LARSON_SHOT_DAMAGE;
                    g_LaraItem->hit_status = 1;
                }
                item->required_anim_state = LARSON_AIM;
            }
            if (person->mood == MOOD_ESCAPE) {
                item->required_anim_state = LARSON_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);

    CreatureAnimation(item_num, angle, 0);
}
