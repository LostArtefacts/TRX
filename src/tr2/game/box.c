#include "game/box.h"

#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define BOX_MAX_EXPANSION 5

#define BOX_BIFF (WALL_L / 2) // = 0x200 = 512
#define BOX_CLIP_LEFT 1
#define BOX_CLIP_RIGHT 2
#define BOX_CLIP_TOP 4
#define BOX_CLIP_BOTTOM 8
#define BOX_CLIP_ALL                                                           \
    (BOX_CLIP_LEFT | BOX_CLIP_RIGHT | BOX_CLIP_TOP | BOX_CLIP_BOTTOM) // = 15
#define BOX_CLIP_SECONDARY 16

bool Box_StalkBox(
    const ITEM *const item, const ITEM *const enemy, const int16_t box_num)
{
    const BOX_INFO *const box = Box_GetBox(box_num);
    // TODO: determine if +1 on box right/bottom is essential
    const int32_t z = ((box->left + box->right + 1) >> 1) - enemy->pos.z;
    const int32_t x = ((box->top + box->bottom + 1) >> 1) - enemy->pos.x;

    const int32_t x_range = box->bottom + 1 - box->top + CREATURE_STALK_DIST;
    const int32_t z_range = box->right + 1 - box->left + CREATURE_STALK_DIST;
    if (x > x_range || x < -x_range || z > z_range || z < -z_range) {
        return false;
    }

    const int32_t enemy_quad = (enemy->rot.y >> 14) + 2;
    const int32_t box_quad = (z > 0) ? ((x > 0) ? DIR_SOUTH : DIR_EAST)
                                     : ((x > 0) ? DIR_WEST : DIR_NORTH);
    if (enemy_quad == box_quad) {
        return false;
    }

    const int32_t baddie_quad = item->pos.z > enemy->pos.z
        ? (item->pos.x > enemy->pos.x ? DIR_SOUTH : DIR_EAST)
        : (item->pos.x > enemy->pos.x ? DIR_WEST : DIR_NORTH);

    return enemy_quad != baddie_quad || ABS(enemy_quad - box_quad) != 2;
}

bool Box_EscapeBox(
    const ITEM *const item, const ITEM *const enemy, const int16_t box_num)
{
    const BOX_INFO *const box = Box_GetBox(box_num);
    // TODO: determine if +1 on box right/bottom is essential
    const int32_t x = ((box->top + box->bottom + 1) >> 1) - enemy->pos.x;
    const int32_t z = ((box->left + box->right + 1) >> 1) - enemy->pos.z;

    if (x > -CREATURE_ESCAPE_DIST && x < CREATURE_ESCAPE_DIST
        && z > -CREATURE_ESCAPE_DIST && z < CREATURE_ESCAPE_DIST) {
        return false;
    }

    return ((z > 0) == (item->pos.z > enemy->pos.z))
        || ((x > 0) == (item->pos.x > enemy->pos.x));
}

bool Box_ValidBox(
    const ITEM *const item, const int16_t zone_num, const int16_t box_num)
{
    const CREATURE *const creature = item->data;
    const int16_t *const zone = Box_GetLotZone(&creature->lot);

    if (zone[box_num] != zone_num) {
        return false;
    }

    const BOX_INFO *const box = Box_GetBox(box_num);
    if ((creature->lot.block_mask & box->overlap_index) != 0) {
        return false;
    }

    // TODO: determine if +1 on box right/bottom is essential
    return !(
        item->pos.z > box->left && item->pos.z < box->right + 1
        && item->pos.x > box->top && item->pos.x < box->bottom + 1);
}

