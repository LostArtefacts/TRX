#include "game/camera/common.h"

#include "config.h"
#include "debug.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/los.h"
#include "game/matrix.h"
#include "game/music.h"
#include "game/pathing.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "utils.h"

// clang-format off
#define M_MAX_ELEVATION     (85 * DEG_1) // = 15470
#define M_COMBAT_DISTANCE   (WALL_L * 5 / 2) // = 2560
#define M_LOOK_DISTANCE     (WALL_L * 3 / 2) // = 1536
#define M_LOOK_CLAMP        (STEP_L + 50) // = 296
#define M_MAX_HEAD_ROTATION (50 * DEG_1) // = 9100
#define M_MIN_HEAD_ROTATION (-M_MAX_HEAD_ROTATION) // = -9100
#define M_MAX_HEAD_TILT     (85 * DEG_1) // = 15470
#define M_MIN_HEAD_TILT     (-M_MAX_HEAD_TILT) // = -15470
#define M_HEAD_TURN         (4 * DEG_1) // = 728
#define M_CHASE_ELEVATION   (WALL_L * 3 / 2) // = 1536
#define M_COMBAT_SPEED      8
#define M_LOOK_SPEED        4
// clang-format on

#define M_SHIFT_ARGS                                                           \
    int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,    \
        int32_t target_h, int32_t left, int32_t top, int32_t right,            \
        int32_t bottom

typedef struct {
    int16_t chase_speed;
    int32_t min_square;
    int32_t shift_scale;
    bool test_shift_pair;
    bool clip_shift_height;
    bool test_early_lb_shift;
} M_SETTINGS;

static const M_SETTINGS m_CameraSettings[CAMERA_MODE_NUMBER_OF] = {
    [CAMERA_MODE_TR1] = {
        .chase_speed = 12,
        .min_square = SQUARE(WALL_L / 4),
        .shift_scale = 1,
        .test_shift_pair = false,
        .clip_shift_height = false,
        .test_early_lb_shift = true,
    },
    [CAMERA_MODE_TR2] = {
        .chase_speed = 10,
        .min_square = SQUARE(WALL_L / 3),
        .shift_scale = 2,
        .test_shift_pair = true,
        .clip_shift_height = true,
        .test_early_lb_shift = false,
    },
};

// Camera speed option ranges from 1-10, so index 0 is unused.
static const double m_ManualCameraMultiplier[11] = {
    1.0, .5, .625, .75, .875, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0,
};

static BOX_INFO m_FixedBox = {};
static bool m_IsChunky = false;
static bool m_IsInitialised = false;
#if TR_VERSION == 2
// TODO: consolidate with Viewport API
extern int32_t g_PhdPersp;
#endif

static M_SETTINGS M_GetSettings(void)
{
    return m_CameraSettings[g_Config.visuals.camera_mode];
}

static void M_AdjustMusicVolume(const bool is_underwater)
{
    if (!Game_IsPlaying()) {
        return;
    }
    const bool is_ambient =
        Music_GetCurrentPlayingTrack() == Music_GetCurrentLoopedTrack();
    const bool is_cutscene = GF_GetCurrentLevel()->type == GFL_CUTSCENE;
    const double base_volume = is_cutscene ? g_Config.audio.cutscene_volume
        : is_ambient                       ? g_Config.audio.ambient_volume
                                           : g_Config.audio.music_volume;
    const double multiplier = !is_underwater || is_cutscene ? 1.0
        : is_ambient ? g_Config.audio.underwater_ambient_volume
                     : g_Config.audio.underwater_music_volume;
    Music_SetVolume(base_volume * multiplier);
}

static void M_OffsetAdditionalAngle(const int16_t delta)
{
    g_Camera.additional_angle += delta;
}

static void M_OffsetAdditionalElevation(const int16_t delta)
{
    // Do not allow elevation to overflow.
    int32_t new_elevation = g_Camera.additional_elevation + delta;
    CLAMP(new_elevation, INT16_MIN, INT16_MAX);
    g_Camera.additional_elevation = new_elevation;
}

static void M_OffsetReset(void)
{
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
}

static const BOX_INFO *M_GetBox(
    const SECTOR *const sector, const int32_t x, const int32_t z,
    const bool generate_box)
{
    if (sector->box != NO_BOX) {
        return Box_GetBox(sector->box);
    }

    if (!generate_box) {
        return nullptr;
    }

    // A level may have blocked specific sector or room pathfinding, so create a
    // dummy one-sector box to prevent erratic camera positioning.
    m_FixedBox.left = z & ~(WALL_L - 1);
    m_FixedBox.top = x & ~(WALL_L - 1);
    m_FixedBox.right = m_FixedBox.left + WALL_L - 1;
    m_FixedBox.bottom = m_FixedBox.top + WALL_L - 1;
    return &m_FixedBox;
}

static const SECTOR *M_GetSector(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int32_t height = Room_GetHeight(sector, x, y, z);
    const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
    if (y > height || y < ceiling) {
        return nullptr;
    }
    return sector;
}

