#include "game/creature.h"

#include "game/box.h"
#include "game/draw.h"
#include "game/random.h"
#include "global/vars.h"
#include "math/math.h"

#define MAX_CREATURE_DISTANCE (WALL_L * 30)

void Creature_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    item->pos.y_rot += (PHD_ANGLE)((Random_GetControl() - PHD_90) >> 1);
    item->collidable = 1;
    item->data = NULL;
}

void Creature_AIInfo(ITEM_INFO *item, AI_INFO *info)
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
        - ((Math_Cos(item->pos.y_rot) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.z;
    int32_t x = g_LaraItem->pos.x
        - ((Math_Sin(item->pos.y_rot) * object->pivot_length) >> W2V_SHIFT)
        - item->pos.x;

    PHD_ANGLE angle = Math_Atan(z, x);
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

void Creature_Mood(ITEM_INFO *item, AI_INFO *info, bool violent)
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
