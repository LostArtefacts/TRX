#include "3dsystem/phd_math.h"
#include "game/draw.h"
#include "game/box.h"
#include "game/data.h"
#include "game/game.h"
#include "util.h"

void __cdecl InitialiseCreature(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    item->pos.y_rot += (PHD_ANGLE)((GetRandomControl() - 0x4000) >> 1);
    item->collidable = 1;
    item->data = NULL;
}

void __cdecl CreatureAIInfo(ITEM_INFO* item, AI_INFO* info)
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

void Tomb1MInjectGameBox()
{
    INJECT(0x0040DA60, InitialiseCreature);
    INJECT(0x0040DAA0, CreatureAIInfo);
}