static bool M_IsGoodPosition(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    return M_GetSector(x, y, z, room_num) != nullptr;
}

static int32_t M_ShiftClamp(GAME_VECTOR *const pos, const int32_t clamp)
{
    const int32_t x = pos->x;
    const int32_t y = pos->y;
    const int32_t z = pos->z;

    const SECTOR *const sector = Room_GetSector(x, y, z, &pos->room_num);
    const BOX_INFO *const box = M_GetBox(sector, x, z, true);

    const int32_t left = box->left + clamp;
    const int32_t right = box->right - clamp;
    if (z < left && !M_IsGoodPosition(x, y, z - clamp, pos->room_num)) {
        pos->z = left;
    } else if (z > right && !M_IsGoodPosition(x, y, z + clamp, pos->room_num)) {
        pos->z = right;
    }

    const int32_t top = box->top + clamp;
    const int32_t bottom = box->bottom - clamp;
    if (x < top && !M_IsGoodPosition(x - clamp, y, z, pos->room_num)) {
        pos->x = top;
    } else if (
        x > bottom && !M_IsGoodPosition(x + clamp, y, z, pos->room_num)) {
        pos->x = bottom;
    }

    int32_t height = Room_GetHeight(sector, x, y, z) - clamp;
    int32_t ceiling = Room_GetCeiling(sector, x, y, z) + clamp;

    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (y > height) {
        return height - y;
    } else if (y < ceiling) {
        return ceiling - y;
    }

    return 0;
}

