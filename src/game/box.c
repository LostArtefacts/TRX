#include "game/box.h"

#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

bool Box_SearchLOT(LOT_INFO *LOT, int32_t expansion)
{
    int16_t *zone;
    if (LOT->fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else if (LOT->step == STEP_L) {
        zone = g_GroundZone[g_FlipStatus];
    } else {
        zone = g_GroundZone2[g_FlipStatus];
    }

    int16_t search_zone = zone[LOT->head];
    for (int i = 0; i < expansion; i++) {
        if (LOT->head == NO_BOX) {
            return false;
        }

        BOX_NODE *node = &LOT->node[LOT->head];
        BOX_INFO *box = &g_Boxes[LOT->head];

        int done = 0;
        int index = box->overlap_index & OVERLAP_INDEX;
        do {
            int16_t box_number = g_Overlap[index++];
            if (box_number & END_BIT) {
                done = 1;
                box_number &= BOX_NUMBER;
            }

            if (search_zone != zone[box_number]) {
                continue;
            }

            int change = g_Boxes[box_number].height - box->height;
            if (change > LOT->step || change < LOT->drop) {
                continue;
            }

            BOX_NODE *expand = &LOT->node[box_number];
            if ((node->search_number & SEARCH_NUMBER)
                < (expand->search_number & SEARCH_NUMBER)) {
                continue;
            }

            if (node->search_number & BLOCKED_SEARCH) {
                if ((node->search_number & SEARCH_NUMBER)
                    == (expand->search_number & SEARCH_NUMBER)) {
                    continue;
                }
                expand->search_number = node->search_number;
            } else {
                if ((node->search_number & SEARCH_NUMBER)
                        == (expand->search_number & SEARCH_NUMBER)
                    && !(expand->search_number & BLOCKED_SEARCH)) {
                    continue;
                }

                if (g_Boxes[box_number].overlap_index & LOT->block_mask) {
                    expand->search_number =
                        node->search_number | BLOCKED_SEARCH;
                } else {
                    expand->search_number = node->search_number;
                    expand->exit_box = LOT->head;
                }
            }

            if (expand->next_expansion == NO_BOX && box_number != LOT->tail) {
                LOT->node[LOT->tail].next_expansion = box_number;
                LOT->tail = box_number;
            }
        } while (!done);

        LOT->head = node->next_expansion;
        node->next_expansion = NO_BOX;
    }

    return true;
}

bool Box_UpdateLOT(LOT_INFO *LOT, int32_t expansion)
{
    if (LOT->required_box != NO_BOX && LOT->required_box != LOT->target_box) {
        LOT->target_box = LOT->required_box;

        BOX_NODE *expand = &LOT->node[LOT->target_box];
        if (expand->next_expansion == NO_BOX && LOT->tail != LOT->target_box) {
            expand->next_expansion = LOT->head;

            if (LOT->head == NO_BOX) {
                LOT->tail = LOT->target_box;
            }

            LOT->head = LOT->target_box;
        }

        expand->search_number = ++LOT->search_number;
        expand->exit_box = NO_BOX;
    }

    return Box_SearchLOT(LOT, expansion);
}

void Box_TargetBox(LOT_INFO *LOT, int16_t box_number)
{
    box_number &= BOX_NUMBER;

    BOX_INFO *box = &g_Boxes[box_number];

    LOT->target.z = box->left + WALL_L / 2
        + (Random_GetControl() * (box->right - box->left - WALL_L) >> 15);
    LOT->target.x = box->top + WALL_L / 2
        + (Random_GetControl() * (box->bottom - box->top - WALL_L) >> 15);
    LOT->required_box = box_number;

    if (LOT->fly) {
        LOT->target.y = box->height - STEP_L * 3 / 2;
    } else {
        LOT->target.y = box->height;
    }
}

bool Box_StalkBox(ITEM_INFO *item, int16_t box_number)
{
    BOX_INFO *box = &g_Boxes[box_number];
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

bool Box_EscapeBox(ITEM_INFO *item, int16_t box_number)
{
    BOX_INFO *box = &g_Boxes[box_number];
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

bool Box_ValidBox(ITEM_INFO *item, int16_t zone_number, int16_t box_number)
{
    CREATURE_INFO *creature = item->data;

    int16_t *zone;
    if (creature->LOT.fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else if (creature->LOT.step == STEP_L) {
        zone = g_GroundZone[g_FlipStatus];
    } else {
        zone = g_GroundZone2[g_FlipStatus];
    }

    if (zone[box_number] != zone_number) {
        return false;
    }

    BOX_INFO *box = &g_Boxes[box_number];
    if (box->overlap_index & creature->LOT.block_mask) {
        return false;
    }

    if (item->pos.z > box->left && item->pos.z < box->right
        && item->pos.x > box->top && item->pos.x < box->bottom) {
        return false;
    }

    return true;
}

TARGET_TYPE Box_CalculateTarget(
    VECTOR_3D *target, ITEM_INFO *item, LOT_INFO *LOT)
{
    int32_t left = 0;
    int32_t right = 0;
    int32_t top = 0;
    int32_t bottom = 0;

    Box_UpdateLOT(LOT, MAX_EXPANSION);

    target->x = item->pos.x;
    target->y = item->pos.y;
    target->z = item->pos.z;

    int32_t box_number = item->box_number;
    if (box_number == NO_BOX) {
        return TARGET_NONE;
    }

    BOX_INFO *box;
    int32_t prime_free = ALL_CLIP;
    do {
        box = &g_Boxes[box_number];

        if (LOT->fly) {
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

        if (box_number == LOT->target_box) {
            if (prime_free & (CLIP_LEFT | CLIP_RIGHT)) {
                target->z = LOT->target.z;
            } else if (!(prime_free & SECONDARY_CLIP)) {
                if (target->z < box->left + BIFF) {
                    target->z = box->left + BIFF;
                } else if (target->z > box->right - BIFF) {
                    target->z = box->right - BIFF;
                }
            }

            if (prime_free & (CLIP_TOP | CLIP_BOTTOM)) {
                target->x = LOT->target.x;
            } else if (!(prime_free & SECONDARY_CLIP)) {
                if (target->x < box->top + BIFF) {
                    target->x = box->top + BIFF;
                } else if (target->x > box->bottom - BIFF) {
                    target->x = box->bottom - BIFF;
                }
            }

            target->y = LOT->target.y;
            return TARGET_PRIMARY;
        }

        box_number = LOT->node[box_number].exit_box;
        if (box_number != NO_BOX
            && (g_Boxes[box_number].overlap_index & LOT->block_mask)) {
            break;
        }
    } while (box_number != NO_BOX);

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

    if (!LOT->fly) {
        target->y = box->height;
    } else {
        target->y = box->height - STEP_L * 3 / 2;
    }

    return TARGET_NONE;
}

bool Box_BadFloor(
    int32_t x, int32_t y, int32_t z, int16_t box_height, int16_t next_height,
    int16_t room_number, LOT_INFO *LOT)
{
    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_number);
    if (floor->box == NO_BOX) {
        return true;
    }

    if (g_Boxes[floor->box].overlap_index & LOT->block_mask) {
        return true;
    }

    int32_t height = g_Boxes[floor->box].height;
    if (box_height - height > LOT->step || box_height - height < LOT->drop) {
        return true;
    }

    if (box_height - height < -LOT->step && height > next_height) {
        return true;
    }

    if (LOT->fly && y > height + LOT->fly) {
        return true;
    }

    return false;
}
