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

bool Box_EscapeBox(ITEM *item, int16_t box_num)
{
    const BOX_INFO *const box = Box_GetBox(box_num);
    int32_t z = ((box->left + box->right) >> 1) - g_LaraItem->pos.z;
    int32_t x = ((box->top + box->bottom) >> 1) - g_LaraItem->pos.x;

    if (x > -CREATURE_ESCAPE_DIST && x < CREATURE_ESCAPE_DIST
        && z > -CREATURE_ESCAPE_DIST && z < CREATURE_ESCAPE_DIST) {
        return false;
    }

    if (((z > 0) ^ (item->pos.z > g_LaraItem->pos.z))
        && ((x > 0) ^ (item->pos.x > g_LaraItem->pos.x))) {
        return false;
    }

    return true;
}

bool Box_ValidBox(ITEM *item, int16_t zone_num, int16_t box_num)
{
    CREATURE *creature = item->data;
    const int16_t *const zone = Box_GetLotZone(&creature->lot);

    if (zone[box_num] != zone_num) {
        return false;
    }

    const BOX_INFO *const box = Box_GetBox(box_num);
    if (box->overlap_index & creature->lot.block_mask) {
        return false;
    }

    if (item->pos.z > box->left && item->pos.z < box->right
        && item->pos.x > box->top && item->pos.x < box->bottom) {
        return false;
    }

    return true;
}

