#include <trx/core/log.h>
#include <trx/core/math/const.h>
#include <trx/core/math/trig.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_buf.h>
#include <trx/game/items.h>
#include <trx/game/level/finalize.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>

#include <string.h>

static inline BOUNDS_32 M_GetStaticBounds(const STATIC_MESH *const mesh)
{
    const STATIC_OBJECT_3D *const obj = Object_Get3DStatic(mesh->static_num);

    // The draw bounds are the object's own, and the mesh is drawn turned by
    // its Y rotation (see M_DrawSingleRoom). A box taken without that turn
    // sits askew of the mesh it stands for, and one at an angle the axes do
    // not share misses the portal it leans through.
    const int32_t xs[2] = { obj->draw_bounds.min.x, obj->draw_bounds.max.x };
    const int32_t zs[2] = { obj->draw_bounds.min.z, obj->draw_bounds.max.z };

    BOUNDS_32 bounds = {
        .min = {
            .x = INT32_MAX,
            .y = mesh->pos.y + obj->draw_bounds.min.y,
            .z = INT32_MAX,
        },
        .max = {
            .x = INT32_MIN,
            .y = mesh->pos.y + obj->draw_bounds.max.y,
            .z = INT32_MIN,
        },
    };
    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < 2; j++) {
            const XYZ_32 corner = XYZ_32_OffsetLocalYaw(
                mesh->pos, (XYZ_32) { .x = xs[i], .z = zs[j] }, mesh->rot.y);
            bounds.min.x = MIN(bounds.min.x, corner.x);
            bounds.max.x = MAX(bounds.max.x, corner.x);
            bounds.min.z = MIN(bounds.min.z, corner.z);
            bounds.max.z = MAX(bounds.max.z, corner.z);
        }
    }

    return bounds;
}

static void M_ComputePortalBounds(void)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        ROOM *const room = Room_Get(i);
        PORTALS *const portals = room->portals;
        if (portals == nullptr) {
            continue;
        }
        for (uint16_t p = 0; p < portals->count; p++) {
            PORTAL *const portal = &portals->portal[p];
            BOUNDS_32 *const bounds = &portal->bounds;
            bounds->min.x = room->pos.x + portal->vertex[0].x;
            bounds->min.y = room->pos.y + portal->vertex[0].y;
            bounds->min.z = room->pos.z + portal->vertex[0].z;
            bounds->max.x = room->pos.x + portal->vertex[0].x;
            bounds->max.y = room->pos.y + portal->vertex[0].y;
            bounds->max.z = room->pos.z + portal->vertex[0].z;
            for (int32_t k = 1; k < 4; k++) {
                bounds->min.x =
                    MIN(bounds->min.x, room->pos.x + portal->vertex[k].x);
                bounds->min.y =
                    MIN(bounds->min.y, room->pos.y + portal->vertex[k].y);
                bounds->min.z =
                    MIN(bounds->min.z, room->pos.z + portal->vertex[k].z);
                bounds->max.x =
                    MAX(bounds->max.x, room->pos.x + portal->vertex[k].x);
                bounds->max.y =
                    MAX(bounds->max.y, room->pos.y + portal->vertex[k].y);
                bounds->max.z =
                    MAX(bounds->max.z, room->pos.z + portal->vertex[k].z);
            }
        }
    }
}

