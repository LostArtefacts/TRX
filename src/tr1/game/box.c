#include "game/box.h"

#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

bool Box_BadFloor(
    int32_t x, int32_t y, int32_t z, int16_t box_height, int16_t next_height,
    int16_t room_num, LOT_INFO *lot)
{
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    if (sector->box == NO_BOX) {
        return true;
    }

    const BOX_INFO *const box = Box_GetBox(sector->box);
    if (box->overlap_index & lot->block_mask) {
        return true;
    }

    const int32_t height = box->height;
    if (box_height - height > lot->step || box_height - height < lot->drop) {
        return true;
    }

    if (box_height - height < -lot->step && height > next_height) {
        return true;
    }

    if (lot->fly && y > height + lot->fly) {
        return true;
    }

    return false;
}
