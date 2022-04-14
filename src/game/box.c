#include "game/box.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/sphere.h"
#include "global/const.h"
#include "global/vars.h"

#include <stddef.h>

#define MAX_CREATURE_DISTANCE (WALL_L * 30)

void InitialiseCreature(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    item->pos.y_rot += (PHD_ANGLE)((Random_GetControl() - PHD_90) >> 1);
    item->collidable = 1;
    item->data = NULL;
}

void CreatureAIInfo(ITEM_INFO *item, AI_INFO *info)
{
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return;
    }

    int16_t *zone;
    if (creature->LOT.fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else if (creature->LOT.step == STEP_L) {
        zone = g_GroundZone[g_FlipStatus];
    } else {
        zone = g_GroundZone2[g_FlipStatus];
    }

    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    int32_t x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    item->box_number = r->floor[x_floor + y_floor * r->x_size].box;
    info->zone_number = zone[item->box_number];

    r = &g_RoomInfo[g_LaraItem->room_number];
    x_floor = (g_LaraItem->pos.z - r->z) >> WALL_SHIFT;
    y_floor = (g_LaraItem->pos.x - r->x) >> WALL_SHIFT;
    g_LaraItem->box_number = r->floor[x_floor + y_floor * r->x_size].box;
    info->enemy_zone = zone[g_LaraItem->box_number];

    if (g_Boxes[g_LaraItem->box_number].overlap_index
        & creature->LOT.block_mask) {
        info->enemy_zone |= BLOCKED;
    } else if (
        creature->LOT.node[item->box_number].search_number
        == (creature->LOT.search_number | BLOCKED_SEARCH)) {
        info->enemy_zone |= BLOCKED;
    }

    OBJECT_INFO *object = &g_Objects[item->object_number];
    GetBestFrame(item);

    int32_t z = g_LaraItem->pos.z
        - ((phd_cos(item->pos.y_rot) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.z;
    int32_t x = g_LaraItem->pos.x
        - ((phd_sin(item->pos.y_rot) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.x;

    PHD_ANGLE angle = phd_atan(z, x);
    info->distance = SQUARE(x) + SQUARE(z);
    if (ABS(x) > MAX_CREATURE_DISTANCE || ABS(z) > MAX_CREATURE_DISTANCE) {
        info->distance = SQUARE(MAX_CREATURE_DISTANCE);
    }
    info->angle = angle - item->pos.y_rot;
    info->enemy_facing = angle - g_LaraItem->pos.y_rot + PHD_180;
    info->ahead = info->angle > -FRONT_ARC && info->angle < FRONT_ARC;
    info->bite = info->ahead && (g_LaraItem->pos.y > item->pos.y - STEP_L)
        && (g_LaraItem->pos.y < item->pos.y + STEP_L);
}

int32_t SearchLOT(LOT_INFO *LOT, int32_t expansion)
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
            return 0;
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

    return 1;
}

int32_t UpdateLOT(LOT_INFO *LOT, int32_t expansion)
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

    return SearchLOT(LOT, expansion);
}

void TargetBox(LOT_INFO *LOT, int16_t box_number)
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

int32_t StalkBox(ITEM_INFO *item, int16_t box_number)
{
    BOX_INFO *box = &g_Boxes[box_number];
    int32_t z = ((box->left + box->right) >> 1) - g_LaraItem->pos.z;
    int32_t x = ((box->top + box->bottom) >> 1) - g_LaraItem->pos.x;

    if (x > STALK_DIST || x < -STALK_DIST || z > STALK_DIST
        || z < -STALK_DIST) {
        return 0;
    }

    int enemy_quad = (g_LaraItem->pos.y_rot >> 14) + 2;
    int box_quad = (z > 0) ? ((x > 0) ? 2 : 1) : ((x > 0) ? 3 : 0);

    if (enemy_quad == box_quad) {
        return 0;
    }

    int baddie_quad = (item->pos.z > g_LaraItem->pos.z)
        ? ((item->pos.x > g_LaraItem->pos.x) ? 2 : 1)
        : ((item->pos.x > g_LaraItem->pos.x) ? 3 : 0);

    if (enemy_quad == baddie_quad && ABS(enemy_quad - box_quad) == 2) {
        return 0;
    }

    return 1;
}

int32_t EscapeBox(ITEM_INFO *item, int16_t box_number)
{
    BOX_INFO *box = &g_Boxes[box_number];
    int32_t z = ((box->left + box->right) >> 1) - g_LaraItem->pos.z;
    int32_t x = ((box->top + box->bottom) >> 1) - g_LaraItem->pos.x;

    if (x > -ESCAPE_DIST && x < ESCAPE_DIST && z > -ESCAPE_DIST
        && z < ESCAPE_DIST) {
        return 0;
    }

    if (((z > 0) ^ (item->pos.z > g_LaraItem->pos.z))
        && ((x > 0) ^ (item->pos.x > g_LaraItem->pos.x))) {
        return 0;
    }

    return 1;
}

int32_t ValidBox(ITEM_INFO *item, int16_t zone_number, int16_t box_number)
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
        return 0;
    }

    BOX_INFO *box = &g_Boxes[box_number];
    if (box->overlap_index & creature->LOT.block_mask) {
        return 0;
    }

    if (item->pos.z > box->left && item->pos.z < box->right
        && item->pos.x > box->top && item->pos.x < box->bottom) {
        return 0;
    }

    return 1;
}