static void M_SmartShift(GAME_VECTOR *const target, void (*shift)(M_SHIFT_ARGS))
{
    LOS_Check(&g_Camera.target, target);

    const ROOM *room = Room_Get(g_Camera.target.room_num);
    const SECTOR *sector =
        Room_GetWorldSector(room, g_Camera.target.x, g_Camera.target.z);
    const BOX_INFO *box =
        M_GetBox(sector, g_Camera.target.x, g_Camera.target.z, true);

    room = Room_Get(target->room_num);
    sector = Room_GetWorldSector(room, target->x, target->z);

    if (target->z < box->left || target->z > box->right || target->x < box->top
        || target->x > box->bottom) {
        box = M_GetBox(sector, target->x, target->z, true);
    }

    int32_t left = box->left;
    int32_t right = box->right;
    int32_t top = box->top;
    int32_t bottom = box->bottom;

    const M_SETTINGS settings = M_GetSettings();

    int32_t test = (target->z - WALL_L) | (WALL_L - 1);
    const SECTOR *const good_left =
        M_GetSector(target->x, target->y, test, target->room_num);
    if (good_left != nullptr) {
        box = M_GetBox(good_left, target->x, test, false);
        if (box != nullptr && box->left < left) {
            left = box->left;
        }
    } else if (settings.test_shift_pair) {
        left = test;
    }

    test = (target->z + WALL_L) & (~(WALL_L - 1));
    const SECTOR *const good_right =
        M_GetSector(target->x, target->y, test, target->room_num);
    if (good_right != nullptr) {
        box = M_GetBox(good_right, target->x, test, false);
        if (box != nullptr && box->right > right) {
            right = box->right;
        }
    } else if (settings.test_shift_pair) {
        right = test;
    }

    test = (target->x - WALL_L) | (WALL_L - 1);
    const SECTOR *const good_top =
        M_GetSector(test, target->y, target->z, target->room_num);
    if (good_top != nullptr) {
        box = M_GetBox(good_top, test, target->z, false);
        if (box != nullptr && box->top < top) {
            top = box->top;
        }
    } else if (settings.test_shift_pair) {
        top = test;
    }

    test = (target->x + WALL_L) & (~(WALL_L - 1));
    const SECTOR *const good_bottom =
        M_GetSector(test, target->y, target->z, target->room_num);
    if (good_bottom != nullptr) {
        box = M_GetBox(good_bottom, test, target->z, false);
        if (box != nullptr && box->bottom > bottom) {
            bottom = box->bottom;
        }
    } else if (settings.test_shift_pair) {
        bottom = test;
    }

    left += STEP_L;
    right -= STEP_L;
    top += STEP_L;
    bottom -= STEP_L;

    GAME_VECTOR target_a = {
        .x = target->x,
        .y = target->y,
        .z = target->z,
        .room_num = target->room_num,
    };

    GAME_VECTOR target_b = {
        .x = target->x,
        .y = target->y,
        .z = target->z,
        .room_num = target->room_num,
    };

    bool clip = false;
    bool prefer_a = !settings.test_shift_pair;

#define SHIFT(axis1, axis2, l1, l2, r1, r2)                                    \
    shift(                                                                     \
        &target_a.axis1, &target_a.axis2, &target_a.y, g_Camera.target.axis1,  \
        g_Camera.target.axis2, g_Camera.target.y, l1, l2, r1, r2);             \
    shift(                                                                     \
        &target_b.axis1, &target_b.axis2, &target_b.y, g_Camera.target.axis1,  \
        g_Camera.target.axis2, g_Camera.target.y, l1, r2, r1, l2)

    if (!settings.test_shift_pair) {
        if (target->z < left && good_left == nullptr) {
            clip = true;
            if (target->x < g_Camera.target.x) {
                SHIFT(z, x, left, top, right, bottom);
            } else {
                SHIFT(z, x, left, bottom, right, top);
            }
        } else if (target->z > right && good_right == nullptr) {
            clip = true;
            if (target->x < g_Camera.target.x) {
                SHIFT(z, x, right, top, left, bottom);
            } else {
                SHIFT(z, x, right, bottom, left, top);
            }
        } else if (target->x < top && good_top == nullptr) {
            clip = true;
            if (target->z < g_Camera.target.z) {
                SHIFT(x, z, top, left, bottom, right);
            } else {
                SHIFT(x, z, top, right, bottom, left);
            }
        } else if (target->x > bottom && good_bottom == nullptr) {
            clip = true;
            if (target->z < g_Camera.target.z) {
                SHIFT(x, z, bottom, left, top, right);
            } else {
                SHIFT(x, z, bottom, right, top, left);
            }
        }
    } else {
        if (ABS(target->z - g_Camera.target.z)
            > ABS(target->x - g_Camera.target.x)) {
            if (target->z < left && good_left == nullptr) {
                clip = true;
                prefer_a = g_Camera.pos.x < g_Camera.target.x;
                SHIFT(z, x, left, top, right, bottom);
            } else if (target->z > right && good_right == nullptr) {
                clip = true;
                prefer_a = g_Camera.pos.x < g_Camera.target.x;
                SHIFT(z, x, right, top, left, bottom);
            } else if (target->x < top && good_top == nullptr) {
                clip = true;
                prefer_a = target->z < g_Camera.target.z;
                SHIFT(x, z, top, left, bottom, right);
            } else if (target->x > bottom && good_bottom == nullptr) {
                clip = true;
                prefer_a = target->z < g_Camera.target.z;
                SHIFT(x, z, bottom, left, top, right);
            }
        } else {
            if (target->x < top && good_top == nullptr) {
                clip = true;
                prefer_a = g_Camera.pos.z < g_Camera.target.z;
                SHIFT(x, z, top, left, bottom, right);
            } else if (target->x > bottom && good_bottom == nullptr) {
                clip = true;
                prefer_a = g_Camera.pos.z < g_Camera.target.z;
                SHIFT(x, z, bottom, left, top, right);
            } else if (target->z < left && good_left == nullptr) {
                clip = true;
                prefer_a = target->x < g_Camera.target.x;
                SHIFT(z, x, left, top, right, bottom);
            } else if (target->z > right && good_right == nullptr) {
                clip = true;
                prefer_a = target->x < g_Camera.target.x;
                SHIFT(z, x, right, top, left, bottom);
            }
        }
    }

#undef SHIFT

    if (clip == CLIP_NOT_VISIBLE) {
        return;
    }

    if (settings.test_shift_pair) {
        if (prefer_a) {
            prefer_a = LOS_Check(&g_Camera.target, &target_a);
        } else {
            prefer_a = !LOS_Check(&g_Camera.target, &target_b);
        }
    }

    if (prefer_a) {
        target->pos = target_a.pos;
    } else {
        target->pos = target_b.pos;
    }

    Room_GetSector(target->x, target->y, target->z, &target->room_num);
}

static void M_Clip(M_SHIFT_ARGS)
{
    const int32_t x_diff = *x - target_x;
    const int32_t y_diff = *y - target_y;
    const int32_t h_diff = *h - target_h;
    int32_t height = *h;

    if ((right > left) != (target_x < left)) {
        if (x_diff != 0) {
            *y = target_y + (left - target_x) * y_diff / x_diff;
            height = target_h + (left - target_x) * h_diff / x_diff;
        }
        *x = left;
    }

    if ((bottom > top && target_y > top && (*y) < top)
        || (bottom < top && target_y < top && (*y) > top)) {
        if (y_diff != 0) {
            *x = target_x + (top - target_y) * x_diff / y_diff;
            height = target_h + (top - target_y) * h_diff / y_diff;
        }
        *y = top;
    }

    const M_SETTINGS settings = M_GetSettings();
    if (settings.clip_shift_height) {
        *h = height;
    }
}

