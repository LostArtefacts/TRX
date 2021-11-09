#include "game/ai/crocodile.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lot.h"
#include "global/vars.h"

BITE_INFO CrocodileBite = { 5, -21, 467, 9 };

void SetupCrocodile(OBJECT_INFO *obj)
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
        if (item->current_anim_state != CROCODILE_DEATH) {
            item->current_anim_state = CROCODILE_DEATH;
            Item_SetAnimBase(
                item, Objects[O_CROCODILE].anim_index + CROCODILE_DIE_ANIM);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        if (item->current_anim_state == CROCODILE_FASTTURN) {
            item->pos.y_rot += CROCODILE_FASTTURN_TURN;
        } else {
            angle = CreatureTurn(item, CROCODILE_TURN);
        }

        switch (item->current_anim_state) {
        case CROCODILE_STOP:
            if (info.bite && info.distance < CROCODILE_BITE_RANGE) {
                item->goal_anim_state = CROCODILE_ATTACK1;
            } else if (croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROCODILE_RUN;
            } else if (croc->mood == MOOD_ATTACK) {
                if ((info.angle < -CROCODILE_FASTTURN_ANGLE
                     || info.angle > CROCODILE_FASTTURN_ANGLE)
                    && info.distance > CROCODILE_FASTTURN_RANGE) {
                    item->goal_anim_state = CROCODILE_FASTTURN;
                } else {
                    item->goal_anim_state = CROCODILE_RUN;
                }
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROCODILE_WALK;
            }
            break;

        case CROCODILE_WALK:
            if (info.ahead && (item->touch_bits & CROCODILE_TOUCH)) {
                item->goal_anim_state = CROCODILE_STOP;
            } else if (croc->mood == MOOD_ATTACK || croc->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CROCODILE_RUN;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROCODILE_STOP;
            }
            break;

        case CROCODILE_FASTTURN:
            if (info.angle > -CROCODILE_FASTTURN_ANGLE
                && info.angle < CROCODILE_FASTTURN_ANGLE) {
                item->goal_anim_state = CROCODILE_WALK;
            }
            break;

        case CROCODILE_RUN:
            if (info.ahead && (item->touch_bits & CROCODILE_TOUCH)) {
                item->goal_anim_state = CROCODILE_STOP;
            } else if (croc->mood == MOOD_STALK) {
                item->goal_anim_state = CROCODILE_WALK;
            } else if (croc->mood == MOOD_BORED) {
                item->goal_anim_state = CROCODILE_STOP;
            } else if (
                croc->mood == MOOD_ATTACK
                && info.distance > CROCODILE_FASTTURN_RANGE
                && (info.angle < -CROCODILE_FASTTURN_ANGLE
                    || info.angle > CROCODILE_FASTTURN_ANGLE)) {
                item->goal_anim_state = CROCODILE_STOP;
            }
            break;

        case CROCODILE_ATTACK1:
            if (item->required_anim_state == CROCODILE_EMPTY) {
                CreatureEffect(item, &CrocodileBite, DoBloodSplat);
                LaraItem->hit_points -= CROCODILE_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = CROCODILE_STOP;
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
        Item_SetAnimBase(item, Objects[O_ALLIGATOR].anim_index);
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
