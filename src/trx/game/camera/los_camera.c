#include <trx/game/camera.h>
#include <trx/game/lara.h>
#include <trx/game/rooms.h>

// clang-format off
#define M_WALL_MASK         (WALL_L - 1)
#define M_LOS_STEPS         8
#define M_MAX_SNAPS         8
#define M_SNAP_DELTA        (STEP_L * 3) // = 768
#define M_DEFAULT_ELEVATION (-10 * DEG_1) // = -1820
#define M_MAX_ELEVATION     (85 * DEG_1) // = 15470
#define M_CHASE_SHIFT       (STEP_L * 3 / 2) // = 384
// clang-format on

typedef struct {
    struct {
        int16_t current_anim_state;
        int16_t goal_anim_state;
        XYZ_32 pos;
        XYZ_16 rot;
        XYZ_16 head_rot;
        XYZ_16 torso_rot;
    } lara;
    CAMERA_TYPE cam_type;
} M_STATE;

typedef struct {
    GAME_VECTOR pos;
    GAME_VECTOR target;
} M_IDEAL;

static M_STATE m_LastState = {};
static M_IDEAL m_LastIdeal = {};
static int32_t m_Snaps = 0;

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

static bool M_UpdateLaraState(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    const bool same_lara_state =
        XYZ_16_AreEquivalent(&m_LastState.lara.rot, &lara_item->rot)
        && XYZ_16_AreEquivalent(&m_LastState.lara.head_rot, &lara->head_rot)
        && XYZ_16_AreEquivalent(&m_LastState.lara.torso_rot, &lara->torso_rot)
        && XYZ_32_AreEquivalent(&m_LastState.lara.pos, &lara_item->pos)
        && m_LastState.lara.current_anim_state == lara_item->current_anim_state
        && m_LastState.lara.goal_anim_state == lara_item->goal_anim_state;
    if (same_lara_state && m_LastState.cam_type == g_Camera.type) {
        return false;
    }

    m_LastState.lara.rot = lara_item->rot;
    m_LastState.lara.head_rot = lara->head_rot;
    m_LastState.lara.torso_rot = lara->torso_rot;
    m_LastState.lara.pos = lara_item->pos;
    m_LastState.lara.current_anim_state = lara_item->current_anim_state;
    m_LastState.lara.goal_anim_state = lara_item->goal_anim_state;
    return true;
}

static void M_Move(GAME_VECTOR *const ideal, const int32_t speed)
{
    if (M_UpdateLaraState()) {
        m_LastIdeal.pos = *ideal;
    } else {
        *ideal = m_LastIdeal.pos;
    }

    g_Camera.pos.x += (ideal->x - g_Camera.pos.x) / speed;
    g_Camera.pos.y += (ideal->y - g_Camera.pos.y) / speed;
    g_Camera.pos.z += (ideal->z - g_Camera.pos.z) / speed;
    g_Camera.pos.room_num = ideal->room_num;

    Camera_ApplyBounce();

    XYZ_32 pos = g_Camera.pos.pos;
    int16_t room_num = g_Camera.pos.room_num;
    const SECTOR *sector =
        Room_GetSector(pos.x, pos.y + STEP_L, pos.z, &room_num);

    const ROOM *const room = Room_Get(room_num);
    if (room->flags.swamp) {
        pos.y = room->max_ceiling - STEP_L;
        Room_GetSector(pos.x, pos.y, pos.z, &g_Camera.pos.room_num);
    }

    sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    int16_t height =
        Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
    int16_t ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);

    if (pos.y < ceiling || pos.y > height) {
        M_LOS(&g_Camera.target, &g_Camera.pos, 0);
        const XYZ_32 delta = {
            .x = ABS(g_Camera.pos.x - ideal->x),
            .y = ABS(g_Camera.pos.y - ideal->y),
            .z = ABS(g_Camera.pos.z - ideal->z),
        };

        if (delta.x < M_SNAP_DELTA && delta.y < M_SNAP_DELTA
            && delta.z < M_SNAP_DELTA) {
            GAME_VECTOR start = *ideal;
            GAME_VECTOR target = g_Camera.pos;

            if (!M_LOS(&start, &target, 0)) {
                m_Snaps++;
                if (m_Snaps >= M_MAX_SNAPS) {
                    g_Camera.pos = *ideal;
                    m_Snaps = 0;
                }
            }
        }
    }

    pos = g_Camera.pos.pos;
    room_num = g_Camera.pos.room_num;
    sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    height = Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
    ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);

    if (pos.y - 255 < ceiling && pos.y + 255 > height && ceiling < height
        && ceiling != NO_HEIGHT && height != NO_HEIGHT) {
        g_Camera.pos.y = (ceiling + height) >> 1;
    } else if (
        pos.y + 255 > height && ceiling < height && ceiling != NO_HEIGHT
        && height != NO_HEIGHT) {
        g_Camera.pos.y = height - 255;
    } else if (
        pos.y - 255 < ceiling && ceiling < height && ceiling != NO_HEIGHT
        && height != NO_HEIGHT) {
        g_Camera.pos.y = ceiling + 255;
    } else if (
        ceiling >= height || height == NO_HEIGHT || ceiling == NO_HEIGHT) {
        g_Camera.pos = *ideal;
    }

    Room_GetSector(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z, &g_Camera.pos.room_num);
    m_LastState.cam_type = g_Camera.type;

    Camera_UpdateMicPosition();
}

