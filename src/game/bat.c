#include "game/bat.h"
#include "game/box.h"
#include "game/effects.h"
#include "game/lot.h"
#include "game/types.h"
#include "game/vars.h"
#include "util.h"

#define BAT_ATTACK_DAMAGE 2
#define BAT_TURN (20 * PHD_DEGREE) // = 3640

typedef enum {
    BAT_EMPTY = 0,
    BAT_STOP = 1,
    BAT_FLY = 2,
    BAT_ATTACK = 3,
    BAT_FALL = 4,
    BAT_DEATH = 5,
} BAT_ANIMS;

BITE_INFO BatBite = { 0, 16, 45, 4 };

void __cdecl BatControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO* bat = item->data;
    PHD_ANGLE angle = 0;
    if (item->hit_points <= 0) {
        if (item->pos.y < item->floor) {
            item->gravity_status = 1;
            item->goal_anim_state = BAT_FALL;
            item->speed = 0;
        } else {
            item->gravity_status = 0;
            item->goal_anim_state = BAT_DEATH;
            item->pos.y = item->floor;
        }
        CreatureAnimation(item_num, 0, 0);
        return;
    } else {
        AI_INFO info;

        CreatureAIInfo(item, &info);
        CreatureMood(item, &info, 0);
        angle = CreatureTurn(item, BAT_TURN);

        switch (item->current_anim_state) {
        case BAT_STOP:
            item->goal_anim_state = BAT_FLY;
            break;

        case BAT_FLY:
            if (item->touch_bits) {
                item->goal_anim_state = BAT_ATTACK;
                CreatureAnimation(item_num, angle, 0);
                return;
            }
            break;

        case BAT_ATTACK:
            if (item->touch_bits) {
                CreatureEffect(item, &BatBite, DoBloodSplat);
                LaraItem->hit_points -= BAT_ATTACK_DAMAGE;
                LaraItem->hit_status = 1;
            } else {
                item->goal_anim_state = BAT_FLY;
                bat->mood = MOOD_BORED;
            }
            break;
        }
    }

    CreatureAnimation(item_num, angle, 0);
}

void T1MInjectGameBat()
{
    INJECT(0x0040D470, BatControl);
}
