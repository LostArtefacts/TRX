#include "game/items.h"
#include "game/room.h"
#include "global/const.h"

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Room_AlterFloorHeight(item, -WALL_L * 2);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == SPS_START) {
            item->goal_anim_state = SPS_END;
            Room_AlterFloorHeight(item, WALL_L * 2);
        }
    } else if (item->current_anim_state == SPS_END) {
        item->goal_anim_state = SPS_START;
        Room_AlterFloorHeight(item, WALL_L * 2);
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_ACTIVE;
        Room_AlterFloorHeight(item, -WALL_L * 2);
        item->pos.x &= -WALL_L;
        item->pos.x += WALL_L / 2;
        item->pos.z &= -WALL_L;
        item->pos.z += WALL_L / 2;
    }
}

REGISTER_OBJECT(O_SLIDING_PILLAR, M_Setup)