static GAME_VECTOR M_GetIdeal(
    const int32_t distance, const int16_t target_rot_y)
{
    int32_t farthest = 0x7FFFFFFF;
    int32_t farthest_num = 0;
    GAME_VECTOR temp[2] = {};
    GAME_VECTOR ideals[5] = {};

    for (int32_t i = 0; i < 5; i++) {
        ideals[i].y =
            ((Math_Sin(g_Camera.target_elevation) * g_Camera.target_distance)
             >> W2V_SHIFT)
            + g_Camera.target.y;
    }

    for (int32_t i = 0; i < 5; i++) {
        const int16_t angle = i > 0 ? ((i - 1) << W2V_SHIFT)
                                    : (g_Camera.target_angle + target_rot_y);
        ideals[i].x =
            g_Camera.target.x - ((distance * Math_Sin(angle)) >> W2V_SHIFT);
        ideals[i].z =
            g_Camera.target.z - ((distance * Math_Cos(angle)) >> W2V_SHIFT);

        ideals[i].room_num = g_Camera.target.room_num;

        if (M_LOS(&g_Camera.target, &ideals[i], 200)) {
            temp[0] = ideals[i];
            temp[1] = g_Camera.pos;

            if (i == 0 || M_LOS(&temp[0], &temp[1], 0)) {
                if (i == 0) {
                    farthest_num = 0;
                    break;
                }

                const int32_t dx = SQUARE(g_Camera.pos.x - ideals[i].x);
                const int32_t dz = SQUARE(g_Camera.pos.z - ideals[i].z) + dx;
                if (dz < farthest) {
                    farthest = dz;
                    farthest_num = i;
                }
            }
        } else if (i == 0) {
            temp[0] = ideals[i];
            temp[1] = g_Camera.pos;

            const int32_t dx = SQUARE(g_Camera.target.x - ideals[i].x);
            const int32_t dz = SQUARE(g_Camera.target.z - ideals[i].z) + dx;
            if (dz > 0x90000) {
                farthest_num = 0;
                break;
            }
        }
    }

    return ideals[farthest_num];
}