void CreatureMood(ITEM_INFO *item, AI_INFO *info, int32_t violent)
{
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return;
    }

    LOT_INFO *LOT = &creature->LOT;
    if (LOT->node[item->box_number].search_number
        == (LOT->search_number | BLOCKED_SEARCH)) {
        LOT->required_box = NO_BOX;
    }

    if (creature->mood != MOOD_ATTACK && LOT->required_box != NO_BOX
        && !ValidBox(item, info->zone_number, LOT->target_box)) {
        if (info->zone_number == info->enemy_zone) {
            creature->mood = MOOD_BORED;
        }
        LOT->required_box = NO_BOX;
    }

    MOOD_TYPE mood = creature->mood;

    if (g_LaraItem->hit_points <= 0) {
        creature->mood = MOOD_BORED;
    } else if (violent) {
        switch (mood) {
        case MOOD_ATTACK:
            if (info->zone_number != info->enemy_zone) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_BORED:
        case MOOD_STALK:
            if (info->zone_number == info->enemy_zone) {
                creature->mood = MOOD_ATTACK;
            } else if (item->hit_status) {
                creature->mood = MOOD_ESCAPE;
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_number == info->enemy_zone) {
                creature->mood = MOOD_ATTACK;
            }
            break;
        }
    } else {
        switch (mood) {
        case MOOD_ATTACK:
            if (item->hit_status
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_number != info->enemy_zone)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_number != info->enemy_zone) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_BORED:
        case MOOD_STALK:
            if (item->hit_status
                && (Random_GetControl() < ESCAPE_CHANCE
                    || info->zone_number != info->enemy_zone)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_number == info->enemy_zone) {
                if (info->distance < ATTACK_RANGE
                    || (creature->mood == MOOD_STALK
                        && LOT->required_box == NO_BOX)) {
                    creature->mood = MOOD_ATTACK;
                } else {
                    creature->mood = MOOD_STALK;
                }
            }
            break;

        case MOOD_ESCAPE:
            if (info->zone_number == info->enemy_zone
                && Random_GetControl() < RECOVER_CHANCE) {
                creature->mood = MOOD_STALK;
            }
            break;
        }
    }

    if (mood != creature->mood) {
        if (mood == MOOD_ATTACK) {
            TargetBox(LOT, LOT->target_box);
        }
        LOT->required_box = NO_BOX;
    }

    switch (creature->mood) {
    case MOOD_ATTACK:
        if (Random_GetControl() < g_Objects[item->object_number].smartness) {
            LOT->target.x = g_LaraItem->pos.x;
            LOT->target.y = g_LaraItem->pos.y;
            LOT->target.z = g_LaraItem->pos.z;
            LOT->required_box = g_LaraItem->box_number;
            if (LOT->fly && g_Lara.water_status == LWS_ABOVE_WATER) {
                int16_t *bounds = GetBestFrame(g_LaraItem);
                LOT->target.y += bounds[FRAME_BOUND_MIN_Y];
            }
        }
        break;

    case MOOD_BORED: {
        int box_number =
            LOT->node[Random_GetControl() * LOT->zone_count / 0x7FFF]
                .box_number;
        if (ValidBox(item, info->zone_number, box_number)) {
            if (StalkBox(item, box_number)) {
                TargetBox(LOT, box_number);
                creature->mood = MOOD_STALK;
            } else if (LOT->required_box == NO_BOX) {
                TargetBox(LOT, box_number);
            }
        }
        break;
    }

    case MOOD_STALK: {
        if (LOT->required_box == NO_BOX || !StalkBox(item, LOT->required_box)) {
            int box_number =
                LOT->node[Random_GetControl() * LOT->zone_count / 0x7FFF]
                    .box_number;
            if (ValidBox(item, info->zone_number, box_number)) {
                if (StalkBox(item, box_number)) {
                    TargetBox(LOT, box_number);
                } else if (LOT->required_box == NO_BOX) {
                    TargetBox(LOT, box_number);
                    if (info->zone_number != info->enemy_zone) {
                        creature->mood = MOOD_BORED;
                    }
                }
            }
        }
        break;
    }

    case MOOD_ESCAPE: {
        int box_number =
            LOT->node[Random_GetControl() * LOT->zone_count / 0x7FFF]
                .box_number;
        if (ValidBox(item, info->zone_number, box_number)
            && LOT->required_box == NO_BOX) {
            if (EscapeBox(item, box_number)) {
                TargetBox(LOT, box_number);
            } else if (
                info->zone_number == info->enemy_zone
                && StalkBox(item, box_number)) {
                TargetBox(LOT, box_number);
                creature->mood = MOOD_STALK;
            }
        }
        break;
    }
    }

    if (LOT->target_box == NO_BOX) {
        TargetBox(LOT, item->box_number);
    }

    CalculateTarget(&creature->target, item, &creature->LOT);
}

