#include "game/ai/lion.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "global/vars.h"

BITE_INFO LionBite = { -2, -10, 132, 21 };

void SetupLion(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = LionControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = LION_HITPOINTS;
    obj->pivot_length = 400;
    obj->radius = LION_RADIUS;
    obj->smartness = LION_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index + 76] |= BEB_ROT_Y;
}

void SetupLioness(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = LionControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = LIONESS_HITPOINTS;
    obj->pivot_length = 400;
    obj->radius = LIONESS_RADIUS;
    obj->smartness = LIONESS_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index + 76] |= BEB_ROT_Y;
}

void SetupPuma(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = LionControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = PUMA_HITPOINTS;
    obj->pivot_length = 400;
    obj->radius = PUMA_RADIUS;
    obj->smartness = PUMA_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index + 76] |= BEB_ROT_Y;
}

void LionControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *lion = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != LION_DEATH) {
            item->current_anim_state = LION_DEATH;
            int16_t anim_number = -1;
            if (item->object_number == O_PUMA) {
                anim_number = Objects[O_PUMA].anim_index + PUMA_DIE_ANIM
                    + (int16_t)(GetRandomControl() / 0x4000);
            } else if (item->object_number == O_LION) {
                anim_number = Objects[O_LION].anim_index + LION_DIE_ANIM
                    + (int16_t)(GetRandomControl() / 0x4000);
            } else {
                anim_number = Objects[O_LIONESS].anim_index
                    + LION_DIE_ANIM + (int16_t)(GetRandomControl() / 0x4000);
            }
            Item_SetAnimBase(item, anim_number);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, lion->maximum_turn);

        switch (item->current_anim_state) {
        case LION_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (lion->mood == MOOD_BORED) {
                item->goal_anim_state = LION_WALK;
            } else if (info.ahead && (item->touch_bits & LION_TOUCH)) {
                item->goal_anim_state = LION_ATTACK2;
            } else if (info.ahead && info.distance < LION_POUNCE_RANGE) {
                item->goal_anim_state = LION_ATTACK1;
            } else {
                item->goal_anim_state = LION_RUN;
            }
            break;

        case LION_WALK:
            lion->maximum_turn = LION_WALK_TURN;
            if (lion->mood != MOOD_BORED) {
                item->goal_anim_state = LION_STOP;
            } else if (GetRandomControl() < LION_ROAR_CHANCE) {
                item->required_anim_state = LION_WARNING;
                item->goal_anim_state = LION_STOP;
            }
            break;

        case LION_RUN:
            lion->maximum_turn = LION_RUN_TURN;
            tilt = angle;
            if (lion->mood == MOOD_BORED) {
                item->goal_anim_state = LION_STOP;
            } else if (info.ahead && info.distance < LION_POUNCE_RANGE) {
                item->goal_anim_state = LION_STOP;
            } else if ((item->touch_bits & LION_TOUCH) && info.ahead) {
                item->goal_anim_state = LION_STOP;
            } else if (
                lion->mood != MOOD_ESCAPE
                && GetRandomControl() < LION_ROAR_CHANCE) {
                item->required_anim_state = LION_WARNING;
                item->goal_anim_state = LION_STOP;
            }
            break;

        case LION_ATTACK1:
            if (item->required_anim_state == LION_EMPTY
                && (item->touch_bits & LION_TOUCH)) {
                LaraItem->hit_points -= LION_POUNCE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = LION_STOP;
            }
            break;

        case LION_ATTACK2:
            if (item->required_anim_state == LION_EMPTY
                && (item->touch_bits & LION_TOUCH)) {
                CreatureEffect(item, &LionBite, DoBloodSplat);
                LaraItem->hit_points -= LION_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = LION_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, tilt);
}
