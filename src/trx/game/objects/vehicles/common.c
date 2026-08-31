#include <trx/game/objects/vehicles/common.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/collision.h>
#include <trx/game/cutscene.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/table.h>
#include <trx/game/items/enum.h>
#include <trx/game/lara.h>
#include <trx/game/level.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/viewport.h>

#include <stdio.h>

typedef enum {
    LA_VEHICLE_HIT_LEFT = 11,
    LA_VEHICLE_HIT_RIGHT = 12,
    LA_VEHICLE_HIT_FRONT = 13,
    LA_VEHICLE_HIT_BACK = 14,
} LARA_ANIM_VEHICLE;

static int32_t M_LoadTrackPool(
    const ITEM *const item, const char *const key_prefix,
    MUSIC_SLOT *const out_tracks, const int32_t max_tracks)
{
    if (item == nullptr || key_prefix == nullptr || out_tracks == nullptr
        || max_tracks <= 0) {
        return 0;
    }

    int32_t track_count = 0;
    for (int32_t i = 0; i < VEHICLE_TRACK_POOL_SIZE && track_count < max_tracks;
         i++) {
        char key[32];
        snprintf(key, sizeof(key), "%s_%d", key_prefix, i + 1);

        TRX_VALUE value = {};
        if (ObjectProperty_GetItemValue(item, key, &value)
            && value.as_int >= 0) {
            out_tracks[track_count++] = value.as_int;
        }
    }

    return track_count;
}

int32_t Vehicle_DoShift(
    ITEM *const vehicle, const XYZ_32 *const pos, const XYZ_32 *const old)
{
    int32_t x = pos->x >> WALL_SHIFT;
    int32_t z = pos->z >> WALL_SHIFT;
    const int32_t old_x = old->x >> WALL_SHIFT;
    const int32_t old_z = old->z >> WALL_SHIFT;
    const int32_t shift_x = pos->x & (WALL_L - 1);
    const int32_t shift_z = pos->z & (WALL_L - 1);

    if (x == old_x) {
        if (z == old_z) {
            vehicle->pos.x += old->x - pos->x;
            vehicle->pos.z += old->z - pos->z;
        } else if (z > old_z) {
            vehicle->pos.z -= shift_z + 1;
            return pos->x - vehicle->pos.x;
        } else {
            vehicle->pos.z += WALL_L - shift_z;
            return vehicle->pos.x - pos->x;
        }
    } else if (z == old_z) {
        if (x > old_x) {
            vehicle->pos.x -= shift_x + 1;
            return vehicle->pos.z - pos->z;
        } else {
            vehicle->pos.x += WALL_L - shift_x;
            return pos->z - vehicle->pos.z;
        }
    } else {
        int16_t room_num;
        const SECTOR *sector;
        int32_t height;

        x = 0;
        z = 0;

        XYZ_32 test_pos = (XYZ_32) { old->x, pos->y, pos->z };
        room_num = vehicle->room_num;
        sector = Room_GetSector(test_pos, &room_num);
        height = Room_GetHeight(sector, test_pos);
        if (height < old->y - STEP_L) {
            if (pos->z > old->z) {
                z = -shift_z - 1;
            } else {
                z = WALL_L - shift_z;
            }
        }

        test_pos = (XYZ_32) { pos->x, pos->y, old->z };
        room_num = vehicle->room_num;
        sector = Room_GetSector(test_pos, &room_num);
        height = Room_GetHeight(sector, test_pos);
        if (height < old->y - STEP_L) {
            if (pos->x > old->x) {
                x = -shift_x - 1;
            } else {
                x = WALL_L - shift_x;
            }
        }

        if (x != 0 && z != 0) {
            vehicle->pos.x += x;
            vehicle->pos.z += z;
        } else if (z != 0) {
            vehicle->pos.z += z;
            if (z > 0) {
                return vehicle->pos.x - pos->x;
            } else {
                return pos->x - vehicle->pos.x;
            }
        } else if (x != 0) {
            vehicle->pos.x += x;
            if (x > 0) {
                return pos->z - vehicle->pos.z;
            } else {
                return vehicle->pos.z - pos->z;
            }
        } else {
            vehicle->pos.x += old->x - pos->x;
            vehicle->pos.z += old->z - pos->z;
        }
    }

    return 0;
}

int32_t Vehicle_GetCollisionAnim(const ITEM *const vehicle, XYZ_32 *const moved)
{
    moved->x = vehicle->pos.x - moved->x;
    moved->z = vehicle->pos.z - moved->z;

    if (moved->x != 0 || moved->z != 0) {
        const XYZ_32 local = XYZ_32_UnrotateYaw(*moved, vehicle->rot.y);
        const int32_t front = local.z;
        const int32_t side = local.x;
        if (ABS(front) > ABS(side)) {
            if (front > 0) {
                return LA_VEHICLE_HIT_BACK;
            } else {
                return LA_VEHICLE_HIT_FRONT;
            }
        } else {
            if (side > 0) {
                return LA_VEHICLE_HIT_LEFT;
            } else {
                return LA_VEHICLE_HIT_RIGHT;
            }
        }
    }

    return 0;
}

void Vehicle_PlayTrackPool(
    const ITEM *const item, const char *const key_prefix,
    const MUSIC_PLAY_MODE mode)
{
    MUSIC_SLOT tracks[VEHICLE_TRACK_POOL_SIZE];
    const int32_t track_count =
        M_LoadTrackPool(item, key_prefix, tracks, ARRAY_SIZE(tracks));
    if (track_count <= 0) {
        return;
    }
    Music_PlayBySlot(tracks[Random_GetControl() % track_count], mode);
}

void Vehicle_PlayOneShotTrackPool(
    const ITEM *const item, const char *const key_prefix)
{
    MUSIC_SLOT tracks[VEHICLE_TRACK_POOL_SIZE];
    const int32_t track_count =
        M_LoadTrackPool(item, key_prefix, tracks, ARRAY_SIZE(tracks));
    if (track_count <= 0) {
        return;
    }

    for (int32_t i = 0; i < track_count; i++) {
        if (Music_GetTrackState(tracks[i])->is_one_shot) {
            return;
        }
    }

    const MUSIC_SLOT track = tracks[Random_GetControl() % track_count];
    Music_PlayBySlot(track, MPM_ONCE);
    Music_GetTrackState(track)->is_one_shot = true;
}

void Vehicle_TestTriggers(
    const ITEM *const lara_item, const ITEM *const vehicle_item)
{
    Room_TestTriggers(lara_item);
    TRX_VALUE value = {};
    if (ObjectProperty_GetItemValue(vehicle_item, "is_heavy", &value)
        && value.as_bool) {
        Room_TestTriggers(vehicle_item);
    }
}

void Vehicle_HandleEvent(
    ITEM *const item, const OBJECT_EVENT event, const void *const data)
{
    if (event != OBJECT_EVENT_FLOOR_MOVED) {
        return;
    }

    const int32_t shift = (int32_t)(intptr_t)data;
    item->pos.y += shift;
    item->floor += shift;

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    if (room_num != item->room_num) {
        Item_UpdateRoom(Item_GetIndex(item), room_num);
    }
}
