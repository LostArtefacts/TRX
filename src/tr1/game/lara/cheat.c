#include "game/console/common.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>
#include <libtrx/vector.h>

bool Lara_Cheat_Teleport(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    if (room_num == NO_ROOM) {
        room_num = Room_GetIndexFromPos(x, y, z);
    }
    if (room_num == NO_ROOM) {
        return false;
    }

    const ROOM *const room = Room_Get(room_num);
    if (room->flip_status == RFS_FLIPPED && Room_GetFlipStatus()) {
        room_num = Room_GetFlippedBaseRoom(room_num);
        if (room_num == NO_ROOM) {
            return false;
        }
    }

    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    int16_t height = Room_GetHeight(sector, x, y, z);

    if (height == NO_HEIGHT) {
        // Sample a sphere of points around target x, y, z
        // and teleport to the first available location.
        VECTOR *const points = Vector_Create(sizeof(XYZ_32));

        const int32_t radius = 10;
        const int32_t unit = STEP_L;
        for (int32_t dx = -radius; dx <= radius; dx++) {
            for (int32_t dz = -radius; dz <= radius; dz++) {
                if (SQUARE(dx) + SQUARE(dz) > SQUARE(radius)) {
                    continue;
                }

                const XYZ_32 point = {
                    .x = ROUND_TO_SECTOR(x + dx * unit) + WALL_L / 2,
                    .y = y,
                    .z = ROUND_TO_SECTOR(z + dz * unit) + WALL_L / 2,
                };
                sector = Room_GetSector(point.x, point.y, point.z, &room_num);
                height =
                    Room_GetHeightEx(sector, point.x, point.y, point.z, true);
                if (height == NO_HEIGHT) {
                    continue;
                }
                Vector_Add(points, (void *)&point);
            }
        }

        int32_t best_distance = INT32_MAX;
        for (int32_t i = 0; i < points->count; i++) {
            const XYZ_32 *const point = (const XYZ_32 *)Vector_Get(points, i);
            const int32_t distance =
                XYZ_32_GetDistance(point, &g_LaraItem->pos);
            if (distance < best_distance) {
                best_distance = distance;
                x = point->x;
                y = point->y;
                z = point->z;
            }
        }

        Vector_Free(points);
        if (best_distance == INT32_MAX) {
            return false;
        }
    }

    sector = Room_GetSector(x, y, z, &room_num);
    height = Room_GetHeightEx(sector, x, y, z, true);
    if (height == NO_HEIGHT) {
        return false;
    }

    g_LaraItem->pos.x = x;
    g_LaraItem->pos.y = y;
    g_LaraItem->pos.z = z;
    g_LaraItem->floor = height;

    const int16_t item_num = Item_GetIndex(g_LaraItem);
    Item_UpdateRoom(item_num, room_num);

    Camera_ResetPosition();
    return true;
}
