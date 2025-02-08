#include "game/box.h"

#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define BOX_OVERLAP_BITS 0x3FFF
#define BOX_SEARCH_NUM 0x7FFF
#define BOX_END_BIT 0x8000
#define BOX_NUM_BITS (~BOX_END_BIT) // = 0x7FFF
#define BOX_STALK_DIST 3 // tiles
#define BOX_ESCAPE_DIST 5 // tiles
#define BOX_MAX_EXPANSION 5

#define BOX_BIFF (WALL_L / 2) // = 0x200 = 512
#define BOX_CLIP_LEFT 1
#define BOX_CLIP_RIGHT 2
#define BOX_CLIP_TOP 4
#define BOX_CLIP_BOTTOM 8
#define BOX_CLIP_ALL                                                           \
    (BOX_CLIP_LEFT | BOX_CLIP_RIGHT | BOX_CLIP_TOP | BOX_CLIP_BOTTOM) // = 15
#define BOX_CLIP_SECONDARY 16

int32_t Box_SearchLOT(LOT_INFO *const lot, const int32_t expansion)
{
    const bool flip_status = Room_GetFlipStatus();
    int16_t *zone;
    if (lot->fly) {
        zone = g_FlyZone[flip_status];
    } else {
        zone = g_GroundZone[BOX_ZONE(lot->step)][flip_status];
    }

    const int16_t search_zone = zone[lot->head];
    for (int32_t i = 0; i < expansion; i++) {
        if (lot->head == NO_BOX) {
            lot->tail = NO_BOX;
            return false;
        }

        BOX_NODE *node = &lot->node[lot->head];
        const BOX_INFO *box = &g_Boxes[lot->head];

        bool done = false;
        int32_t index = box->overlap_index & BOX_OVERLAP_BITS;
        while (!done) {
            int16_t box_num = g_Overlap[index++];
            if ((box_num & BOX_END_BIT) != 0) {
                done = true;
                box_num &= BOX_NUM_BITS;
            }

            if (search_zone != zone[box_num]) {
                continue;
            }

            const int32_t change = g_Boxes[box_num].height - box->height;
            if (change > lot->step || change < lot->drop) {
                continue;
            }

            BOX_NODE *const expand = &lot->node[box_num];
            if ((node->search_num & BOX_SEARCH_NUM)
                < (expand->search_num & BOX_SEARCH_NUM)) {
                continue;
            }

            if ((node->search_num & BOX_BLOCKED_SEARCH) != 0) {
                if ((expand->search_num & BOX_SEARCH_NUM)
                    == (node->search_num & BOX_SEARCH_NUM)) {
                    continue;
                }
                expand->search_num = node->search_num;
            } else {
                if ((expand->search_num & BOX_SEARCH_NUM)
                        == (node->search_num & BOX_SEARCH_NUM)
                    && (expand->search_num & BOX_BLOCKED_SEARCH) == 0) {
                    continue;
                }

                if ((g_Boxes[box_num].overlap_index & lot->block_mask) != 0) {
                    expand->search_num = node->search_num | BOX_BLOCKED_SEARCH;
                } else {
                    expand->search_num = node->search_num;
                    expand->exit_box = lot->head;
                }
            }

            if (expand->next_expansion == NO_BOX && box_num != lot->tail) {
                lot->node[lot->tail].next_expansion = box_num;
                lot->tail = box_num;
            }
        }

        lot->head = node->next_expansion;
        node->next_expansion = NO_BOX;
    }

    return true;
}

int32_t Box_UpdateLOT(LOT_INFO *const lot, const int32_t expansion)
{
    if (lot->required_box == NO_BOX || lot->required_box == lot->target_box) {
        goto end;
    }

    lot->target_box = lot->required_box;
    BOX_NODE *const expand = &lot->node[lot->target_box];
    if (expand->next_expansion == NO_BOX && lot->tail != lot->target_box) {
        expand->next_expansion = lot->head;
        if (lot->head == NO_BOX) {
            lot->tail = lot->target_box;
        }
        lot->head = lot->target_box;
    }
    lot->search_num++;
    expand->search_num = lot->search_num;
    expand->exit_box = NO_BOX;

end:
    return Box_SearchLOT(lot, expansion);
}