static void M_Chase(const ITEM *const item)
{
    if (g_Camera.target_elevation == 0) {
        g_Camera.target_elevation = M_DEFAULT_ELEVATION;
    }
    g_Camera.target_elevation += item->rot.x;
    CLAMP(g_Camera.target_elevation, -M_MAX_ELEVATION, M_MAX_ELEVATION);

    const int32_t distance =
        (g_Camera.target_distance * Math_Cos(g_Camera.target_elevation))
        >> W2V_SHIFT;

    int16_t room_num = g_Camera.target.room_num;
    const SECTOR *sector = Room_GetSector(
        g_Camera.target.x, g_Camera.target.y + STEP_L, g_Camera.target.z,
        &room_num);

    const ROOM *const room = Room_Get(room_num);
    if (room->flags.swamp) {
        g_Camera.target.y = room->max_ceiling - STEP_L;
    }

    XYZ_32 pos = g_Camera.target.pos;
    sector = Room_GetSector(pos.x, pos.y, pos.z, &g_Camera.target.room_num);
    int16_t height =
        Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
    int16_t ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);

    if (ceiling + 16 > height - 16 && height != NO_HEIGHT
        && ceiling != NO_HEIGHT) {
        g_Camera.target.y = (height + ceiling) >> 1;
        g_Camera.target_elevation = 0;
    } else if (pos.y > height - 16 && height != NO_HEIGHT) {
        g_Camera.target.y = height - 16;
        g_Camera.target_elevation = 0;
    } else if (pos.y < ceiling + 16 && ceiling != NO_HEIGHT) {
        g_Camera.target.y = ceiling + 16;
        g_Camera.target_elevation = 0;
    }

    sector = Room_GetSector(
        g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
        &g_Camera.target.room_num);
    pos = g_Camera.target.pos;
    room_num = g_Camera.target.room_num;
    sector = Room_GetSector(
        g_Camera.target.x, g_Camera.target.y, g_Camera.target.z, &room_num);
    height = Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
    ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);

    if (pos.y < ceiling || pos.y > height || ceiling >= height
        || height == NO_HEIGHT || ceiling == NO_HEIGHT) {
        g_Camera.target = m_LastIdeal.target;
    }

    GAME_VECTOR ideal = M_GetIdeal(distance, item->rot.y);
    M_Collide(&ideal, M_CHASE_SHIFT, true);

    if (m_LastState.cam_type == CAM_FIXED) {
        g_Camera.speed = 1;
    }

    M_Move(&ideal, g_Camera.speed);
}

static void M_Combat(const ITEM *const item)
{
    g_Camera.target.x = item->pos.x;
    g_Camera.target.z = item->pos.z;

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->target != nullptr) {
        g_Camera.target_angle = lara->target_angles[0] + item->rot.y;
        g_Camera.target_elevation = lara->target_angles[1] + item->rot.x;
    } else {
        g_Camera.target_angle =
            lara->torso_rot.y + lara->head_rot.y + item->rot.y;
        g_Camera.target_elevation =
            lara->head_rot.x + item->rot.x + lara->torso_rot.x - 2730;
    }

    int16_t room_num = g_Camera.target.room_num;
    const SECTOR *sector = Room_GetSector(
        g_Camera.target.x, g_Camera.target.y + STEP_L, g_Camera.target.z,
        &room_num);

    const ROOM *const room = Room_Get(room_num);
    if (room->flags.swamp) {
        g_Camera.target.y = room->max_ceiling - STEP_L;
    }

    XYZ_32 pos = g_Camera.target.pos;
    sector = Room_GetSector(pos.x, pos.y, pos.z, &g_Camera.target.room_num);
    int16_t height =
        Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
    int16_t ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);

    if (ceiling + 64 > height - 64 && height != NO_HEIGHT
        && ceiling != NO_HEIGHT) {
        g_Camera.target.y = (ceiling + height) >> 1;
        g_Camera.target_elevation = 0;
    } else if (pos.y > height - 64 && height != NO_HEIGHT) {
        g_Camera.target.y = height - 64;
        g_Camera.target_elevation = 0;
    } else if (pos.y < ceiling + 64 && ceiling != NO_HEIGHT) {
        g_Camera.target.y = ceiling + 64;
        g_Camera.target_elevation = 0;
    }

    pos = g_Camera.target.pos;
    Room_GetSector(pos.x, pos.y, pos.z, &g_Camera.target.room_num);
    room_num = g_Camera.target.room_num;
    sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    height = Room_GetHeightEx(sector, pos.x, pos.y, pos.z, true, NO_ITEM);
    ceiling = Room_GetCeilingEx(sector, pos.x, pos.y, pos.z, true);

    if (pos.y < ceiling || pos.y > height || ceiling >= height
        || height == NO_HEIGHT || ceiling == NO_HEIGHT) {
        g_Camera.target = m_LastIdeal.target;
    }

    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
    const int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    GAME_VECTOR ideal = M_GetIdeal(distance, 0);
    M_Collide(&ideal, M_CHASE_SHIFT, true);

    if (m_LastState.cam_type == CAM_FIXED) {
        g_Camera.speed = 1;
    }

    M_Move(&ideal, g_Camera.speed);
}
