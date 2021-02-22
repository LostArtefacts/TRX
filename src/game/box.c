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

void T1MInjectGameBox()
{
    INJECT(0x0040DA60, InitialiseCreature);
    INJECT(0x0040DAA0, CreatureAIInfo);
    INJECT(0x0040DCD0, SearchLOT);
    INJECT(0x0040DED0, StalkBox);
}
