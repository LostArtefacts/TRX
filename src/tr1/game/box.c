#include "game/box.h"

#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

bool Box_SearchLOT(LOT_INFO *lot, int32_t expansion)
{
    const bool flip_status = Room_GetFlipStatus();
    int16_t *zone;
    if (lot->fly) {
        zone = g_FlyZone[flip_status];
    } else if (lot->step == STEP_L) {
        zone = g_GroundZone[flip_status];
    } else {
        zone = g_GroundZone2[flip_status];
    }

    int16_t search_zone = zone[lot->head];
    for (int i = 0; i < expansion; i++) {
        if (lot->head == NO_BOX) {
            return false;
        }

        BOX_NODE *node = &lot->node[lot->head];
        BOX_INFO *box = &g_Boxes[lot->head];

        int done = 0;
        int index = box->overlap_index & OVERLAP_INDEX;
        do {
            int16_t box_num = g_Overlap[index++];
            if (box_num & END_BIT) {
                done = 1;
                box_num &= BOX_NUMBER;
            }

            if (search_zone != zone[box_num]) {
                continue;
            }

            int change = g_Boxes[box_num].height - box->height;
            if (change > lot->step || change < lot->drop) {
                continue;
            }

            BOX_NODE *expand = &lot->node[box_num];
            if ((node->search_num & SEARCH_NUMBER)
                < (expand->search_num & SEARCH_NUMBER)) {
                continue;
            }

            if (node->search_num & BLOCKED_SEARCH) {
                if ((node->search_num & SEARCH_NUMBER)
                    == (expand->search_num & SEARCH_NUMBER)) {
                    continue;
                }
                expand->search_num = node->search_num;
            } else {
                if ((node->search_num & SEARCH_NUMBER)
                        == (expand->search_num & SEARCH_NUMBER)
                    && !(expand->search_num & BLOCKED_SEARCH)) {
                    continue;
                }

                if (g_Boxes[box_num].overlap_index & lot->block_mask) {
                    expand->search_num = node->search_num | BLOCKED_SEARCH;
                } else {
                    expand->search_num = node->search_num;
                    expand->exit_box = lot->head;
                }
            }

            if (expand->next_expansion == NO_BOX && box_num != lot->tail) {
                lot->node[lot->tail].next_expansion = box_num;
                lot->tail = box_num;
            }
        } while (!done);

        lot->head = node->next_expansion;
        node->next_expansion = NO_BOX;
    }

    return true;
}

bool Box_UpdateLOT(LOT_INFO *lot, int32_t expansion)
{
    if (lot->required_box != NO_BOX && lot->required_box != lot->target_box) {
        lot->target_box = lot->required_box;

        BOX_NODE *expand = &lot->node[lot->target_box];
        if (expand->next_expansion == NO_BOX && lot->tail != lot->target_box) {
            expand->next_expansion = lot->head;

            if (lot->head == NO_BOX) {
                lot->tail = lot->target_box;
            }

            lot->head = lot->target_box;
        }

        expand->search_num = ++lot->search_num;
        expand->exit_box = NO_BOX;
    }

    return Box_SearchLOT(lot, expansion);
}

void Box_TargetBox(LOT_INFO *lot, int16_t box_num)
{
    box_num &= BOX_NUMBER;

    BOX_INFO *box = &g_Boxes[box_num];

    lot->target.z = box->left + WALL_L / 2
        + (Random_GetControl() * (box->right - box->left - WALL_L) >> 15);
    lot->target.x = box->top + WALL_L / 2
        + (Random_GetControl() * (box->bottom - box->top - WALL_L) >> 15);
    lot->required_box = box_num;

    if (lot->fly) {
        lot->target.y = box->height - STEP_L * 3 / 2;
    } else {
        lot->target.y = box->height;
    }
}

bool Box_StalkBox(ITEM *item, int16_t box_num)
{
    BOX_INFO *box = &g_Boxes[box_num];
    int32_t z = ((box->left + box->right) >> 1) - g_LaraItem->pos.z;
    int32_t x = ((box->top + box->bottom) >> 1) - g_LaraItem->pos.x;

    if (x > STALK_DIST || x < -STALK_DIST || z > STALK_DIST
        || z < -STALK_DIST) {
        return false;
    }

    int enemy_quad = (g_LaraItem->rot.y >> 14) + 2;
    int box_quad = (z > 0) ? ((x > 0) ? 2 : 1) : ((x > 0) ? 3 : 0);

    if (enemy_quad == box_quad) {
        return false;
    }

    int baddie_quad = (item->pos.z > g_LaraItem->pos.z)
        ? ((item->pos.x > g_LaraItem->pos.x) ? 2 : 1)
        : ((item->pos.x > g_LaraItem->pos.x) ? 3 : 0);

    if (enemy_quad == baddie_quad && ABS(enemy_quad - box_quad) == 2) {
        return false;
    }

    return true;
}

