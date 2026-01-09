#include <trx/game/camera.h>
#include <trx/game/rooms.h>

// clang-format off
#define M_WALL_MASK (WALL_L - 1)
#define M_LOS_STEPS 8
// clang-format on

static bool M_LOS(
    GAME_VECTOR *const start, GAME_VECTOR *const target, const int32_t shift)
{
    const XYZ_32 delta = {
        .x = (target->x - start->x) >> 3,
        .y = (target->y - start->y) >> 3,
        .z = (target->z - start->z) >> 3,
    };

    XYZ_32 pos = start->pos;
    int16_t room_num = start->room_num;
    bool valid_space = false;
    bool clipped = false;

    int32_t i = 0;
    for (; i < M_LOS_STEPS; i++) {
        int16_t next_room_num = room_num;
        const SECTOR *const sector =
            Room_GetSector(pos.x, pos.y, pos.z, &next_room_num);

        if (Room_Get(next_room_num)->flags.swamp) {
            clipped = true;
            break;
        }

        const int32_t height =
            Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
        const int32_t ceiling =
            Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);

        if (height == NO_HEIGHT || ceiling == NO_HEIGHT || ceiling >= height) {
            if (!valid_space) {
                pos.x += delta.x;
                pos.y += delta.y;
                pos.z += delta.z;
                continue;
            }

            clipped = true;
            break;
        }

        if (pos.y > height) {
            const int32_t height_diff = pos.y - height;
            if (height_diff < shift) {
                pos.y = height;
            } else {
                clipped = true;
                break;
            }
        }

        if (pos.y < ceiling) {
            const int32_t ceiling_diff = ceiling - pos.y;
            if (ceiling_diff < shift) {
                pos.y = ceiling;
            } else {
                clipped = true;
                break;
            }
        }

        valid_space = true;
        room_num = next_room_num;
        pos.x += delta.x;
        pos.y += delta.y;
        pos.z += delta.z;
    }

    if (i != 0) {
        pos.x -= delta.x;
        pos.y -= delta.y;
        pos.z -= delta.z;
    }

    Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    target->pos = pos;
    target->room_num = room_num;

    return !clipped;
}

static inline void M_ClampY(int16_t room_num, XYZ_32 *const pos)
{
    const SECTOR *const sector =
        Room_GetSector(pos->x, pos->y, pos->z, &room_num);
    const int16_t height =
        Room_GetHeightEx(sector, pos->x, pos->y, pos->z, true, NO_ITEM);
    const int16_t ceiling =
        Room_GetCeilingEx(sector, pos->x, pos->y, pos->z, true);

    if (ceiling < height && ceiling != NO_HEIGHT && height != NO_HEIGHT) {
        if (ceiling > pos->y - 255 && height < pos->y + 255) {
            pos->y = (ceiling + height) >> 1;
        } else if (height < pos->y + 255) {
            pos->y = height - 255;
        } else if (ceiling > pos->y - 255) {
            pos->y = ceiling + 255;
        }
    }
}

static bool M_Collide(
    GAME_VECTOR *const ideal, const int32_t shift, const bool y_first)
{
    XYZ_32 pos = ideal->pos;
    if (y_first) {
        M_ClampY(ideal->room_num, &pos);
    }

#define L_OUT_OF_BOUNDS                                                        \
    (height < pos.y || height == NO_HEIGHT || ceiling == NO_HEIGHT             \
     || ceiling >= height || pos.y < ceiling)

    // -X clamp
    int16_t room_num = ideal->room_num;
    const SECTOR *sector =
        Room_GetSector(pos.x - shift, pos.y, pos.z, &room_num);
    int16_t height =
        Room_GetHeightEx(sector, pos.x - shift, pos.y, pos.z, true, NO_ITEM);
    int16_t ceiling =
        Room_GetCeilingEx(sector, pos.x - shift, pos.y, pos.z, true);
    if (L_OUT_OF_BOUNDS) {
        pos.x = shift + (pos.x & ~M_WALL_MASK);
    }

    // -Z clamp
    room_num = ideal->room_num;
    sector = Room_GetSector(pos.x, pos.y, pos.z - shift, &room_num);
    height =
        Room_GetHeightEx(sector, pos.x, pos.y, pos.z - shift, true, NO_ITEM);
    ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z - shift, true);
    if (L_OUT_OF_BOUNDS) {
        pos.z = shift + (pos.z & ~M_WALL_MASK);
    }

    // +X clamp
    room_num = ideal->room_num;
    sector = Room_GetSector(pos.x + shift, pos.y, pos.z, &room_num);
    height =
        Room_GetHeightEx(sector, pos.x + shift, pos.y, pos.z, true, NO_ITEM);
    ceiling = Room_GetCeilingEx(sector, pos.x + shift, pos.y, pos.z, true);
    if (L_OUT_OF_BOUNDS) {
        pos.x = (pos.x | M_WALL_MASK) - shift;
    }

    // +Z clamp
    room_num = ideal->room_num;
    sector = Room_GetSector(pos.x, pos.y, pos.z + shift, &room_num);
    height =
        Room_GetHeightEx(sector, pos.x, pos.y, pos.z + shift, true, NO_ITEM);
    ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z + shift, true);
    if (L_OUT_OF_BOUNDS) {
        pos.z = (pos.z | M_WALL_MASK) - shift;
    }

    if (!y_first) {
        M_ClampY(ideal->room_num, &pos);
    }

    room_num = ideal->room_num;
    sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    height = Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
    ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);
    if (L_OUT_OF_BOUNDS) {
        return true;
    }

#undef L_OUT_OF_BOUNDS

    Room_GetSector(pos.x, pos.y, pos.z, &ideal->room_num);
    ideal->pos = pos;
    return false;
}