static void M_Shift(M_SHIFT_ARGS)
{
    const int32_t l_square = SQUARE(target_x - left);
    const int32_t r_square = SQUARE(target_x - right);
    const int32_t t_square = SQUARE(target_y - top);
    const int32_t b_square = SQUARE(target_y - bottom);

    const int32_t tl_square = t_square + l_square;
    const int32_t tr_square = t_square + r_square;
    const int32_t bl_square = b_square + l_square;

    const M_SETTINGS settings = M_GetSettings();
    const int32_t scaled_target = g_Camera.target_square * settings.shift_scale;

    int32_t shift;
    if (g_Camera.target_square < tl_square) {
        *x = left;
        shift = g_Camera.target_square - l_square;
        if (shift >= 0) {
            shift = Math_Sqrt(shift);
            *y = target_y + (top >= bottom ? shift : -shift);
        }
    } else if (tl_square > settings.min_square) {
        *x = left;
        *y = top;
    } else if (g_Camera.target_square < bl_square) {
        *x = left;
        shift = g_Camera.target_square - l_square;
        if (shift >= 0) {
            shift = Math_Sqrt(shift);
            *y = target_y + (top < bottom ? shift : -shift);
        }
    } else if (
        settings.test_early_lb_shift && bl_square > settings.min_square) {
        *x = left;
        *y = bottom;
    } else if (scaled_target < tr_square) {
        shift = scaled_target - t_square;
        if (shift >= 0) {
            shift = Math_Sqrt(shift);
            *x = target_x + (left < right ? shift : -shift);
            *y = top;
        }
    } else if (settings.test_early_lb_shift || bl_square <= tr_square) {
        *x = right;
        *y = top;
    } else {
        *x = left;
        *y = bottom;
    }
}

static void M_Move(const GAME_VECTOR *const target, const int32_t speed)
{
    const GAME_VECTOR old_pos = g_Camera.pos;
    GAME_VECTOR pos = g_Camera.pos;
    pos.x += (target->x - pos.x) / speed;
    pos.z += (target->z - pos.z) / speed;
    pos.y += (target->y - pos.y) / speed;
    pos.room_num = target->room_num;

    Camera_SetChunky(false);

    const SECTOR *sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
    int32_t height = Room_GetHeight(sector, pos.x, pos.y, pos.z);
    if (height == NO_HEIGHT) {
        // Attempt to clamp within the previous sector's height bounds. Only if
        // that fails continue to revert fully to the last good Y position.
        pos.room_num = old_pos.room_num;
        sector = Room_GetSector(old_pos.x, old_pos.y, old_pos.z, &pos.room_num);
        height = Room_GetHeight(sector, old_pos.x, old_pos.y, old_pos.z);
        const int32_t old_ceiling =
            Room_GetCeiling(sector, old_pos.x, old_pos.y, old_pos.z);
        CLAMP(pos.y, old_ceiling + STEP_L, height - STEP_L);
        sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
        height = Room_GetHeight(sector, pos.x, pos.y, pos.z);
        if (height == NO_HEIGHT) {
            pos.y = old_pos.y;
            pos.room_num = old_pos.room_num;
            sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
            height = Room_GetHeight(sector, pos.x, pos.y, pos.z);
        }
    }

    height -= STEP_L;
    if (pos.y >= height && target->y >= height) {
        LOS_Check(&g_Camera.target, &pos);
        sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
        height = Room_GetHeight(sector, pos.x, pos.y, pos.z) - STEP_L;
    }

    g_Camera.pos = pos;

    int32_t ceiling = Room_GetCeiling(sector, pos.x, pos.y, pos.z) + STEP_L;
    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (g_Camera.bounce > 0) {
        g_Camera.pos.y += g_Camera.bounce;
        g_Camera.target.y += g_Camera.bounce;
        g_Camera.bounce = 0;
    } else if (g_Camera.bounce < 0) {
        const XYZ_32 shake = {
            .x = g_Camera.bounce * (Random_GetControl() - 0x4000) / 0x7FFF,
            .y = g_Camera.bounce * (Random_GetControl() - 0x4000) / 0x7FFF,
            .z = g_Camera.bounce * (Random_GetControl() - 0x4000) / 0x7FFF,
        };
        g_Camera.pos.x += shake.x;
        g_Camera.pos.y += shake.y;
        g_Camera.pos.z += shake.z;
        g_Camera.target.y += shake.x;
        g_Camera.target.y += shake.y;
        g_Camera.target.z += shake.z;
        g_Camera.bounce += 5;
    }

    if (g_Camera.pos.y > height) {
        g_Camera.shift = height - g_Camera.pos.y;
    } else if (g_Camera.pos.y < ceiling) {
        g_Camera.shift = ceiling - g_Camera.pos.y;
    } else {
        g_Camera.shift = 0;
    }

    Camera_UpdateMicPosition();
}

