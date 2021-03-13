#include "game/ai/raptor.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/lot.h"
#include "game/vars.h"

BITE_INFO RaptorBite = { 0, 66, 318, 22 };

void SetupRaptor(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = RaptorControl;
    obj->collision = CreatureCollision;
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
    AnimBones[obj->bone_index + 84] |= BEB_ROT_Y;
}

void RaptorControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
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
            item->anim_number = Objects[O_RAPTOR].anim_index + RAPTOR_DIE_ANIM
                + (GetRandomControl() / 16200);
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, raptor->maximum_turn);

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
            } else if (info.ahead && GetRandomControl() < RAPTOR_ROAR_CHANCE) {
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
                    if (GetRandomControl() < 0x2000) {
                        item->goal_anim_state = RAPTOR_STOP;
                    } else {
                        item->goal_anim_state = RAPTOR_ATTACK2;
                    }
                }
            } else if (
                info.ahead && raptor->mood != MOOD_ESCAPE
                && GetRandomControl() < RAPTOR_ROAR_CHANCE) {
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
                CreatureEffect(item, &RaptorBite, DoBloodSplat);
                LaraItem->hit_points -= RAPTOR_LUNGE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAPTOR_STOP;
            }
            break;

        case RAPTOR_ATTACK2:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY && info.ahead
                && (item->touch_bits & RAPTOR_TOUCH)) {
                CreatureEffect(item, &RaptorBite, DoBloodSplat);
                LaraItem->hit_points -= RAPTOR_CHARGE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAPTOR_RUN;
            }
            break;

        case RAPTOR_ATTACK3:
            tilt = angle;
            if (item->required_anim_state == RAPTOR_EMPTY
                && (item->touch_bits & RAPTOR_TOUCH)) {
                CreatureEffect(item, &RaptorBite, DoBloodSplat);
                LaraItem->hit_points -= RAPTOR_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAPTOR_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, tilt);
}
