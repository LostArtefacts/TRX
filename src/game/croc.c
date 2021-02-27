#include "game/box.h"
#include "game/control.h"
#include "game/croc.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/types.h"
#include "game/vars.h"
#include "util.h"

#define GATOR_BITE_DAMAGE 100
#define GATOR_TURN (3 * PHD_DEGREE) // = 546
#define GATOR_FLOAT_SPEED (WALL_L / 32) // = 32
#define GATOR_DIE_ANIM 4
#define CROC_DIE_ANIM 11

typedef enum {
    CROC_DEATH = 7,
} CROC_ANIMS;

typedef enum {
    GATOR_EMPTY = 0,
    GATOR_SWIM = 1,
    GATOR_ATTACK = 2,
    GATOR_DEATH = 3,
} GATOR_ANIMS;

BITE_INFO CrocBite = { 5, -21, 467, 9 };

void AlligatorControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* gator = item->data;
    FLOOR_INFO* floor;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t room_num;
    int32_t wh;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != GATOR_DEATH) {
            item->current_anim_state = GATOR_DEATH;
            item->anim_number =
                Objects[O_ALLIGATOR].anim_index + GATOR_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            item->hit_points = DONT_TARGET;
        }

        wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh == NO_HEIGHT) {
            item->object_number = O_CROCODILE;
            item->current_anim_state = CROC_DEATH;
            item->goal_anim_state = CROC_DEATH;
            item->anim_number = Objects[O_CROCODILE].anim_index + CROC_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            room_num = item->room_number;
            floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->pos.y =
                GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
            item->pos.x_rot = 0;
        } else if (item->pos.y > wh + GATOR_FLOAT_SPEED) {
            item->pos.y -= GATOR_FLOAT_SPEED;
        } else if (item->pos.y < wh) {
            item->pos.y = wh;
            if (gator) {
                DisableBaddieAI(item_num);
            }
        }

        AnimateItem(item);

        room_num = item->room_number;
        floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        if (room_num != item->room_number) {
            ItemNewRoom(item_num, room_num);
        }
        return;
    }

    AI_INFO info;
    CreatureAIInfo(item, &info);

    if (info.ahead) {
        head = info.angle;
    }

    CreatureMood(item, &info, 1);
    CreatureTurn(item, GATOR_TURN);

    switch (item->current_anim_state) {
    case GATOR_SWIM:
        if (info.bite && item->touch_bits) {
            item->goal_anim_state = GATOR_ATTACK;
        }
        break;

    case GATOR_ATTACK:
        if (item->frame_number == Anims[item->anim_number].frame_base) {
            item->required_anim_state = GATOR_EMPTY;
        }

        if (info.bite && item->touch_bits) {
            if (item->required_anim_state == GATOR_EMPTY) {
                CreatureEffect(item, &CrocBite, DoBloodSplat);
                LaraItem->hit_points -= GATOR_BITE_DAMAGE;
                LaraItem->hit_status = 1;
                item->required_anim_state = GATOR_SWIM;
            }
        } else {
            item->goal_anim_state = GATOR_SWIM;
        }
        break;
    }

    CreatureHead(item, head);

    wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh == NO_HEIGHT) {
        item->object_number = O_CROCODILE;
        item->current_anim_state = Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->anim_number = Objects[O_CROCODILE].anim_index;
        item->frame_number = Anims[item->anim_number].frame_base;
        item->pos.y = item->floor;
        item->pos.x_rot = 0;
        gator->LOT.step = STEP_L;
        gator->LOT.drop = -STEP_L;
        gator->LOT.fly = 0;
    } else if (item->pos.y < wh + STEP_L) {
        item->pos.y = wh + STEP_L;
    }

    CreatureAnimation(item_num, angle, 0);
}

void T1MInjectGameCroc()
{
    INJECT(0x00415520, AlligatorControl);
}