static void M_Chase(const ITEM *const item)
{
    g_Camera.target_elevation += item->rot.x;
    g_Camera.target_elevation = MIN(g_Camera.target_elevation, M_MAX_ELEVATION);
    g_Camera.target_elevation =
        MAX(g_Camera.target_elevation, -M_MAX_ELEVATION);

    const int32_t distance =
        (g_Camera.target_distance * Math_Cos(g_Camera.target_elevation))
        >> W2V_SHIFT;
    const int16_t angle = g_Camera.target_angle + item->rot.y;

    g_Camera.target_square = SQUARE(distance);

    const XYZ_32 offset = {
        .y = (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation))
            >> W2V_SHIFT,
        .x = -((distance * Math_Sin(angle)) >> W2V_SHIFT),
        .z = -((distance * Math_Cos(angle)) >> W2V_SHIFT),
    };

    GAME_VECTOR target = {
        .x = g_Camera.target.x + offset.x,
        .y = g_Camera.target.y + offset.y,
        .z = g_Camera.target.z + offset.z,
        .room_num = g_Camera.pos.room_num,
    };

    const M_SETTINGS settings = M_GetSettings();
    const int16_t speed = TR_VERSION == 2 || g_Camera.fixed_camera
        ? g_Camera.speed
        : settings.chase_speed;
    M_SmartShift(&target, M_Shift);
    M_Move(&target, speed);
}

static void M_Combat(const ITEM *const item)
{
    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;
    g_Camera.target_distance = M_COMBAT_DISTANCE;

    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->target != nullptr) {
        g_Camera.target_angle = lara_info->target_angles[0] + item->rot.y;
        g_Camera.target_elevation = lara_info->target_angles[1] + item->rot.x;
    } else {
        g_Camera.target_angle =
            lara_info->torso_rot.y + lara_info->head_rot.y + item->rot.y;
        g_Camera.target_elevation =
            lara_info->torso_rot.x + lara_info->head_rot.x + item->rot.x;
    }

    const int32_t distance =
        (M_COMBAT_DISTANCE * Math_Cos(g_Camera.target_elevation)) >> W2V_SHIFT;

    const XYZ_32 offset = {
        .y =
            +((g_Camera.target_distance * Math_Sin(g_Camera.target_elevation))
              >> W2V_SHIFT),
        .x = -((distance * Math_Sin(g_Camera.target_angle)) >> W2V_SHIFT),
        .z = -((distance * Math_Cos(g_Camera.target_angle)) >> W2V_SHIFT),
    };

    GAME_VECTOR target = {
        .x = g_Camera.target.x + offset.x,
        .y = g_Camera.target.y + offset.y,
        .z = g_Camera.target.z + offset.z,
        .room_num = g_Camera.pos.room_num,
    };

    if (lara_info->water_status == LWS_UNDERWATER) {
        const ITEM *const lara_item = Lara_GetItem();
        const int32_t water_height =
            lara_info->water_surface_dist + lara_item->pos.y;
        if (g_Camera.target.y > water_height && water_height > target.y) {
            target.y = lara_info->water_surface_dist + lara_item->pos.y;
            target.z = g_Camera.target.z
                + (water_height - g_Camera.target.y)
                    * (target.z - g_Camera.target.z)
                    / (target.y - g_Camera.target.y);
            target.x = g_Camera.target.x
                + (water_height - g_Camera.target.y)
                    * (target.x - g_Camera.target.x)
                    / (target.y - g_Camera.target.y);
        }
    }

    M_SmartShift(&target, M_Shift);
    M_Move(&target, g_Camera.speed);
}

static void M_Fixed(void)
{
    const OBJECT_VECTOR *const fixed = Camera_GetFixedObject(g_Camera.num);
    GAME_VECTOR target = {
        .x = fixed->x,
        .y = fixed->y,
        .z = fixed->z,
        .room_num = fixed->data,
    };
#if TR_VERSION >= 2
    if (!LOS_Check(&g_Camera.target, &target)) {
        M_ShiftClamp(&target, STEP_L);
    }
#endif

    g_Camera.fixed_camera = true;
    M_Move(&target, g_Camera.speed);

    if (g_Camera.timer != 0) {
        g_Camera.timer--;
        if (g_Camera.timer == 0) {
            g_Camera.timer = -1;
        }
    }
}

