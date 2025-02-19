#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/room.h"

#define ICICLE_HIT_DAMAGE 200

typedef enum {
    // clang-format off
    ICICLE_EMPTY = 0,
    ICICLE_BREAK = 1,
    ICICLE_FALL  = 2,
    ICICLE_LAND  = 3,
    // clang-format on
} ICICLE_STATE;

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    switch (item->current_anim_state) {
    case ICICLE_BREAK:
        item->goal_anim_state = ICICLE_FALL;
        break;

    case ICICLE_FALL:
        if (!item->gravity) {
            item->gravity = 1;
            item->fall_speed = 50;
        }
        if (item->touch_bits != 0) {
            Lara_TakeDamage(ICICLE_HIT_DAMAGE, true);
        }
        break;

    case ICICLE_LAND:
        item->gravity = 0;
        break;
    }

    Item_Animate(item);
    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    item->floor = height;
    if (item->current_anim_state == ICICLE_FALL && item->pos.y >= height) {
        item->gravity = 0;
        item->goal_anim_state = ICICLE_LAND;
        item->pos.y = height;
        item->fall_speed = 0;
        item->mesh_bits = 0b00101011;
    }
}

REGISTER_OBJECT(O_ICICLE, M_Setup)