static void M_FixStaticsVisibility(void)
{
    int32_t total_rooms = Room_GetCount();
    int32_t draw_num = 0;
    VECTOR **room_stat_vecs =
        Memory_Alloc(sizeof(*room_stat_vecs) * total_rooms);

    for (int32_t i = 0; i < total_rooms; i++) {
        room_stat_vecs[i] = Vector_Create(sizeof(STATIC_MESH));
        ROOM *const room = Room_Get(i);
        for (int32_t m = 0; m < room->num_static_meshes; m++) {
            STATIC_MESH *const static_mesh = &room->static_meshes[m];
            if (Object_IsValidStatid3D(static_mesh->static_num)) {
                ASSERT(draw_num < MAX_ITEMS);
                static_mesh->draw_num = draw_num++;
                static_mesh->room_num = i;
                Vector_Add(room_stat_vecs[i], static_mesh);
            } else {
                LOG_WARNING(
                    "Invalid static 3D (id %d) in room %d",
                    static_mesh->static_num, i);
            }
        }
    }

    // A room lends out the statics it holds, not the ones it was lent. Reading
    // the vector as it grows would pass a mesh on from room to room, into ones
    // it never reaches.
    int32_t *own_counts = Memory_Alloc(sizeof(int32_t) * total_rooms);
    for (int32_t i = 0; i < total_rooms; i++) {
        own_counts[i] = room_stat_vecs[i]->count;
    }

    for (int32_t i = 0; i < total_rooms; i++) {
        ROOM *const room = Room_Get(i);
        PORTALS *const portals = room->portals;
        if (portals == nullptr) {
            continue;
        }
        for (uint16_t p = 0; p < portals->count; p++) {
            const PORTAL *const portal = &portals->portal[p];
            ROOM *const dest_room = Room_Get(portal->room_num);
            if (room->flip_status != dest_room->flip_status) {
                continue;
            }
            for (int32_t m = 0; m < own_counts[i]; m++) {
                const STATIC_MESH *const mesh =
                    Vector_Get(room_stat_vecs[i], m);
                const BOUNDS_32 bounds = M_GetStaticBounds(mesh);
                if (!Room_BoundsReachPortal(&bounds, portal)) {
                    continue;
                }
                if (Vector_Contains(room_stat_vecs[portal->room_num], mesh)) {
                    continue;
                }
                Vector_Add(room_stat_vecs[portal->room_num], mesh);
                LOG_WARNING(
                    "Static #%d bleeds into room #%d", mesh->static_num,
                    portal->room_num);
            }
        }
    }

    Memory_FreePointer(&own_counts);

    int32_t total_needed = 0;
    for (int32_t i = 0; i < total_rooms; i++) {
        total_needed += room_stat_vecs[i]->count;
    }
    if (total_needed == 0) {
        for (int32_t i = 0; i < total_rooms; i++) {
            Vector_Free(room_stat_vecs[i]);
        }
        Memory_FreePointer(&room_stat_vecs);
    } else {
        STATIC_MESH *all_statics = GameBuf_Alloc(
            sizeof(STATIC_MESH) * total_needed, GBUF_ROOM_STATIC_MESHES);
        int32_t offset = 0;
        for (int32_t i = 0; i < total_rooms; i++) {
            ROOM *const room = Room_Get(i);
            room->static_meshes = &all_statics[offset];
            room->num_static_meshes = 0;
            VECTOR *vec = room_stat_vecs[i];
            for (int32_t m = 0; m < vec->count; m++) {
                room->static_meshes[room->num_static_meshes++] =
                    *(STATIC_MESH *)Vector_Get(vec, m);
            }
            offset += vec->count;
            Vector_Free(vec);
        }
        Memory_FreePointer(&room_stat_vecs);
    }
}

static void M_FixStaticsCollision(void)
{
    const int32_t count = Object_GetStaticObjects3DCount();
    for (int32_t i = 0; i < count; i++) {
        STATIC_OBJECT_3D *const obj = Object_Get3DStatic(i);
        if (!obj->loaded || !obj->collidable) {
            continue;
        }

        const XYZ_32 hitbox = {
            .x = obj->collision_bounds.max.x - obj->collision_bounds.min.x,
            .y = obj->collision_bounds.max.y - obj->collision_bounds.min.y,
            .z = obj->collision_bounds.max.z - obj->collision_bounds.min.z,
        };

        if (hitbox.x <= 0 && hitbox.y <= 0 && hitbox.z <= 0) {
            LOG_WARNING(
                "Static %d is marked as collidable, but has degenerate "
                "hitbox (%d x %d x %d)",
                i, hitbox.x, hitbox.y, hitbox.z);
            obj->collidable = false;
        }
    }
}

void Level_Finalize_LoadRooms(LEVEL_CONTEXT *const ctx)
{
    M_ComputePortalBounds();
    M_FixStaticsCollision();
    M_FixStaticsVisibility();
    Room_InitialiseOverlapMap();
    Item_InitialiseDrawQueues();
}