TARGET_TYPE Box_CalculateTarget(
    XYZ_32 *const target, const ITEM *const item, LOT_INFO *const lot)
{
    Box_UpdateLOT(lot, BOX_MAX_EXPANSION);

    *target = item->pos;

    int32_t box_num = item->box_num;
    if (box_num == NO_BOX) {
        return TARGET_NONE;
    }

    int32_t bottom = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t left = 0;

    const BOX_INFO *box = nullptr;
    int32_t box_left = 0;
    int32_t box_right = 0;
    int32_t box_top = 0;
    int32_t box_bottom = 0;

    int32_t prime_free = BOX_CLIP_ALL;
    do {
        box = Box_GetBox(box_num);
        if (lot->fly != 0) {
            CLAMPG(target->y, box->height - WALL_L);
        } else {
            CLAMPG(target->y, box->height);
        }

        box_left = box->left;
        box_right = box->right;
        box_top = box->top;
        box_bottom = box->bottom;

        if (item->pos.z >= box_left && item->pos.z <= box_right
            && item->pos.x >= box_top && item->pos.x <= box_bottom) {
            left = box_left;
            right = box_right;
            top = box_top;
            bottom = box_bottom;
        } else {
            if (item->pos.z < box_left) {
                if ((prime_free & BOX_CLIP_LEFT) != 0 && item->pos.x >= box_top
                    && item->pos.x <= box_bottom) {
                    CLAMPL(target->z, box_left + BOX_BIFF);
                    if ((prime_free & BOX_CLIP_SECONDARY) != 0) {
                        return TARGET_SECONDARY;
                    }
                    CLAMPL(top, box_top);
                    CLAMPG(bottom, box_bottom);
                    prime_free = BOX_CLIP_LEFT;
                } else if (prime_free != BOX_CLIP_LEFT) {
                    target->z = right - BOX_BIFF;
                    if (prime_free != BOX_CLIP_ALL) {
                        return TARGET_SECONDARY;
                    }
                    prime_free |= BOX_CLIP_SECONDARY;
                }
            } else if (item->pos.z > box_right) {
                if ((prime_free & BOX_CLIP_RIGHT) != 0 && item->pos.x >= box_top
                    && item->pos.x <= box_bottom) {
                    CLAMPG(target->z, box_right - BOX_BIFF);
                    if ((prime_free & BOX_CLIP_SECONDARY) != 0) {
                        return TARGET_SECONDARY;
                    }
                    CLAMPL(top, box_top);
                    CLAMPG(bottom, box_bottom);
                    prime_free = BOX_CLIP_RIGHT;
                } else if (prime_free != BOX_CLIP_RIGHT) {
                    target->z = left + BOX_BIFF;
                    if (prime_free != BOX_CLIP_ALL) {
                        return TARGET_SECONDARY;
                    }
                    prime_free |= BOX_CLIP_SECONDARY;
                }
            }

            if (item->pos.x < box_top) {
                if ((prime_free & BOX_CLIP_TOP) != 0
                    && (item->pos.z >= box_left) && item->pos.z <= box_right) {
                    CLAMPL(target->x, box_top + BOX_BIFF);
                    if ((prime_free & BOX_CLIP_SECONDARY) != 0) {
                        return TARGET_SECONDARY;
                    }
                    CLAMPL(left, box_left);
                    CLAMPG(right, box_right);
                    prime_free = BOX_CLIP_TOP;
                } else if (prime_free != BOX_CLIP_TOP) {
                    target->x = bottom - BOX_BIFF;
                    if (prime_free != BOX_CLIP_ALL) {
                        return TARGET_SECONDARY;
                    }
                    prime_free |= BOX_CLIP_SECONDARY;
                }
            } else if (item->pos.x > box_bottom) {
                if ((prime_free & BOX_CLIP_BOTTOM) != 0
                    && item->pos.z >= box_left && item->pos.z <= box_right) {
                    CLAMPG(target->x, box_bottom - BOX_BIFF);
                    if ((prime_free & BOX_CLIP_SECONDARY) != 0) {
                        return TARGET_SECONDARY;
                    }
                    CLAMPL(left, box_left);
                    CLAMPG(right, box_right);
                    prime_free = BOX_CLIP_BOTTOM;
                } else if (prime_free != BOX_CLIP_BOTTOM) {
                    target->x = top + BOX_BIFF;
                    if (prime_free != BOX_CLIP_ALL) {
                        return TARGET_SECONDARY;
                    }
                    prime_free |= BOX_CLIP_SECONDARY;
                }
            }
        }

        if (box_num == lot->target_box) {
            if ((prime_free & (BOX_CLIP_LEFT | BOX_CLIP_RIGHT)) != 0) {
                target->z = lot->target.z;
            } else if ((prime_free & BOX_CLIP_SECONDARY) == 0) {
                CLAMP(target->z, box_left + BOX_BIFF, box_right - BOX_BIFF);
            }

            if ((prime_free & (BOX_CLIP_TOP | BOX_CLIP_BOTTOM)) != 0) {
                target->x = lot->target.x;
            } else if ((prime_free & BOX_CLIP_SECONDARY) == 0) {
                CLAMP(target->x, box_top + BOX_BIFF, box_bottom - BOX_BIFF);
            }

            target->y = lot->target.y;
            return TARGET_PRIMARY;
        }

        box_num = lot->node[box_num].exit_box;
        if (box_num != NO_BOX
            && (Box_GetBox(box_num)->overlap_index & lot->block_mask) != 0) {
            break;
        }
    } while (box_num != NO_BOX);

    if ((prime_free & (BOX_CLIP_LEFT | BOX_CLIP_RIGHT)) != 0) {
        target->z = box_left + WALL_L / 2
            + (((box_right - box_left - WALL_L) * Random_GetControl()) >> 15);
    } else if ((prime_free & BOX_CLIP_SECONDARY) == 0) {
        CLAMP(target->z, box_left + BOX_BIFF, box_right - BOX_BIFF);
    }

    if ((prime_free & (BOX_CLIP_TOP | BOX_CLIP_BOTTOM)) != 0) {
        target->x = box_top + WALL_L / 2
            + (((box_bottom - box_top - WALL_L) * Random_GetControl()) >> 15);
    } else if ((prime_free & BOX_CLIP_SECONDARY) == 0) {
        CLAMP(target->x, box_top + BOX_BIFF, box_bottom - BOX_BIFF);
    }

    if (lot->fly != 0) {
        target->y = box->height - STEP_L * 3 / 2;
    } else {
        target->y = box->height;
    }

    return TARGET_NONE;
}

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
