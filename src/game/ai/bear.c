#include "game/ai/bear.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/game.h"
#include "game/lot.h"
#include "global/types.h"
#include "global/vars.h"

BITE_INFO BearHeadBite = { 0, 96, 335, 14 };

void SetupBear(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = BearControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = BEAR_HITPOINTS;
    obj->pivot_length = 500;
    obj->radius = BEAR_RADIUS;
    obj->smartness = BEAR_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index + 52] |= BEB_ROT_Y;
}

void BearControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *bear = (CREATURE_INFO *)item->data;
    int16_t head = 0;
    PHD_ANGLE angle = 0;

    if (item->hit_points <= 0) {
        angle = CreatureTurn(item, PHD_DEGREE);

        switch (item->current_anim_state) {
        case BEAR_WALK:
            item->goal_anim_state = BEAR_REAR;
            break;

        case BEAR_RUN:
        case BEAR_STROLL:
            item->goal_anim_state = BEAR_STOP;
            break;

        case BEAR_REAR:
            bear->flags = 1;
            item->goal_anim_state = BEAR_DEATH;
            break;

        case BEAR_STOP:
            bear->flags = 0;
            item->goal_anim_state = BEAR_DEATH;
            break;

        case BEAR_DEATH:
            if (bear->flags && (item->touch_bits & BEAR_TOUCH)) {
                LaraItem->hit_points -= BEAR_SLAM_DAMAGE;
                LaraItem->hit_status = 1;
                bear->flags = 0;
            }
            break;
        }
    } else {
        AI_INFO info; // [esp+18h] [ebp-14h]
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, bear->maximum_turn);

        int dead_enemy = LaraItem->hit_points <= 0;
        if (item->hit_status) {
            bear->flags = 1;
        }

        switch ((int16_t)item->current_anim_state) {
        case BEAR_STOP:
            if (dead_enemy) {
                if (info.bite && info.distance < BEAR_EAT_RANGE) {
                    item->goal_anim_state = BEAR_EAT;
                } else {
                    item->goal_anim_state = BEAR_STROLL;
                }
            } else if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (bear->mood == MOOD_BORED) {
                item->goal_anim_state = BEAR_STROLL;
            } else {
                item->goal_anim_state = BEAR_RUN;
            }
            break;

        case BEAR_STROLL:
            bear->maximum_turn = BEAR_WALK_TURN;
            if (dead_enemy && (item->touch_bits & BEAR_TOUCH) && info.ahead) {
                item->goal_anim_state = BEAR_STOP;
            } else if (bear->mood != MOOD_BORED) {
                item->goal_anim_state = BEAR_STOP;
                if (bear->mood == MOOD_ESCAPE) {
                    item->required_anim_state = BEAR_STROLL;
                }
            } else if (GetRandomControl() < BEAR_ROAR_CHANCE) {
                item->required_anim_state = BEAR_ROAR;
                item->goal_anim_state = BEAR_STOP;
            }
            break;

        case BEAR_RUN:
            bear->maximum_turn = BEAR_RUN_TURN;
            if (item->touch_bits & BEAR_TOUCH) {
                LaraItem->hit_points -= BEAR_CHARGE_DAMAGE;
                LaraItem->hit_status = 1;
            }
            if (bear->mood == MOOD_BORED || dead_enemy) {
                item->goal_anim_state = BEAR_STOP;
            } else if (info.ahead && !item->required_anim_state) {
                if (!bear->flags && info.distance < BEAR_REAR_RANGE
                    && GetRandomControl() < BEAR_REAR_CHANCE) {
                    item->required_anim_state = BEAR_REAR;
                    item->goal_anim_state = BEAR_STOP;
                } else if (info.distance < BEAR_ATTACK_RANGE) {
                    item->goal_anim_state = BEAR_ATTACK1;
                }
            }
            break;

        case BEAR_REAR:
            if (bear->flags) {
                item->required_anim_state = BEAR_STROLL;
                item->goal_anim_state = BEAR_STOP;
            } else if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (bear->mood == MOOD_BORED || bear->mood == MOOD_ESCAPE) {
                item->goal_anim_state = BEAR_STOP;
            } else if (info.bite && info.distance < BEAR_PAT_RANGE) {
                item->goal_anim_state = BEAR_ATTACK2;
            } else {
                item->goal_anim_state = BEAR_WALK;
            }
            break;

        case BEAR_WALK:
            if (bear->flags) {
                item->required_anim_state = BEAR_STROLL;
                item->goal_anim_state = BEAR_REAR;
            } else if (info.ahead && (item->touch_bits & BEAR_TOUCH)) {
                item->goal_anim_state = BEAR_REAR;
            } else if (bear->mood == MOOD_ESCAPE) {
                item->goal_anim_state = BEAR_REAR;
                item->required_anim_state = BEAR_STROLL;
            } else if (
                bear->mood == MOOD_BORED
                || GetRandomControl() < BEAR_ROAR_CHANCE) {
                item->required_anim_state = BEAR_ROAR;
                item->goal_anim_state = BEAR_REAR;
            } else if (
                info.distance > BEAR_REAR_RANGE
                || GetRandomControl() < BEAR_DROP_CHANCE) {
                item->required_anim_state = BEAR_STOP;
                item->goal_anim_state = BEAR_REAR;
            }
            break;

        case BEAR_ATTACK1:
            if (!item->required_anim_state && (item->touch_bits & BEAR_TOUCH)) {
                CreatureEffect(item, &BearHeadBite, DoBloodSplat);
                LaraItem->hit_points -= BEAR_ATTACK_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = BEAR_STOP;
            }
            break;

        case BEAR_ATTACK2:
            if (!item->required_anim_state && (item->touch_bits & BEAR_TOUCH)) {
                LaraItem->hit_points -= BEAR_PAT_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = BEAR_REAR;
            }
            break;
        }
    }

    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);
}
