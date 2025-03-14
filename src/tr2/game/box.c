#include "game/box.h"

#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

bool Box_BadFloor(
    const int32_t x, const int32_t y, const int32_t z, const int32_t box_height,
    const int32_t next_height, int16_t room_num, const LOT_INFO *const lot)
{
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int16_t box_num = sector->box;
    if (box_num == NO_BOX) {
        return true;
    }

    const BOX_INFO *const box = Box_GetBox(box_num);
    if ((box->overlap_index & lot->block_mask) != 0) {
        return true;
    }

    if (box_height - box->height > lot->step
        || box_height - box->height < lot->drop) {
        return true;
    }

    if (box_height - box->height < -lot->step && box->height > next_height) {
        return true;
    }

    if (lot->fly != 0 && y > lot->fly + box->height) {
        return true;
    }

    return false;
}
