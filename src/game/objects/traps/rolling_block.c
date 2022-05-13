#include "game/objects/traps/rolling_block.h"

#include "game/items.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

void RollingBlock_Setup(OBJECT_INFO *obj)
{
    obj->initialise = RollingBlock_Initialise;
    obj->control = RollingBlock_Control;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void RollingBlock_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    Room_AlterFloorHeight(item, -WALL_L * 2);
}

void RollingBlock_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == RBS_START) {
            item->goal_anim_state = RBS_END;
            Room_AlterFloorHeight(item, WALL_L * 2);
        }
    } else if (item->current_anim_state == RBS_END) {
        item->goal_anim_state = RBS_START;
        Room_AlterFloorHeight(item, WALL_L * 2);
    }

    Item_Animate(item);

    int16_t room_num = item->room_number;
    Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_number != room_num) {
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