void Box_TargetBox(LOT_INFO *const lot, const int16_t box_num)
{
    const BOX_INFO *const box = &g_Boxes[box_num & BOX_NUM_BITS];

    lot->target.z = ((box->right - box->left - 1) >> (15 - WALL_SHIFT))
            * Random_GetControl()
        + (box->left << WALL_SHIFT) + WALL_L / 2;
    lot->target.x = ((box->bottom - box->top - 1) >> (15 - WALL_SHIFT))
            * Random_GetControl()
        + (box->top << WALL_SHIFT) + WALL_L / 2;
    lot->required_box = box_num & BOX_NUM_BITS;

    if (lot->fly != 0) {
        lot->target.y = box->height - STEP_L * 3 / 2;
    } else {
        lot->target.y = box->height;
    }
}

int32_t Box_StalkBox(
    const ITEM *const item, const ITEM *const enemy, const int16_t box_num)
{
    const BOX_INFO *const box = &g_Boxes[box_num];

    const int32_t z =
        ((box->left + box->right) << (WALL_SHIFT - 1)) - enemy->pos.z;
    const int32_t x =
        ((box->top + box->bottom) << (WALL_SHIFT - 1)) - enemy->pos.x;

    const int32_t x_range = (box->bottom - box->top + BOX_STALK_DIST)
        << WALL_SHIFT;
    const int32_t z_range = (box->right - box->left + BOX_STALK_DIST)
        << WALL_SHIFT;
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
        ? (item->pos.x > x ? DIR_SOUTH : DIR_EAST)
        : (item->pos.x > x ? DIR_WEST : DIR_NORTH);

    return enemy_quad != baddie_quad || ABS(enemy_quad - box_quad) != 2;
}

int32_t Box_EscapeBox(
    const ITEM *const item, const ITEM *const enemy, const int16_t box_num)
{
    const BOX_INFO *const box = &g_Boxes[box_num];
    const int32_t x =
        ((box->bottom + box->top) << (WALL_SHIFT - 1)) - enemy->pos.x;
    const int32_t z =
        ((box->left + box->right) << (WALL_SHIFT - 1)) - enemy->pos.z;

    const int32_t x_range = BOX_ESCAPE_DIST << WALL_SHIFT;
    const int32_t z_range = BOX_ESCAPE_DIST << WALL_SHIFT;
    if (x > -x_range && x < x_range && z > -z_range && z < z_range) {
        return false;
    }

    return ((z > 0) == (item->pos.z > enemy->pos.z))
        || ((x > 0) == (item->pos.x > enemy->pos.x));
}

int32_t Box_ValidBox(
    const ITEM *const item, const int16_t zone_num, const int16_t box_num)
{
    const CREATURE *const creature = item->data;
    const bool flip_status = Room_GetFlipStatus();
    int16_t *zone;
    if (creature->lot.fly) {
        zone = g_FlyZone[flip_status];
    } else {
        zone = g_GroundZone[BOX_ZONE(creature->lot.step)][flip_status];
    }

    if (zone[box_num] != zone_num) {
        return false;
    }

    const BOX_INFO *const box = &g_Boxes[box_num];
    if ((creature->lot.block_mask & box->overlap_index) != 0) {
        return false;
    }

    return !(
        item->pos.z > (box->left << WALL_SHIFT)
        && item->pos.z < (box->right << WALL_SHIFT)
        && item->pos.x > (box->top << WALL_SHIFT)
        && item->pos.x < (box->bottom << WALL_SHIFT));
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

    BOX_INFO *box = nullptr;
    int32_t box_left = 0;
    int32_t box_right = 0;
    int32_t box_top = 0;
    int32_t box_bottom = 0;

    int32_t prime_free = BOX_CLIP_ALL;
    do {
        box = &g_Boxes[box_num];
        if (lot->fly != 0) {
            CLAMPG(target->y, box->height - WALL_L);
        } else {
            CLAMPG(target->y, box->height);
        }

        box_left = (int32_t)box->left << WALL_SHIFT;
        box_right = ((int32_t)box->right << WALL_SHIFT) - 1;
        box_top = (int32_t)box->top << WALL_SHIFT;
        box_bottom = ((int32_t)box->bottom << WALL_SHIFT) - 1;

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
            && (g_Boxes[box_num].overlap_index & lot->block_mask) != 0) {
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

int32_t Box_BadFloor(
    const int32_t x, const int32_t y, const int32_t z, const int32_t box_height,
    const int32_t next_height, int16_t room_num, const LOT_INFO *const lot)
{
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int16_t box_num = sector->box;
    if (box_num == NO_BOX) {
        return true;
    }

    const BOX_INFO *const box = &g_Boxes[box_num];
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