static void M_Look(const ITEM *const item)
{
    const XYZ_32 old = {
        .x = g_Camera.target.x,
        .y = g_Camera.target.y,
        .z = g_Camera.target.z,
    };

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;
    g_Camera.target_distance = M_LOOK_DISTANCE;

    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    g_Camera.target_angle =
        item->rot.y + lara_info->torso_rot.y + lara_info->head_rot.y;
    g_Camera.target_elevation =
        item->rot.x + lara_info->torso_rot.x + lara_info->head_rot.x;

    const int32_t distance =
        (M_LOOK_DISTANCE * Math_Cos(g_Camera.target_elevation)) >> W2V_SHIFT;

    g_Camera.shift =
        (-STEP_L * 2 * Math_Sin(g_Camera.target_elevation)) >> W2V_SHIFT;
    g_Camera.target.z += (g_Camera.shift * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    g_Camera.target.x += (g_Camera.shift * Math_Sin(item->rot.y)) >> W2V_SHIFT;

    if (!M_IsGoodPosition(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            g_Camera.target.room_num)) {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;
    }

    g_Camera.target.y += M_ShiftClamp(&g_Camera.target, M_LOOK_CLAMP);

    const XYZ_32 offset = {
        .y =
            +((g_Camera.target_distance * Math_Sin(g_Camera.target_elevation))
              >> W2V_SHIFT),
        .x = -((distance * Math_Sin(g_Camera.target_angle)) >> W2V_SHIFT),
        .z = -((distance * Math_Cos(g_Camera.target_angle)) >> W2V_SHIFT),
    };

    GAME_VECTOR target = {
        .x = g_Camera.target.x + offset.x,
        .y = g_Camera.target.y + offset.y,
        .z = g_Camera.target.z + offset.z,
        .room_num = g_Camera.pos.room_num,
    };

    M_SmartShift(&target, M_Clip);
    g_Camera.target.z = old.z + (g_Camera.target.z - old.z) / g_Camera.speed;
    g_Camera.target.x = old.x + (g_Camera.target.x - old.x) / g_Camera.speed;
    M_Move(&target, g_Camera.speed);
    g_Camera.debuff = 5;
}

bool Camera_IsChunky(void)
{
    return m_IsChunky;
}

void Camera_SetChunky(const bool is_chunky)
{
    m_IsChunky = is_chunky;
}

void Camera_Initialise(void)
{
    m_IsInitialised = false;
    Matrix_ResetStack();
    g_Camera.last = NO_CAMERA;
    g_Camera.underwater = false;
    Camera_ResetPosition();
#if TR_VERSION == 2
    Viewport_AlterFOV(-1);
#endif
    Camera_Update();
    m_IsInitialised = true;
}

void Camera_ResetPosition(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    ASSERT(lara_item != nullptr);
    g_Camera.shift = lara_item->pos.y - WALL_L;

    g_Camera.target.x = lara_item->pos.x;
    g_Camera.target.y = g_Camera.shift;
    g_Camera.target.z = lara_item->pos.z;
    g_Camera.target.room_num = lara_item->room_num;

    g_Camera.pos.x = g_Camera.target.x;
    g_Camera.pos.y = g_Camera.target.y;
    g_Camera.pos.z = g_Camera.target.z - 100;
    g_Camera.pos.room_num = g_Camera.target.room_num;

    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
    g_Camera.item = nullptr;

    g_Camera.speed = 1;
    g_Camera.flags = CF_NORMAL;
    g_Camera.bounce = 0;
    g_Camera.num = NO_CAMERA;
    g_Camera.fixed_camera = false;
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
#if TR_VERSION == 1
    g_Camera.type = CAM_CHASE;
#else
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (!lara_info->extra_anim) {
        g_Camera.type = CAM_CHASE;
    }
#endif
}

void Camera_Reset(void)
{
    g_Camera.pos.room_num = NO_ROOM;
}

void Camera_ClampInterpResult(void)
{
    if (g_Camera.type == CAM_PHOTO_MODE) {
        Room_GetSector(
            g_Camera.interp.result.pos.x,
            g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
            g_Camera.interp.result.pos.z, &g_Camera.interp.room_num);
        return;
    }

    XYZ_32 *const pos = &g_Camera.interp.result.pos;
    const int32_t shift = g_Camera.interp.result.shift;
    const ROOM *const room = Room_Get(g_Camera.interp.room_num);
    const SECTOR *sector = Room_GetWorldSector(room, pos->x, pos->z);
    if (sector->box != NO_BOX) {
        goto finish;
    }

    sector = Room_GetWorldSector(room, g_Camera.pos.x, g_Camera.pos.z);
    if (sector->box == NO_BOX) {
        goto finish;
    }

    const BOX_INFO *const box = Box_GetBox(sector->box);
    CLAMP(pos->x, box->top, box->bottom);
    CLAMP(pos->z, box->left, box->right);

finish:
    const int32_t floor =
        Room_GetHeightEx(sector, pos->x, pos->y, pos->z, true, NO_ITEM);
    const int32_t ceiling =
        Room_GetCeilingEx(sector, pos->x, pos->y, pos->z, true);
    if (floor != NO_HEIGHT && ceiling != NO_HEIGHT) {
        CLAMP(pos->y, ceiling - shift, floor - shift);
    }
    Room_GetSector(pos->x, pos->y + shift, pos->z, &g_Camera.interp.room_num);
}

void Camera_RefreshFromTrigger(const TRIGGER *const trigger)
{
    int16_t target_ok = 2;
#if TR_VERSION == 1
    const bool fix_glide_cameras = false;
#else
    const bool fix_glide_cameras = g_Config.visuals.fix_glide_cameras;
#endif

    const TRIGGER_CMD *cmd = trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type == TO_CAMERA) {
            const TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (cam_data->camera_num == g_Camera.last) {
                g_Camera.num = cam_data->camera_num;

                if (g_Camera.timer < 0 || g_Camera.type == CAM_LOOK
                    || g_Camera.type == CAM_COMBAT) {
                    g_Camera.timer = -1;
                    target_ok = 0;
                } else {
                    g_Camera.type = CAM_FIXED;
                    if (fix_glide_cameras && cam_data->glide != 0) {
                        g_Camera.speed = cam_data->glide + 1;
                    }
                    target_ok = 1;
                }
            } else {
                target_ok = 0;
            }
        } else if (cmd->type == TO_TARGET) {
            if (g_Camera.type != CAM_LOOK && g_Camera.type != CAM_COMBAT) {
                g_Camera.item = Item_Get((int16_t)(intptr_t)cmd->parameter);
            }
        }
    }

    if (g_Camera.item != nullptr
        && (target_ok == 0
            || (target_ok == 2 && g_Camera.item->looked_at
                && g_Camera.item != g_Camera.last_item))) {
        g_Camera.item = nullptr;
    }
