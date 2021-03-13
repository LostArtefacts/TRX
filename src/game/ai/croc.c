#include "game/ai/croc.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/lot.h"
#include "game/vars.h"

BITE_INFO CrocBite = { 5, -21, 467, 9 };

void SetupCroc(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = CrocControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = CROCODILE_HITPOINTS;
    obj->pivot_length = 600;
    obj->radius = CROCODILE_RADIUS;
    obj->smartness = CROCODILE_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index + 28] |= BEB_ROT_Y;
}

void CrocControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *croc = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != CROC_DEATH) {
            item->current_anim_state = CROC_DEATH;
            item->anim_number = Objects[O_CROCODILE].anim_index + CROC_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        if (item->current_anim_state == CROC_FASTTURN) {
            item->pos.y_rot += CROC_FASTTURN_TURN;
        } else {
            angle = CreatureTurn(item, CROC_TURN);
        }

        switch (item->current_anim_state) {
        case CROC_STOP:
            if (info.bite && info.distance < CROC_BITE_RANGE) {
                item->goal_anim_state = CROC_ATTACK1;
            } else if (croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROC_RUN;
            } else if (croc->mood == MOOD_ATTACK) {
                if ((info.angle < -CROC_FASTTURN_ANGLE
                     || info.angle > CROC_FASTTURN_ANGLE)
                    && info.distance > CROC_FASTTURN_RANGE) {
                    item->goal_anim_state = CROC_FASTTURN;
                } else {
                    item->goal_anim_state = CROC_RUN;
                }
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROC_WALK;
            }
            break;

        case CROC_WALK:
            if (info.ahead && (item->touch_bits & CROC_TOUCH)) {
                item->goal_anim_state = CROC_STOP;
            } else if (croc->mood == MOOD_ATTACK || croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROC_RUN;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROC_STOP;
            }
            break;

        case CROC_FASTTURN:
            if (info.angle > -CROC_FASTTURN_ANGLE
                && info.angle < CROC_FASTTURN_ANGLE) {
                item->goal_anim_state = CROC_WALK;
            }
            break;

        case CROC_RUN:
            if (info.ahead && (item->touch_bits & CROC_TOUCH)) {
                item->goal_anim_state = CROC_STOP;
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROC_WALK;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROC_STOP;
            } else if (
                croc->mood == MOOD_ATTACK && info.distance > CROC_FASTTURN_RANGE
                && (info.angle < -CROC_FASTTURN_ANGLE
                    || info.angle > CROC_FASTTURN_ANGLE)) {
                item->goal_anim_state = CROC_STOP;
            }
            break;

        case CROC_ATTACK1:
            if (item->required_anim_state == CROC_EMPTY) {
                CreatureEffect(item, &CrocBite, DoBloodSplat);
                LaraItem->hit_points -= CROC_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = CROC_STOP;
            }
            break;
        }
    }

    if (croc) {
        CreatureHead(item, head);
    }

    if (RoomInfo[item->room_number].flags & RF_UNDERWATER) {
        item->object_number = O_ALLIGATOR;
        item->current_anim_state = Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->anim_number = Objects[O_ALLIGATOR].anim_index;
        item->frame_number = Anims[item->anim_number].frame_base;
        if (croc) {
            croc->LOT.step = WALL_L * 20;
            croc->LOT.drop = -WALL_L * 20;
            croc->LOT.fly = STEP_L / 16;
        }
    }

    if (croc) {
        CreatureAnimation(item_num, angle, 0);
    } else {
        AnimateItem(item);
    }
}