bool Box_EscapeBox(ITEM *item, int16_t box_num)
{
    BOX_INFO *box = &g_Boxes[box_num];
    int32_t z = ((box->left + box->right) >> 1) - g_LaraItem->pos.z;
    int32_t x = ((box->top + box->bottom) >> 1) - g_LaraItem->pos.x;

    if (x > -ESCAPE_DIST && x < ESCAPE_DIST && z > -ESCAPE_DIST
        && z < ESCAPE_DIST) {
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

    const bool flip_status = Room_GetFlipStatus();
    int16_t *zone;
    if (creature->lot.fly) {
        zone = g_FlyZone[flip_status];
    } else if (creature->lot.step == STEP_L) {
        zone = g_GroundZone[flip_status];
    } else {
        zone = g_GroundZone2[flip_status];
    }

    if (zone[box_num] != zone_num) {
        return false;
    }

    BOX_INFO *box = &g_Boxes[box_num];
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

    Box_UpdateLOT(lot, MAX_EXPANSION);

    target->x = item->pos.x;
    target->y = item->pos.y;
    target->z = item->pos.z;

    int32_t box_num = item->box_num;
    if (box_num == NO_BOX) {
        return TARGET_NONE;
    }

    BOX_INFO *box;
    int32_t prime_free = ALL_CLIP;
    do {
        box = &g_Boxes[box_num];

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
                if ((prime_free & CLIP_LEFT) && item->pos.x >= box->top
                    && item->pos.x <= box->bottom) {
                    if (target->z < box->left + BIFF) {
                        target->z = box->left + BIFF;
                    }

                    if (prime_free & SECONDARY_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    if (box->top > top) {
                        top = box->top;
                    }
                    if (box->bottom < bottom) {
                        bottom = box->bottom;
                    }

                    prime_free = CLIP_LEFT;
                } else if (prime_free != CLIP_LEFT) {
                    target->z = right - BIFF;
                    if (prime_free != ALL_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    prime_free |= SECONDARY_CLIP;
                }
            } else if (item->pos.z > box->right) {
                if ((prime_free & CLIP_RIGHT) && item->pos.x >= box->top
                    && item->pos.x <= box->bottom) {
                    if (target->z > box->right - BIFF) {
                        target->z = box->right - BIFF;
                    }

                    if (prime_free & SECONDARY_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    if (box->top > top) {
                        top = box->top;
                    }
                    if (box->bottom < bottom) {
                        bottom = box->bottom;
                    }

                    prime_free = CLIP_RIGHT;
                } else if (prime_free != CLIP_RIGHT) {
                    target->z = left + BIFF;
                    if (prime_free != ALL_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    prime_free |= SECONDARY_CLIP;
                }
            }

            if (item->pos.x < box->top) {
                if ((prime_free & CLIP_TOP) && item->pos.z >= box->left
                    && item->pos.z <= box->right) {
                    if (target->x < box->top + BIFF) {
                        target->x = box->top + BIFF;
                    }

                    if (prime_free & SECONDARY_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    if (box->left > left) {
                        left = box->left;
                    }
                    if (box->right < right) {
                        right = box->right;
                    }

                    prime_free = CLIP_TOP;
                } else if (prime_free != CLIP_TOP) {
                    target->x = bottom - BIFF;
                    if (prime_free != ALL_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    prime_free |= SECONDARY_CLIP;
                }
            } else if (item->pos.x > box->bottom) {
                if ((prime_free & CLIP_BOTTOM) && item->pos.z >= box->left
                    && item->pos.z <= box->right) {
                    if (target->x > box->bottom - BIFF) {
                        target->x = box->bottom - BIFF;
                    }

                    if (prime_free & SECONDARY_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    if (box->left > left) {
                        left = box->left;
                    }
                    if (box->right < right) {
                        right = box->right;
                    }

                    prime_free = CLIP_BOTTOM;
                } else if (prime_free != CLIP_BOTTOM) {
                    target->x = top + BIFF;
                    if (prime_free != ALL_CLIP) {
                        return TARGET_SECONDARY;
                    }

                    prime_free |= SECONDARY_CLIP;
                }
            }
        }

        if (box_num == lot->target_box) {
            if (prime_free & (CLIP_LEFT | CLIP_RIGHT)) {
                target->z = lot->target.z;
            } else if (!(prime_free & SECONDARY_CLIP)) {
                if (target->z < box->left + BIFF) {
                    target->z = box->left + BIFF;
                } else if (target->z > box->right - BIFF) {
                    target->z = box->right - BIFF;
                }
            }

            if (prime_free & (CLIP_TOP | CLIP_BOTTOM)) {
                target->x = lot->target.x;
            } else if (!(prime_free & SECONDARY_CLIP)) {
                if (target->x < box->top + BIFF) {
                    target->x = box->top + BIFF;
                } else if (target->x > box->bottom - BIFF) {
                    target->x = box->bottom - BIFF;
                }
            }

            target->y = lot->target.y;
            return TARGET_PRIMARY;
        }

        box_num = lot->node[box_num].exit_box;
        if (box_num != NO_BOX
            && (g_Boxes[box_num].overlap_index & lot->block_mask)) {
            break;
        }
    } while (box_num != NO_BOX);

    if (prime_free & (CLIP_LEFT | CLIP_RIGHT)) {
        target->z = box->left + WALL_L / 2
            + (Random_GetControl() * (box->right - box->left - WALL_L) >> 15);
    } else if (!(prime_free & SECONDARY_CLIP)) {
        if (target->z < box->left + BIFF) {
            target->z = box->left + BIFF;
        } else if (target->z > box->right - BIFF) {
            target->z = box->right - BIFF;
        }
    }

    if (prime_free & (CLIP_TOP | CLIP_BOTTOM)) {
        target->x = box->top + WALL_L / 2
            + (Random_GetControl() * (box->bottom - box->top - WALL_L) >> 15);
    } else if (!(prime_free & SECONDARY_CLIP)) {
        if (target->x < box->top + BIFF) {
            target->x = box->top + BIFF;
        } else if (target->x > box->bottom - BIFF) {
            target->x = box->bottom - BIFF;
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

    if (g_Boxes[sector->box].overlap_index & lot->block_mask) {
        return true;
    }

    const int32_t height = g_Boxes[sector->box].height;
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
