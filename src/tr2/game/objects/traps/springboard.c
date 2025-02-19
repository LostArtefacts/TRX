#include "game/items.h"
#include "game/objects/common.h"
#include "game/spawn.h"
#include "global/utils.h"
#include "global/vars.h"

#include <libtrx/game/lara/common.h>

typedef enum {
    // clang-format off
    SPRINGBOARD_STATE_OFF = 0,
    SPRINGBOARD_STATE_ON = 1,
    // clang-format on
} SPRINGBOARD_STATE;

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    ITEM *const lara_item = Lara_GetItem();

    if (item->current_anim_state == SPRINGBOARD_STATE_OFF
        && lara_item->pos.y == item->pos.y
        && ROUND_TO_SECTOR(lara_item->pos.x) == ROUND_TO_SECTOR(item->pos.x)
        && ROUND_TO_SECTOR(lara_item->pos.z) == ROUND_TO_SECTOR(item->pos.z)) {
        if (lara_item->hit_points <= 0) {
            return;
        }

        if (lara_item->current_anim_state == LS_BACK
            || lara_item->current_anim_state == LS_FAST_BACK) {
            lara_item->speed = -lara_item->speed;
        }

        lara_item->fall_speed = -240;
        lara_item->gravity = 1;

        Item_SwitchToAnim(lara_item, LA_FALL_START, 0);
        lara_item->current_anim_state = LS_FORWARD_JUMP;
        lara_item->goal_anim_state = LS_FORWARD_JUMP;
        item->goal_anim_state = SPRINGBOARD_STATE_ON;
    }

    Item_Animate(item);
}

REGISTER_OBJECT(O_SPRINGBOARD, M_Setup)
