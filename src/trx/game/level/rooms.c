#include <trx/game/items.h>
#include <trx/game/level/loader.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/log.h>
#include <trx/utils.h>

static bool M_CheckOverlap(
    const BOUNDS_32 *const room_bounds, const BOUNDS_32 *const other_bounds)
{
    int32_t margin = 0;
    margin = MAX(margin, MAX(0, room_bounds->min.x - other_bounds->min.x));
    margin = MAX(margin, MAX(0, other_bounds->max.x - room_bounds->max.x));
    margin = MAX(margin, MAX(0, room_bounds->min.y - other_bounds->min.y));
    margin = MAX(margin, MAX(0, other_bounds->max.y - room_bounds->max.y));
    margin = MAX(margin, MAX(0, room_bounds->min.z - other_bounds->min.z));
    margin = MAX(margin, MAX(0, other_bounds->max.z - room_bounds->max.z));
    return margin > 32;
}

static void M_CheckProtrudingItems(void)
{
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (item->room_num == NO_ROOM) {
            continue;
        }
        if (Object_Get(item->object_id)->intelligent) {
            continue;
        }

        ROOM *const room = Room_Get(item->room_num);
        if (room->flags.protruding_items) {
            continue;
        }
        const BOUNDS_16 *const item_bounds = Item_GetBoundsAccurate(item);
        const BOUNDS_32 other_bounds = {
            .min = {
                .x = item->pos.x + item_bounds->min.x,
                .y = item->pos.y + item_bounds->min.y,
                .z = item->pos.z + item_bounds->min.z,
            },
            .max = {
                .x = item->pos.x + item_bounds->max.x,
                .y = item->pos.y + item_bounds->max.y,
                .z = item->pos.z + item_bounds->max.z,
            },
        };
        const BOUNDS_32 room_bounds = Room_GetRoomBounds(item->room_num);
        if (M_CheckOverlap(&room_bounds, &other_bounds)) {
            room->flags.protruding_items = true;
            LOG_DEBUG(
                "Room %d has a protruding item: %d", item->room_num, item_num);
        }
    }
}

static void M_CheckProtrudingStatics(void)
{
    for (int32_t room_num = 0; room_num < Room_GetCount(); room_num++) {
        ROOM *const room = Room_Get(room_num);
        if (room->flags.protruding_items) {
            continue;
        }
        const BOUNDS_32 room_bounds = Room_GetRoomBounds(room_num);
        for (int32_t i = 0; i < room->num_static_meshes; i++) {
            const STATIC_MESH *const mesh = &room->static_meshes[i];
            const STATIC_OBJECT_3D *const obj =
                Object_Get3DStatic(mesh->static_num);
            const BOUNDS_32 other_bounds = {
                .min = {
                    .x = mesh->pos.x + obj->draw_bounds.min.x,
                    .y = mesh->pos.y + obj->draw_bounds.min.y,
                    .z = mesh->pos.z + obj->draw_bounds.min.z,
                },
                .max = {
                    .x = mesh->pos.x + obj->draw_bounds.max.x,
                    .y = mesh->pos.y + obj->draw_bounds.max.y,
                    .z = mesh->pos.z + obj->draw_bounds.max.z,
                },
            };
            if (M_CheckOverlap(&room_bounds, &other_bounds)) {
                room->flags.protruding_items = true;
                LOG_DEBUG("Room %d has a protruding static: %d", room_num, i);
                break;
            }
        }
    }
}

void Level_LoadRooms(void)
{
    // Check all items whether they fit properly within their rooms. If not,
    // mark the room as having protruding items. Do the same with statics.
    //
    // This flag is used by the room traverser to still let a room to be drawn,
    // even if all portals leading to it are currently offscreen.
    // This often happens with trapdoors – they're placed directly on portals.

    M_CheckProtrudingItems();
    M_CheckProtrudingStatics();
}
