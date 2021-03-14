#include "game/box.h"
#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/game.h"
#include "game/lot.h"
#include "game/vars.h"
#include "game/ai/wolf.h"

BITE_INFO WolfJawBite = { 0, -14, 174, 6 };

void SetupWolf(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseWolf;
    obj->control = WolfControl;
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
    AnimBones[obj->bone_index + 8] |= BEB_ROT_Y;
}

void InitialiseWolf(int16_t item_num)
{
    Items[item_num].frame_number = WOLF_SLEEP_FRAME;
    InitialiseCreature(item_num);
}

void WolfControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

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
            item->anim_number = Objects[O_WOLF].anim_index + WOLF_DIE_ANIM
                + (int16_t)(GetRandomControl() / 11000);
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, wolf->maximum_turn);

        switch (item->current_anim_state) {
        case WOLF_SLEEP:
            head = 0;
            if (wolf->mood == MOOD_ESCAPE
                || info.zone_number == info.enemy_zone) {
                item->required_anim_state = WOLF_CROUCH;
                item->goal_anim_state = WOLF_STOP;
            } else if (GetRandomControl() < WOLF_WAKE_CHANCE) {
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
            } else if (GetRandomControl() < WOLF_SLEEP_CHANCE) {
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
            } else if (GetRandomControl() < WOLF_HOWL_CHANCE) {
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
                CreatureEffect(item, &WolfJawBite, DoBloodSplat);
                LaraItem->hit_points -= WOLF_POUNCE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = WOLF_RUN;
            }
            item->goal_anim_state = WOLF_RUN;
            break;

        case WOLF_BITE:
            if (item->required_anim_state == WOLF_EMPTY
                && (item->touch_bits & WOLF_TOUCH) && info.ahead) {
                CreatureEffect(item, &WolfJawBite, DoBloodSplat);
                LaraItem->hit_points -= WOLF_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = WOLF_CROUCH;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, tilt);
}
