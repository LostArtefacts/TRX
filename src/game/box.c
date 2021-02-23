#include "3dsystem/phd_math.h"
#include "game/box.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/misc.h"
#include "game/vars.h"
#include "util.h"

void InitialiseCreature(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    item->pos.y_rot += (PHD_ANGLE)((GetRandomControl() - 0x4000) >> 1);
    item->collidable = 1;
    item->data = NULL;
}

void CreatureAIInfo(ITEM_INFO* item, AI_INFO* info)
{
    CREATURE_INFO* creature = item->data;
    if (!creature) {
        return;
    }

    int16_t* zone;
    if (creature->LOT.fly) {
        zone = FlyZone[FlipStatus];
    } else if (creature->LOT.step == STEP_L) {
        zone = GroundZone[FlipStatus];
    } else {
        zone = GroundZone2[FlipStatus];
    }

    ROOM_INFO* r = &RoomInfo[item->room_number];
    item->box_number = r->floor
                           [((item->pos.z - r->z) >> WALL_SHIFT)
                            + ((item->pos.x - r->x) >> WALL_SHIFT) * r->x_size]
                               .box;
    info->zone_number = zone[item->box_number];

    LaraItem->box_number =
        r->floor
            [((LaraItem->pos.z - r->z) >> WALL_SHIFT)
             + r->x_size * ((LaraItem->pos.x - r->x) >> WALL_SHIFT)]
                .box;
    info->enemy_zone = zone[LaraItem->box_number];

    if (Boxes[LaraItem->box_number].overlap_index & creature->LOT.block_mask) {
        info->enemy_zone |= BLOCKED;
    } else if (
        creature->LOT.node[item->box_number].search_number
        == (creature->LOT.search_number | BLOCKED_SEARCH)) {
        info->enemy_zone |= BLOCKED;
    }

    OBJECT_INFO* object = &Objects[item->object_number];
    GetBestFrame(item);

    int32_t z = LaraItem->pos.z
        - ((phd_cos(item->pos.y_rot) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.z;
    int32_t x = LaraItem->pos.x
        - ((phd_sin(item->pos.y_rot) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.x;

    PHD_ANGLE angle = phd_atan(z, x);
    info->distance = x * x + z * z;
    info->angle = angle - item->pos.y_rot;
    info->enemy_facing = angle - LaraItem->pos.y_rot - PHD_ONE / 2;
    info->ahead = info->angle > -FRONT_ARC && info->angle < FRONT_ARC;
    info->bite = info->ahead && (LaraItem->pos.y > item->pos.y - STEP_L)
        && (LaraItem->pos.y < item->pos.y + STEP_L);
}

int32_t SearchLOT(LOT_INFO* LOT, int32_t expansion)
{
    int16_t* zone;
    if (LOT->fly) {
        zone = FlyZone[FlipStatus];
    } else if (LOT->step == STEP_L) {
        zone = GroundZone[FlipStatus];
    } else {
        zone = GroundZone2[FlipStatus];
    }

    int16_t search_zone = zone[LOT->head];
    for (int i = 0; i < expansion; i++) {
        if (LOT->head == NO_BOX) {
            return 0;
        }

        BOX_NODE* node = &LOT->node[LOT->head];
        BOX_INFO* box = &Boxes[LOT->head];

        int done = 0;
        int index = box->overlap_index & OVERLAP_INDEX;
        do {
            int16_t box_number = Overlap[index++];
            if (box_number & END_BIT) {
                done = 1;
                box_number &= BOX_NUMBER;
            }

            if (search_zone != zone[box_number]) {
                continue;
            }

            int change = Boxes[box_number].height - box->height;
            if (change > LOT->step || change < LOT->drop) {
                continue;
            }

            BOX_NODE* expand = &LOT->node[box_number];
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

                if (Boxes[box_number].overlap_index & LOT->block_mask) {
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

int32_t UpdateLOT(LOT_INFO* LOT, int32_t expansion)
{
    if (LOT->required_box != NO_BOX && LOT->required_box != LOT->target_box) {
        LOT->target_box = LOT->required_box;

        BOX_NODE* expand = &LOT->node[LOT->target_box];
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

void TargetBox(LOT_INFO* LOT, int16_t box_number)
{
    box_number &= BOX_NUMBER;

    BOX_INFO* box = &Boxes[box_number];

    LOT->target.z = box->left + WALL_L / 2
        + (GetRandomControl() * (box->right - box->left - WALL_L) >> 15);
    LOT->target.x = box->top + WALL_L / 2
        + (GetRandomControl() * (box->bottom - box->top - WALL_L) >> 15);
    LOT->required_box = box_number;

    if (LOT->fly) {
        LOT->target.y = box->height - STEP_L * 3 / 2;
    } else {
        LOT->target.y = box->height;
    }
}

int32_t StalkBox(ITEM_INFO* item, int16_t box_number)
{
    BOX_INFO* box = &Boxes[box_number];
    int32_t z = ((box->left + box->right) >> 1) - LaraItem->pos.z;
    int32_t x = ((box->top + box->bottom) >> 1) - LaraItem->pos.x;

    if (x > STALK_DIST || x < -STALK_DIST || z > STALK_DIST
        || z < -STALK_DIST) {
        return 0;
    }

    int enemy_quad = (LaraItem->pos.y_rot >> 14) + 2;
    int box_quad = (z > 0) ? ((x > 0) ? 2 : 1) : ((x > 0) ? 3 : 0);

    if (enemy_quad == box_quad) {
        return 0;
    }

    int baddie_quad = (item->pos.z > LaraItem->pos.z)
        ? ((item->pos.x > LaraItem->pos.x) ? 2 : 1)
        : ((item->pos.x > LaraItem->pos.x) ? 3 : 0);

    if (enemy_quad == baddie_quad && ABS(enemy_quad - box_quad) == 2) {
        return 0;
    }

    return 1;
}

int32_t EscapeBox(ITEM_INFO* item, int16_t box_number)
{
    BOX_INFO* box = &Boxes[box_number];
    int32_t z = ((box->left + box->right) >> 1) - LaraItem->pos.z;
    int32_t x = ((box->top + box->bottom) >> 1) - LaraItem->pos.x;

    if (x > -ESCAPE_DIST && x < ESCAPE_DIST && z > -ESCAPE_DIST
        && z < ESCAPE_DIST) {
        return 0;
    }

    if (((z > 0) ^ (item->pos.z > LaraItem->pos.z))
        && ((x > 0) ^ (item->pos.x > LaraItem->pos.x))) {
        return 0;
    }

    return 1;
}

int32_t ValidBox(ITEM_INFO* item, int16_t zone_number, int16_t box_number)
{
    CREATURE_INFO* creature = item->data;

    int16_t* zone;
    if (creature->LOT.fly) {
        zone = FlyZone[FlipStatus];
    } else if (creature->LOT.step == STEP_L) {
        zone = GroundZone[FlipStatus];
    } else {
        zone = GroundZone2[FlipStatus];
    }

    if (zone[box_number] != zone_number) {
        return 0;
    }

    BOX_INFO* box = &Boxes[box_number];
    if (box->overlap_index & creature->LOT.block_mask) {
        return 0;
    }

    if (item->pos.z > box->left && item->pos.z < box->right
        && item->pos.x > box->top && item->pos.x < box->bottom) {
        return 0;
    }

    return 1;
}

void CreatureMood(ITEM_INFO* item, AI_INFO* info, int32_t violent)
{
    CREATURE_INFO* creature = item->data;
    if (!creature) {
        return;
    }

    LOT_INFO* LOT = &creature->LOT;
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

    if (LaraItem->hit_points <= 0) {
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
                && (GetRandomControl() < ESCAPE_CHANCE
                    || info->zone_number != info->enemy_zone)) {
                creature->mood = MOOD_ESCAPE;
            } else if (info->zone_number != info->enemy_zone) {
                creature->mood = MOOD_BORED;
            }
            break;

        case MOOD_BORED:
        case MOOD_STALK:
            if (item->hit_status
                && (GetRandomControl() < ESCAPE_CHANCE
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
                && GetRandomControl() < RECOVER_CHANCE) {
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
        if (GetRandomControl() < Objects[item->object_number].smartness) {
            LOT->target.x = LaraItem->pos.x;
            LOT->target.y = LaraItem->pos.y;
            LOT->target.z = LaraItem->pos.z;
            LOT->required_box = LaraItem->box_number;
            if (LOT->fly && Lara.water_status == LWS_ABOVEWATER) {
                int16_t* bounds = GetBestFrame(LaraItem);
                LOT->target.y += bounds[FRAME_BOUND_MIN_Y];
            }
        }
        break;

    case MOOD_BORED: {
        int box_number =
            LOT->node[GetRandomControl() * LOT->zone_count / 0x7FFF].box_number;
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
                LOT->node[GetRandomControl() * LOT->zone_count / 0x7FFF]
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
            LOT->node[GetRandomControl() * LOT->zone_count / 0x7FFF].box_number;
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

int32_t CalculateTarget(PHD_VECTOR* target, ITEM_INFO* item, LOT_INFO* LOT)
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

    BOX_INFO* box;
    int32_t prime_free = ALL_CLIP;
    do {
        box = &Boxes[box_number];

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
            && (Boxes[box_number].overlap_index & LOT->block_mask)) {
            break;
        }
    } while (box_number != NO_BOX);

    if (prime_free & (CLIP_LEFT | CLIP_RIGHT)) {
        target->z = box->left + WALL_L / 2
            + (GetRandomControl() * (box->right - box->left - WALL_L) >> 15);
    } else if (!(prime_free & SECONDARY_CLIP)) {
        if (target->z < box->left + BIFF) {
            target->z = box->left + BIFF;
        } else if (target->z > box->right - BIFF) {
            target->z = box->right - BIFF;
        }
    }

    if (prime_free & (CLIP_TOP | CLIP_BOTTOM)) {
        target->x = box->top + WALL_L / 2
            + (GetRandomControl() * (box->bottom - box->top - WALL_L) >> 15);
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
    ITEM_INFO* item = &Items[item_num];

    int32_t x = item->pos.x;
    int32_t y = item->pos.y;
    int32_t z = item->pos.z;
    int32_t radius = SQUARE(Objects[item->object_number].radius);

    int16_t link = RoomInfo[item->room_number].item_number;
    do {
        item = &Items[link];

        if (link == item_num) {
            return 0;
        }

        if (item != LaraItem && item->status == IS_ACTIVE && item->speed != 0) {
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

void T1MInjectGameBox()
{
    INJECT(0x0040DA60, InitialiseCreature);
    INJECT(0x0040DAA0, CreatureAIInfo);
    INJECT(0x0040DCD0, SearchLOT);
    INJECT(0x0040DED0, StalkBox);
    INJECT(0x0040DFA0, ValidBox);
    INJECT(0x0040E040, CreatureMood);
    INJECT(0x0040E850, CalculateTarget);
    INJECT(0x0040ED30, CreatureCreature);
}