int32_t CalculateTarget(PHD_VECTOR *target, ITEM_INFO *item, LOT_INFO *LOT)
{
    int32_t left = 0;
    int32_t right = 0;
    int32_t top = 0;
    int32_t bottom = 0;

    UpdateLOT(LOT, MAX_EXPANSION);

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

int32_t CreatureCreature(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int32_t radius = SQUARE(g_Objects[item->object_number].radius);

    int16_t link = g_RoomInfo[item->room_number].item_number;
    do {
        item = &g_Items[link];

        if (link == item_num) {
            return 0;
        }

        if (item != g_LaraItem && item->status == IS_ACTIVE
            && item->speed != 0) {
            int32_t distance = SQUARE(item->pos.x - x) + SQUARE(item->pos.y - y)
                + SQUARE(item->pos.z - z);
            if (distance < radius) {
                return 1;
            }
        }

        link = item->next_item;
    } while (link != NO_ITEM);

    return 0;
}

int32_t BadFloor(
    int32_t x, int32_t y, int32_t z, int16_t box_height, int16_t next_height,
    int16_t room_number, LOT_INFO *LOT)
{
    FLOOR_INFO *floor = GetFloor(x, y, z, &room_number);
    if (floor->box == NO_BOX) {
        return 1;
    }

    if (g_Boxes[floor->box].overlap_index & LOT->block_mask) {
        return 1;
    }

    int32_t height = g_Boxes[floor->box].height;
    if (box_height - height > LOT->step || box_height - height < LOT->drop) {
        return 1;
    }

    if (box_height - height < -LOT->step && height > next_height) {
        return 1;
    }

    if (LOT->fly && y > height + LOT->fly) {
        return 1;
    }

    return 0;
}

int32_t CreatureAnimation(int16_t item_num, int16_t angle, int16_t tilt)
{
    ITEM_INFO *item = &g_Items[item_num];
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return 0;
    }
    LOT_INFO *LOT = &creature->LOT;

    PHD_VECTOR old;
    old.x = item->pos.x;
    old.y = item->pos.y;
    old.z = item->pos.z;

    int32_t box_height = g_Boxes[item->box_number].height;

    int16_t *zone;
    if (LOT->fly) {
        zone = g_FlyZone[g_FlipStatus];
    } else if (LOT->step == STEP_L) {
        zone = g_GroundZone[g_FlipStatus];
    } else {
        zone = g_GroundZone2[g_FlipStatus];
    }

    AnimateItem(item);
    if (item->status == IS_DEACTIVATED) {
        item->collidable = 0;
        item->hit_points = DONT_TARGET;
        DisableBaddieAI(item_num);
        RemoveActiveItem(item_num);
        return 0;
    }

    int16_t *bounds = GetBoundsAccurate(item);
    int32_t y = item->pos.y + bounds[FRAME_BOUND_MIN_Y];

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = GetFloor(item->pos.x, y, item->pos.z, &room_num);
    int32_t height = g_Boxes[floor->box].height;
    int16_t next_box = LOT->node[floor->box].exit_box;
    int32_t next_height;
    if (next_box != NO_BOX) {
        next_height = g_Boxes[next_box].height;
    } else {
        next_height = height;
    }

    int32_t pos_x;
    int32_t pos_z;
    int32_t shift_x;
    int32_t shift_z;
    if (floor->box == NO_BOX || zone[item->box_number] != zone[floor->box]
        || box_height - height > LOT->step || box_height - height < LOT->drop) {
        pos_x = item->pos.x >> WALL_SHIFT;

        shift_x = old.x >> WALL_SHIFT;
        shift_z = old.z >> WALL_SHIFT;

        if (pos_x < shift_x) {
            item->pos.x = old.x & (~(WALL_L - 1));
        } else if (pos_x > shift_x) {
            item->pos.x = old.x | (WALL_L - 1);
        }

        if (pos_x < shift_z) {
            item->pos.z = old.z & (~(WALL_L - 1));
        } else if (pos_x > shift_z) {
            item->pos.z = old.z | (WALL_L - 1);
        }

        floor = GetFloor(item->pos.x, y, item->pos.z, &room_num);
        height = g_Boxes[floor->box].height;
        next_box = LOT->node[floor->box].exit_box;
        if (next_box != NO_BOX) {
            next_height = g_Boxes[next_box].height;
        } else {
            next_height = height;
        }
    }

    int32_t x = item->pos.x;
    int32_t z = item->pos.z;

    pos_x = x & (WALL_L - 1);
    pos_z = z & (WALL_L - 1);
    shift_x = 0;
    shift_z = 0;

    int32_t radius = g_Objects[item->object_number].radius;

    if (pos_z < radius) {
        if (BadFloor(x, y, z - radius, height, next_height, room_num, LOT)) {
            shift_z = radius - pos_z;
        }

        if (pos_x < radius) {
            if (BadFloor(
                    x - radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = radius - pos_x;
            } else if (
                !shift_z
                && BadFloor(
                    x - radius, y, z - radius, height, next_height, room_num,
                    LOT)) {
                if (item->pos.y_rot > -PHD_135 && item->pos.y_rot < PHD_45) {
                    shift_z = radius - pos_z;
                } else {
                    shift_x = radius - pos_x;
                }
            }
        } else if (pos_x > WALL_L - radius) {
            if (BadFloor(
                    x + radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = WALL_L - radius - pos_x;
            } else if (
                !shift_z
                && BadFloor(
                    x + radius, y, z - radius, height, next_height, room_num,
                    LOT)) {
                if (item->pos.y_rot > -PHD_45 && item->pos.y_rot < PHD_135) {
                    shift_z = radius - pos_z;
                } else {
                    shift_x = WALL_L - radius - pos_x;
                }
            }
        }
    } else if (pos_z > WALL_L - radius) {
        if (BadFloor(x, y, z + radius, height, next_height, room_num, LOT)) {
            shift_z = WALL_L - radius - pos_z;
        }

        if (pos_x < radius) {
            if (BadFloor(
                    x - radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = radius - pos_x;
            } else if (
                !shift_z
                && BadFloor(
                    x - radius, y, z + radius, height, next_height, room_num,
                    LOT)) {
                if (item->pos.y_rot > -PHD_45 && item->pos.y_rot < PHD_135) {
                    shift_x = radius - pos_x;
                } else {
                    shift_z = WALL_L - radius - pos_z;
                }
            }
        } else if (pos_x > WALL_L - radius) {
            if (BadFloor(
                    x + radius, y, z, height, next_height, room_num, LOT)) {
                shift_x = WALL_L - radius - pos_x;
            } else if (
                !shift_z
                && BadFloor(
                    x + radius, y, z + radius, height, next_height, room_num,
                    LOT)) {
                if (item->pos.y_rot > -PHD_135 && item->pos.y_rot < PHD_45) {
                    shift_x = WALL_L - radius - pos_x;
                } else {
                    shift_z = WALL_L - radius - pos_z;
                }
            }
        }
    } else if (pos_x < radius) {
        if (BadFloor(x - radius, y, z, height, next_height, room_num, LOT)) {
            shift_x = radius - pos_x;
        }
    } else if (pos_x > WALL_L - radius) {
        if (BadFloor(x + radius, y, z, height, next_height, room_num, LOT)) {
            shift_x = WALL_L - radius - pos_x;
        }
    }

    item->pos.x += shift_x;
    item->pos.z += shift_z;

    if (shift_x || shift_z) {
        floor = GetFloor(item->pos.x, y, item->pos.z, &room_num);

        item->pos.y_rot += angle;
        CreatureTilt(item, tilt * 2);
    }

    if (CreatureCreature(item_num)) {
        item->pos.x = old.x;
        item->pos.y = old.y;
        item->pos.z = old.z;
        return 1;
    }

    if (LOT->fly) {
        int32_t dy = creature->target.y - item->pos.y;
        if (dy > LOT->fly) {
            dy = LOT->fly;
        } else if (dy < -LOT->fly) {
            dy = -LOT->fly;
        }

        height = GetHeight(floor, item->pos.x, y, item->pos.z);
        if (item->pos.y + dy > height) {
            if (item->pos.y > height) {
                item->pos.x = old.x;
                item->pos.z = old.z;
                dy = -LOT->fly;
            } else {
                dy = 0;
                item->pos.y = height;
            }
        } else {
            int32_t ceiling = GetCeiling(floor, item->pos.x, y, item->pos.z);

            if (item->object_number == O_ALLIGATOR) {
                bounds[FRAME_BOUND_MIN_Y] = 0;
            }

            if (item->pos.y + bounds[FRAME_BOUND_MIN_Y] + dy < ceiling) {
                if (item->pos.y + bounds[FRAME_BOUND_MIN_Y] < ceiling) {
                    item->pos.x = old.x;
                    item->pos.z = old.z;
                    dy = LOT->fly;
                } else {
                    dy = 0;
                }
            }
        }

        item->pos.y += dy;
        floor = GetFloor(item->pos.x, y, item->pos.z, &room_num);
        item->floor = GetHeight(floor, item->pos.x, y, item->pos.z);

        angle = item->speed ? phd_atan(item->speed, -dy) : 0;
        if (angle < item->pos.x_rot - PHD_DEGREE) {
            item->pos.x_rot -= PHD_DEGREE;
        } else if (angle > item->pos.x_rot + PHD_DEGREE) {
            item->pos.x_rot += PHD_DEGREE;
        } else {
            item->pos.x_rot = angle;
        }
    } else {
        if (item->pos.y > item->floor) {
            item->pos.y = item->floor;
        } else if (item->floor - item->pos.y > STEP_L / 4) {
            item->pos.y += STEP_L / 4;
        } else if (item->pos.y < item->floor) {
            item->pos.y = item->floor;
        }

        item->pos.x_rot = 0;

        floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
    }

    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    return 1;
}

int16_t CreatureTurn(ITEM_INFO *item, int16_t maximum_turn)
{
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return 0;
    }

    if (!item->speed || !maximum_turn) {
        return 0;
    }

    int32_t x = creature->target.x - item->pos.x;
    int32_t z = creature->target.z - item->pos.z;
    int16_t angle = phd_atan(z, x) - item->pos.y_rot;
    int32_t range = (item->speed << 14) / maximum_turn;

    if (angle > FRONT_ARC || angle < -FRONT_ARC) {
        if (SQUARE(x) + SQUARE(z) < SQUARE(range)) {
            maximum_turn >>= 1;
        }
    }

    if (angle > maximum_turn) {
        angle = maximum_turn;
    } else if (angle < -maximum_turn) {
        angle = -maximum_turn;
    }

    item->pos.y_rot += angle;

    return angle;
}

void CreatureTilt(ITEM_INFO *item, int16_t angle)
{
    angle = angle * 4 - item->pos.z_rot;
    if (angle < -MAX_TILT) {
        angle = -MAX_TILT;
    } else if (angle > MAX_TILT) {
        angle = MAX_TILT;
    }
    item->pos.z_rot += angle;
}

void CreatureHead(ITEM_INFO *item, int16_t required)
{
    CREATURE_INFO *creature = item->data;
    if (!creature) {
        return;
    }

    int16_t change = required - creature->head_rotation;
    if (change > MAX_HEAD_CHANGE) {
        change = MAX_HEAD_CHANGE;
    } else if (change < -MAX_HEAD_CHANGE) {
        change = -MAX_HEAD_CHANGE;
    }

    creature->head_rotation += change;

    if (creature->head_rotation > FRONT_ARC) {
        creature->head_rotation = FRONT_ARC;
    } else if (creature->head_rotation < -FRONT_ARC) {
        creature->head_rotation = -FRONT_ARC;
    }
}

int16_t CreatureEffect(
    ITEM_INFO *item, BITE_INFO *bite,
    int16_t (*spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t yrot,
        int16_t room_num))
{
    PHD_VECTOR pos;
    pos.x = bite->x;
    pos.y = bite->y;
    pos.z = bite->z;
    GetJointAbsPosition(item, &pos, bite->mesh_num);
    return spawn(
        pos.x, pos.y, pos.z, item->speed, item->pos.y_rot, item->room_number);
}
