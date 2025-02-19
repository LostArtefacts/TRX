#include "game/items.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"

#include <libtrx/game/lara/common.h>

typedef enum {
    // clang-format off
    COPTER_STATE_EMPTY   = 0,
    COPTER_STATE_SPIN    = 1,
    COPTER_STATE_TAKEOFF = 2,
    // clang-format on
} COPTER_STATE;

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
    const ITEM *const lara_item = Lara_GetItem();

    if (item->current_anim_state == COPTER_STATE_SPIN
        && (item->flags & IF_ONE_SHOT)) {
        item->goal_anim_state = COPTER_STATE_TAKEOFF;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    XYZ_32 pos = {
        .x = (bounds->min.x + bounds->max.x) / 2,
        .y = (bounds->min.y + bounds->max.y) / 2,
        .z = (bounds->min.z + bounds->max.z) / 2,
    };
    pos.x = lara_item->pos.x + ((pos.x - lara_item->pos.x) >> 2);
    pos.y = lara_item->pos.y + ((pos.y - lara_item->pos.y) >> 2);
    pos.z = lara_item->pos.z + ((pos.z - lara_item->pos.z) >> 2);
    Sound_Effect(SFX_HELICOPTER_LOOP, &pos, SPM_NORMAL);

    if (item->status == IS_DEACTIVATED) {
        Item_Kill(item_num);
    }
}

REGISTER_OBJECT(O_COPTER, M_Setup)
