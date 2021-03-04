#include "game/control.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/moveblock.h"
#include "game/types.h"
#include "game/vars.h"
#include "util.h"

// original name: InitialiseMovingBlock
void InitialiseMovableBlock(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    if (item->status != IS_INVISIBLE) {
        AlterFloorHeight(item, -1024);
    }
}

// original name: MovableBlock
void MovableBlockControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->flags & IF_ONESHOT) {
        AlterFloorHeight(item, 1024);
        KillItem(item_num);
        return;
    }

    AnimateItem(item);

    int16_t room_num = item->room_number;
    FLOOR_INFO* floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    int32_t height = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    if (item->pos.y < height) {
        item->gravity_status = 1;
    } else if (item->gravity_status) {
        item->gravity_status = 0;
        item->pos.y = height;
        item->status = IS_DEACTIVATED;
        FxDinoStomp(item);
        SoundEffect(70, &item->pos, 0);
    }

    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        item->status = IS_NOT_ACTIVE;
        RemoveActiveItem(item_num);
        AlterFloorHeight(item, -1024);

        room_num = item->room_number;
        floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        TestTriggers(TriggerIndex, 1);
    }
}

void T1MInjectGameMoveBlock()
{
    INJECT(0x0042B430, InitialiseMovableBlock);
    INJECT(0x0042B460, MovableBlockControl);
}
