#include "game/box.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/rat.h"
#include "game/types.h"
#include "game/vars.h"
#include "util.h"

#define RAT_BITE_DAMAGE 20
#define RAT_CHARGE_DAMAGE 20
#define RAT_TOUCH 0x300018F
#define RAT_DIE_ANIM 8
#define RAT_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define RAT_BITE_RANGE SQUARE(341) // = 116281
#define RAT_CHARGE_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAT_POSE_CHANCE 256

#define VOLE_DIE_ANIM 2
#define VOLE_SWIM_TURN (PHD_DEGREE * 3) // = 546
#define VOLE_ATTACK_RANGE SQUARE(300) // = 90000

typedef enum {
    RAT_EMPTY = 0,
    RAT_STOP = 1,
    RAT_ATTACK2 = 2,
    RAT_RUN = 3,
    RAT_ATTACK1 = 4,
    RAT_DEATH = 5,
    RAT_POSE = 6,
} RAT_ANIM;

typedef enum {
    VOLE_EMPTY = 0,
    VOLE_SWIM = 1,
    VOLE_ATTACK = 2,
    VOLE_DEATH = 3,
} VOLE_ANIM;

static BITE_INFO RatBite = { 0, -11, 108, 3 };

void RatControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* rat = item->data;
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

void VoleControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != VOLE_DEATH) {
            item->current_anim_state = VOLE_DEATH;
            item->anim_number = Objects[O_VOLE].anim_index + VOLE_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
        }

        CreatureHead(item, head);

        AnimateItem(item);
        if (item->status == IS_DEACTIVATED) {
            item->collidable = 0;
            item->hit_points = DONT_TARGET;

            if (CurrentLevel == LV_LEVEL7A) {
                item->status = IS_ACTIVE;
            } else {
                DisableBaddieAI(item_num);
                RemoveActiveItem(item_num);
            }
        }

        int32_t wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh == NO_HEIGHT) {
            item->object_number = O_RAT;
            item->current_anim_state = RAT_DEATH;
            item->goal_anim_state = RAT_DEATH;
            item->anim_number = Objects[O_RAT].anim_index + RAT_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            item->pos.y = item->floor;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, VOLE_SWIM_TURN);

        switch (item->current_anim_state) {
        case VOLE_SWIM:
            if (info.ahead && (item->touch_bits & RAT_TOUCH)) {
                item->goal_anim_state = VOLE_ATTACK;
            }
            break;

        case VOLE_ATTACK:
            if (item->required_anim_state == VOLE_EMPTY && info.ahead
                && (item->touch_bits & RAT_TOUCH)) {
                CreatureEffect(item, &RatBite, DoBloodSplat);
                LaraItem->hit_points -= RAT_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = VOLE_SWIM;
            }
            item->goal_anim_state = VOLE_EMPTY;
            break;
        }

        CreatureHead(item, head);

        int32_t wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh == NO_HEIGHT) {
            item->object_number = O_RAT;
            item->anim_number = Objects[O_RAT].anim_index;
            item->frame_number = Anims[item->anim_number].frame_base;
            item->current_anim_state =
                Anims[item->anim_number].current_anim_state;
            item->goal_anim_state = item->current_anim_state;
        }

        int32_t height = item->pos.y;
        item->pos.y = item->floor;

        CreatureAnimation(item_num, angle, 0);

        if (height != NO_HEIGHT) {
            if (wh - height < -STEP_L / 8) {
                item->pos.y = height - STEP_L / 8;
            } else if (wh - height > STEP_L / 8) {
                item->pos.y = height + STEP_L / 8;
            } else {
                item->pos.y = wh;
            }
        }
    }
}

void T1MInjectGameRat()
{
    INJECT(0x00433F50, RatControl);
    INJECT(0x00434210, VoleControl);
}