#if TR_VERSION >= 2
    // TODO: check if this can be removed from TR2 during camera module merge.
    // It is required not to be present in TR1 otherwise heavy triggers (that
    // don't have camera data) can cancel active cameras triggered by Lara.
    if (g_Camera.num == -1 && g_Camera.timer > 0) {
        g_Camera.timer = -1;
    }
#endif
}

void Camera_EnsureEnvironment(void)
{
    if (g_Camera.pos.room_num == NO_ROOM) {
        return;
    }

    const ROOM *const room = Room_Get(g_Camera.pos.room_num);
    if ((room->flags & RF_UNDERWATER) != 0) {
        M_AdjustMusicVolume(true);
        Sound_Effect(SFX_UNDERWATER, nullptr, SPM_ALWAYS);
        g_Camera.underwater = true;
    } else {
        M_AdjustMusicVolume(false);
        if (g_Camera.underwater) {
            Sound_StopEffect(SFX_UNDERWATER);
            g_Camera.underwater = false;
        }
    }
}

void Camera_Update(void)
{
    if (g_Camera.type == CAM_PHOTO_MODE) {
        Camera_PhotoMode_Update();
        Camera_EnsureEnvironment();
        return;
    }

    if (g_Camera.type == CAM_CINEMATIC) {
        Camera_LoadCutsceneFrame();
        Camera_EnsureEnvironment();
        return;
    }

    if (g_Camera.flags != CF_NO_CHUNKY) {
        Camera_SetChunky(true);
    }

    const bool fixed_camera = g_Camera.item != nullptr
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY);
    const ITEM *const item = fixed_camera ? g_Camera.item : Lara_GetItem();

    const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);

    int32_t y = item->pos.y;
    if (fixed_camera) {
        y += (bounds->min.y + bounds->max.y) / 2;
    } else {
        y += bounds->max.y
            + (((int32_t)(bounds->min.y - bounds->max.y)) * 3 >> 2);
    }

    if (g_Camera.item != nullptr && !fixed_camera) {
        bounds = Item_GetBoundsAccurate(g_Camera.item);

        const int32_t dx = g_Camera.item->pos.x - item->pos.x;
        const int32_t dz = g_Camera.item->pos.z - item->pos.z;
        const int32_t shift = Math_Sqrt(SQUARE(dx) + SQUARE(dz));
        int16_t angle = Math_Atan(dz, dx) - item->rot.y;

        int16_t tilt = Math_Atan(
            shift,
            y - (bounds->min.y + bounds->max.y) / 2 - g_Camera.item->pos.y);
        angle >>= 1;
        tilt >>= 1;

        if (angle > M_MIN_HEAD_ROTATION && angle < M_MAX_HEAD_ROTATION
            && tilt > M_MIN_HEAD_TILT && tilt < M_MAX_HEAD_TILT) {
            LARA_INFO *const lara_info = Lara_GetLaraInfo();
            int16_t change = angle - lara_info->head_rot.y;
            if (change > M_HEAD_TURN) {
                lara_info->head_rot.y += M_HEAD_TURN;
            } else if (change < -M_HEAD_TURN) {
                lara_info->head_rot.y -= M_HEAD_TURN;
            } else {
                lara_info->head_rot.y = angle;
            }

            change = tilt - lara_info->head_rot.x;
            if (change > M_HEAD_TURN) {
                lara_info->head_rot.x += M_HEAD_TURN;
            } else if (change < -M_HEAD_TURN) {
                lara_info->head_rot.x -= M_HEAD_TURN;
            } else {
                lara_info->head_rot.x += change;
            }
            lara_info->torso_rot.x = lara_info->head_rot.x;
            lara_info->torso_rot.y = lara_info->head_rot.y;
            g_Camera.type = CAM_LOOK;
            g_Camera.item->looked_at = true;
        }
    }

    if (g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT) {
        y -= STEP_L;
        g_Camera.target.room_num = item->room_num;
        if (g_Camera.fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.speed = 1;
        } else {
            g_Camera.target.y += (y - g_Camera.target.y) >> 2;
            g_Camera.speed =
                g_Camera.type == CAM_LOOK ? M_LOOK_SPEED : M_COMBAT_SPEED;
        }
        g_Camera.fixed_camera = false;
        if (g_Camera.type == CAM_LOOK) {
            M_Look(item);
        } else {
            M_Combat(item);
        }
    } else {
        if (fixed_camera) {
            g_Camera.debuff = 0;
        }
        if (g_Camera.debuff > 0) {
            const XYZ_32 old = g_Camera.target.pos;
            g_Camera.target.x = (item->pos.x + old.x) / 2;
            g_Camera.target.z = (item->pos.z + old.z) / 2;
            g_Camera.debuff--;
        } else {
            g_Camera.target.x = item->pos.x;
            g_Camera.target.z = item->pos.z;
        }

        if (g_Camera.flags == CF_FOLLOW_CENTRE) {
            const int32_t shift = (bounds->min.z + bounds->max.z) / 2;
            g_Camera.target.z += (shift * Math_Cos(item->rot.y)) >> W2V_SHIFT;
            g_Camera.target.x += (shift * Math_Sin(item->rot.y)) >> W2V_SHIFT;
        }

        g_Camera.target.room_num = item->room_num;
        if (g_Camera.fixed_camera != fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.fixed_camera = true;
            g_Camera.speed = 1;
        } else {
            g_Camera.fixed_camera = false;
            g_Camera.target.y += (y - g_Camera.target.y) / 4;
        }

        const SECTOR *const sector = Room_GetSector(
            g_Camera.target.x, y, g_Camera.target.z, &g_Camera.target.room_num);
        const int32_t height = Room_GetHeight(
            sector, g_Camera.target.x, g_Camera.target.y, g_Camera.target.z);
        if (g_Camera.target.y > height) {
            Camera_SetChunky(false);
        }

        if (g_Camera.type == CAM_CHASE || g_Camera.flags == CF_CHASE_OBJECT) {
            M_Chase(item);
        } else {
            M_Fixed();
        }
    }

    g_Camera.last = g_Camera.num;
    g_Camera.fixed_camera = fixed_camera;

    switch (g_Camera.type) {
    case CAM_LOOK:
    case CAM_CINEMATIC:
    case CAM_COMBAT:
    case CAM_FIXED:
        g_Camera.additional_angle = 0;
        g_Camera.additional_elevation = 0;
        break;

    default:
        break;
    }

    if (g_Camera.type != CAM_HEAVY || g_Camera.timer == -1) {
        g_Camera.type = CAM_CHASE;
        g_Camera.num = NO_CAMERA;
        g_Camera.last_item = g_Camera.item;
        g_Camera.item = nullptr;
        g_Camera.target_angle = g_Camera.additional_angle;
        g_Camera.target_elevation = g_Camera.additional_elevation;
        g_Camera.target_distance = M_CHASE_ELEVATION;
        g_Camera.flags = CF_NORMAL;
#if TR_VERSION >= 2
        const M_SETTINGS settings = M_GetSettings();
        g_Camera.speed = settings.chase_speed;
#endif
    }
    Camera_SetChunky(false);
    if (m_IsInitialised) {
        Camera_EnsureEnvironment();
    }
}

