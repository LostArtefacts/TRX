#include "game/ai/larson.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/game.h"
#include "game/lot.h"
#include "game/people.h"
#include "global/vars.h"

BITE_INFO LarsonGun = { -60, 170, 0, 14 };

void SetupLarson(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = LarsonControl;
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
    AnimBones[obj->bone_index + 24] |= BEB_ROT_Y;
}

void LarsonControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

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
            item->anim_number = Objects[O_LARSON].anim_index + LARSON_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, person->maximum_turn);

        switch (item->current_anim_state) {
        case LARSON_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (person->mood == MOOD_BORED) {
                item->goal_anim_state = GetRandomControl() < LARSON_POSE_CHANCE
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
            } else if (GetRandomControl() < LARSON_POSE_CHANCE) {
                item->required_anim_state = LARSON_WALK;
                item->goal_anim_state = LARSON_STOP;
            }
            break;

        case LARSON_WALK:
            person->maximum_turn = LARSON_WALK_TURN;
            if (person->mood == MOOD_BORED
                && GetRandomControl() < LARSON_POSE_CHANCE) {
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
                && GetRandomControl() < LARSON_POSE_CHANCE) {
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
                if (ShotLara(item, info.distance, &LarsonGun, head)) {
                    LaraItem->hit_points -= LARSON_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }
                item->required_anim_state = LARSON_AIM;
            }
            if (person->mood == MOOD_ESCAPE) {
                item->required_anim_state = LARSON_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);

    CreatureAnimation(item_num, angle, 0);
}