TARGET_TYPE Box_CalculateTarget(XYZ_32 *target, ITEM *item, LOT_INFO *lot)
{
    int32_t left = 0;
    int32_t right = 0;
    int32_t top = 0;
    int32_t bottom = 0;

    Box_UpdateLOT(lot, BOX_MAX_EXPANSION);

    target->x = item->pos.x;
    target->y = item->pos.y;
    target->z = item->pos.z;

    int32_t box_num = item->box_num;
    if (box_num == NO_BOX) {
        return TARGET_NONE;
    }

    const BOX_INFO *box;
    int32_t prime_free = BOX_CLIP_ALL;
    do {
        box = Box_GetBox(box_num);

        if (lot->fly) {
            if (target->y > box->height - WALL_L) {
                target->y = box->height - WALL_L;
            }
        } else {
            if (target->y > box->height) {
                target->y = box->height;
            }
        }

        if (item->pos.z >= box->left && item->pos.z <= box->right
            && item->pos.x >= box->top && item->pos.x <= box->bottom) {
            left = box->left;
            right = box->right;
            top = box->top;
            bottom = box->bottom;
        } else {
            if (item->pos.z < box->left) {
                if ((prime_free & BOX_CLIP_LEFT) && item->pos.x >= box->top
                    && item->pos.x <= box->bottom) {
                    if (target->z < box->left + BOX_BIFF) {
                        target->z = box->left + BOX_BIFF;
                    }

                    if (prime_free & BOX_CLIP_SECONDARY) {
                        return TARGET_SECONDARY;
                    }

                    if (box->top > top) {
                        top = box->top;
                    }
                    if (box->bottom < bottom) {
                        bottom = box->bottom;
                    }

                    prime_free = BOX_CLIP_LEFT;
                } else if (prime_free != BOX_CLIP_LEFT) {
                    target->z = right - BOX_BIFF;
                    if (prime_free != BOX_CLIP_ALL) {
                        return TARGET_SECONDARY;
                    }

                    prime_free |= BOX_CLIP_SECONDARY;
                }
            } else if (item->pos.z > box->right) {
                if ((prime_free & BOX_CLIP_RIGHT) && item->pos.x >= box->top
                    && item->pos.x <= box->bottom) {
                    if (target->z > box->right - BOX_BIFF) {
                        target->z = box->right - BOX_BIFF;
                    }

                    if (prime_free & BOX_CLIP_SECONDARY) {
                        return TARGET_SECONDARY;
                    }

                    if (box->top > top) {
                        top = box->top;
                    }
                    if (box->bottom < bottom) {
                        bottom = box->bottom;
                    }

                    prime_free = BOX_CLIP_RIGHT;
                } else if (prime_free != BOX_CLIP_RIGHT) {
                    target->z = left + BOX_BIFF;
                    if (prime_free != BOX_CLIP_ALL) {
                        return TARGET_SECONDARY;
                    }

                    prime_free |= BOX_CLIP_SECONDARY;
                }
            }

            if (item->pos.x < box->top) {
                if ((prime_free & BOX_CLIP_TOP) && item->pos.z >= box->left
                    && item->pos.z <= box->right) {
                    if (target->x < box->top + BOX_BIFF) {
                        target->x = box->top + BOX_BIFF;
                    }

                    if (prime_free & BOX_CLIP_SECONDARY) {
                        return TARGET_SECONDARY;
                    }

                    if (box->left > left) {
                        left = box->left;
                    }
                    if (box->right < right) {
                        right = box->right;
                    }

                    prime_free = BOX_CLIP_TOP;
                } else if (prime_free != BOX_CLIP_TOP) {
                    target->x = bottom - BOX_BIFF;
                    if (prime_free != BOX_CLIP_ALL) {
                        return TARGET_SECONDARY;
                    }

                    prime_free |= BOX_CLIP_SECONDARY;
                }
            } else if (item->pos.x > box->bottom) {
                if ((prime_free & BOX_CLIP_BOTTOM) && item->pos.z >= box->left
                    && item->pos.z <= box->right) {
                    if (target->x > box->bottom - BOX_BIFF) {
                        target->x = box->bottom - BOX_BIFF;
                    }

                    if (prime_free & BOX_CLIP_SECONDARY) {
                        return TARGET_SECONDARY;
                    }

                    if (box->left > left) {
                        left = box->left;
                    }
                    if (box->right < right) {
                        right = box->right;
                    }

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
            if (prime_free & (BOX_CLIP_LEFT | BOX_CLIP_RIGHT)) {
                target->z = lot->target.z;
            } else if (!(prime_free & BOX_CLIP_SECONDARY)) {
                if (target->z < box->left + BOX_BIFF) {
                    target->z = box->left + BOX_BIFF;
                } else if (target->z > box->right - BOX_BIFF) {
                    target->z = box->right - BOX_BIFF;
                }
            }

            if (prime_free & (BOX_CLIP_TOP | BOX_CLIP_BOTTOM)) {
                target->x = lot->target.x;
            } else if (!(prime_free & BOX_CLIP_SECONDARY)) {
                if (target->x < box->top + BOX_BIFF) {
                    target->x = box->top + BOX_BIFF;
                } else if (target->x > box->bottom - BOX_BIFF) {
                    target->x = box->bottom - BOX_BIFF;
                }
            }

            target->y = lot->target.y;
            return TARGET_PRIMARY;
        }

        box_num = lot->node[box_num].exit_box;
        if (box_num != NO_BOX
            && (Box_GetBox(box_num)->overlap_index & lot->block_mask)) {
            break;
        }
    } while (box_num != NO_BOX);

    if (prime_free & (BOX_CLIP_LEFT | BOX_CLIP_RIGHT)) {
        target->z = box->left + WALL_L / 2
            + (Random_GetControl() * (box->right - box->left - WALL_L) >> 15);
    } else if (!(prime_free & BOX_CLIP_SECONDARY)) {
        if (target->z < box->left + BOX_BIFF) {
            target->z = box->left + BOX_BIFF;
        } else if (target->z > box->right - BOX_BIFF) {
            target->z = box->right - BOX_BIFF;
        }
    }

    if (prime_free & (BOX_CLIP_TOP | BOX_CLIP_BOTTOM)) {
        target->x = box->top + WALL_L / 2
            + (Random_GetControl() * (box->bottom - box->top - WALL_L) >> 15);
    } else if (!(prime_free & BOX_CLIP_SECONDARY)) {
        if (target->x < box->top + BOX_BIFF) {
            target->x = box->top + BOX_BIFF;
        } else if (target->x > box->bottom - BOX_BIFF) {
            target->x = box->bottom - BOX_BIFF;
        }
    }

    if (!lot->fly) {
        target->y = box->height;
    } else {
        target->y = box->height - STEP_L * 3 / 2;
    }

    return TARGET_NONE;
}

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