void Camera_UpdateMicPosition(void)
{
#if TR_VERSION == 2
    if (g_Config.audio.enable_lara_mic) {
        const LARA_INFO *const lara_info = Lara_GetLaraInfo();
        const ITEM *const lara_item = Lara_GetItem();
        g_Camera.actual_angle =
            lara_info->torso_rot.y + lara_info->head_rot.y + lara_item->rot.y;
        g_Camera.mic_pos = lara_item->pos;
    } else {
        g_Camera.actual_angle = Math_Atan(
            g_Camera.target.z - g_Camera.pos.z,
            g_Camera.target.x - g_Camera.pos.x);
        g_Camera.mic_pos.x = g_Camera.pos.x
            + ((g_PhdPersp * Math_Sin(g_Camera.actual_angle)) >> W2V_SHIFT);
        g_Camera.mic_pos.z = g_Camera.pos.z
            + ((g_PhdPersp * Math_Cos(g_Camera.actual_angle)) >> W2V_SHIFT);
        g_Camera.mic_pos.y = g_Camera.pos.y;
    }
#endif
}

void Camera_MoveManual(void)
{
    const int16_t camera_delta = (const int32_t)(DEG_90 / LOGIC_FPS)
        * (double)m_ManualCameraMultiplier[g_Config.gameplay.camera_speed];

    if (g_Input.camera_left) {
        M_OffsetAdditionalAngle(camera_delta);
    } else if (g_Input.camera_right) {
        M_OffsetAdditionalAngle(-camera_delta);
    }
    if (g_Input.camera_forward) {
        M_OffsetAdditionalElevation(-camera_delta);
    } else if (g_Input.camera_back) {
        M_OffsetAdditionalElevation(camera_delta);
    }
    if (g_Input.camera_reset) {
        M_OffsetReset();
    }
}

void Camera_Apply(void)
{
    Matrix_LookAt(
        g_Camera.interp.result.pos.x,
        g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
        g_Camera.interp.result.pos.z, g_Camera.interp.result.target.x,
        g_Camera.interp.result.target.y, g_Camera.interp.result.target.z,
        g_Camera.roll);
}
