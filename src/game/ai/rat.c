#include "game/ai/rat.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/game.h"
#include "game/lot.h"
#include "game/vars.h"

BITE_INFO RatBite = { 0, -11, 108, 3 };

void SetupRat(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = RatControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = RAT_HITPOINTS;
    obj->pivot_length = 200;
    obj->radius = RAT_RADIUS;
    obj->smartness = RAT_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index + 4] |= BEB_ROT_Y;
}

void RatControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *rat = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != RAT_DEATH) {
            item->current_anim_state = RAT_DEATH;
            item->anim_number = Objects[O_RAT].anim_index + RAT_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, RAT_RUN_TURN);

        switch (item->current_anim_state) {
        case RAT_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < RAT_BITE_RANGE) {
                item->goal_anim_state = RAT_ATTACK1;
            } else {
                item->goal_anim_state = RAT_RUN;
            }
            break;

        case RAT_RUN:
            if (info.ahead && (item->touch_bits & RAT_TOUCH)) {
                item->goal_anim_state = RAT_STOP;
            } else if (info.bite && info.distance < RAT_CHARGE_RANGE) {
                item->goal_anim_state = RAT_ATTACK2;
            } else if (info.ahead && GetRandomControl() < RAT_POSE_CHANCE) {
                item->required_anim_state = RAT_POSE;
                item->goal_anim_state = RAT_STOP;
            }
            break;

        case RAT_ATTACK1:
            if (item->required_anim_state == RAT_EMPTY && info.ahead
                && (item->touch_bits & RAT_TOUCH)) {
                CreatureEffect(item, &RatBite, DoBloodSplat);
                LaraItem->hit_points -= RAT_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAT_STOP;
            }
            break;

        case RAT_ATTACK2:
            if (item->required_anim_state == RAT_EMPTY && info.ahead
                && (item->touch_bits & RAT_TOUCH)) {
                CreatureEffect(item, &RatBite, DoBloodSplat);
                LaraItem->hit_points -= RAT_CHARGE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = RAT_RUN;
            }
            break;

        case RAT_POSE:
            if (rat->mood != MOOD_BORED
                || GetRandomControl() < RAT_POSE_CHANCE) {
                item->goal_anim_state = RAT_STOP;
            }
            break;
        }
    }

    CreatureHead(item, head);

    int32_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT) {
        item->object_number = O_VOLE;
        item->anim_number = Objects[O_VOLE].anim_index;
        item->frame_number = Anims[item->anim_number].frame_base;
        item->current_anim_state = Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->pos.y = wh;
    }

    CreatureAnimation(item_num, angle, 0);
}
