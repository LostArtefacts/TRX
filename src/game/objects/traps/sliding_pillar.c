#include "game/objects/traps/sliding_pillar.h"

#include "game/items.h"
#include "game/room.h"
#include "global/const.h"

void SlidingPillar_Setup(OBJECT_INFO *obj)
{
    obj->initialise = SlidingPillar_Initialise;
    obj->control = SlidingPillar_Control;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void SlidingPillar_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    Room_AlterFloorHeight(item, -WALL_L * 2);
}

void SlidingPillar_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
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
