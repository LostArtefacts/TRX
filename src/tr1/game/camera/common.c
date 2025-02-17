#include "game/camera/common.h"

#include "game/input.h"
#include "game/items.h"
#include "game/los.h"
#include "game/music.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>

// Camera speed option ranges from 1-10, so index 0 is unused.
static double m_ManualCameraMultiplier[11] = {
    1.0, .5, .625, .75, .875, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0,
};

static void M_Chase(const ITEM *item);
static void M_Combat(const ITEM *item);
static void M_Look(const ITEM *item);
static void M_Fixed(void);
static void M_LoadCutsceneFrame(void);

static void M_OffsetAdditionalAngle(int16_t delta);
static void M_OffsetAdditionalElevation(int16_t delta);
static void M_OffsetReset(void);

static void M_AdjustMusicVolume(bool underwater);
static void M_EnsureEnvironment(void);

static void M_Move(const GAME_VECTOR *ideal, int32_t speed);
static bool M_BadPosition(int32_t x, int32_t y, int32_t z, int16_t room_num);
static int32_t M_ShiftClamp(GAME_VECTOR *pos, int32_t clamp);
static void M_SmartShift(
    GAME_VECTOR *ideal,
    void (*shift)(
        int32_t *x, int32_t *y, int32_t target_x, int32_t target_y,
        int32_t left, int32_t top, int32_t right, int32_t bottom));
static void M_Clip(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom);
static void M_Shift(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom);

static void M_Chase(const ITEM *const item)
{
    GAME_VECTOR ideal;

    g_Camera.target_elevation += item->rot.x;
    if (g_Camera.target_elevation > MAX_ELEVATION) {
        g_Camera.target_elevation = MAX_ELEVATION;
    } else if (g_Camera.target_elevation < -MAX_ELEVATION) {
        g_Camera.target_elevation = -MAX_ELEVATION;
    }

    const int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);

    g_Camera.target_square = SQUARE(distance);

    const PHD_ANGLE angle = item->rot.y + g_Camera.target_angle;
    ideal.x = g_Camera.target.x - (distance * Math_Sin(angle) >> W2V_SHIFT);
    ideal.z = g_Camera.target.z - (distance * Math_Cos(angle) >> W2V_SHIFT);
    ideal.room_num = g_Camera.pos.room_num;

    M_SmartShift(&ideal, M_Shift);

    if (g_Camera.fixed_camera) {
        M_Move(&ideal, g_Camera.speed);
    } else {
        M_Move(&ideal, CHASE_SPEED);
    }
}

static void M_Combat(const ITEM *const item)
{
    GAME_VECTOR ideal;

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    if (g_Lara.target) {
        g_Camera.target_angle = item->rot.y + g_Lara.target_angles[0];
        g_Camera.target_elevation = item->rot.x + g_Lara.target_angles[1];
    } else {
        g_Camera.target_angle =
            item->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
        g_Camera.target_elevation =
            item->rot.x + g_Lara.torso_rot.x + g_Lara.head_rot.x;
    }

    g_Camera.target_distance = COMBAT_DISTANCE;

    const int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    ideal.x = g_Camera.target.x
        - (distance * Math_Sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * Math_Cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_num = g_Camera.pos.room_num;

    M_SmartShift(&ideal, M_Shift);
    M_Move(&ideal, g_Camera.speed);
}

static void M_Look(const ITEM *const item)
{
    const GAME_VECTOR old = {
        .x = g_Camera.target.x,
        .z = g_Camera.target.z,
    };

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    g_Camera.target_angle =
        item->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
    g_Camera.target_elevation =
        item->rot.x + g_Lara.torso_rot.x + g_Lara.head_rot.x;
    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;

    const int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    g_Camera.shift =
        -STEP_L * 2 * Math_Sin(g_Camera.target_elevation) >> W2V_SHIFT;
    g_Camera.target.z += g_Camera.shift * Math_Cos(item->rot.y) >> W2V_SHIFT;
    g_Camera.target.x += g_Camera.shift * Math_Sin(item->rot.y) >> W2V_SHIFT;

    if (M_BadPosition(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            g_Camera.target.room_num)) {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;
    }

    g_Camera.target.y += M_ShiftClamp(&g_Camera.target, STEP_L + 50);

    GAME_VECTOR ideal;
    ideal.x = g_Camera.target.x
        - (distance * Math_Sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * Math_Cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_num = g_Camera.pos.room_num;

    M_SmartShift(&ideal, M_Clip);

    g_Camera.target.z = old.z + (g_Camera.target.z - old.z) / g_Camera.speed;
    g_Camera.target.x = old.x + (g_Camera.target.x - old.x) / g_Camera.speed;

    M_Move(&ideal, g_Camera.speed);
    g_Camera.debuff = 5;
}

static void M_Fixed(void)
{
    const OBJECT_VECTOR *const fixed = Camera_GetFixedObject(g_Camera.num);
    GAME_VECTOR ideal = {
        .x = fixed->x,
        .y = fixed->y,
        .z = fixed->z,
        .room_num = fixed->data,
    };

    g_Camera.fixed_camera = true;

    M_Move(&ideal, g_Camera.speed);

    if (g_Camera.timer) {
        g_Camera.timer--;
        if (!g_Camera.timer) {
            g_Camera.timer = -1;
        }
    }
}

static void M_LoadCutsceneFrame(void)
{
    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->frame_idx++;
    if (cine_data->frame_idx >= cine_data->frame_count) {
        cine_data->frame_idx = cine_data->frame_count - 1;
    }

    Camera_UpdateCutscene();
}

static void M_OffsetAdditionalAngle(const int16_t delta)
{
    g_Camera.additional_angle += delta;
}

static void M_OffsetAdditionalElevation(const int16_t delta)
{
    // don't let this value wrap, so clamp it.
    if (delta > 0) {
        if (g_Camera.additional_elevation > INT16_MAX - delta) {
            g_Camera.additional_elevation = INT16_MAX;
        } else {
            g_Camera.additional_elevation += delta;
        }
    } else {
        if (g_Camera.additional_elevation < INT16_MIN - delta) {
            g_Camera.additional_elevation = INT16_MIN;
        } else {
            g_Camera.additional_elevation += delta;
        }
    }
}

static void M_OffsetReset(void)
{
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
}

static void M_AdjustMusicVolume(const bool underwater)
{
    const bool is_ambient =
        Music_GetCurrentPlayingTrack() == Music_GetCurrentLoopedTrack();

    double multiplier = 1.0;

    if (underwater) {
        switch (g_Config.audio.underwater_music_mode) {
        case UMM_FULL:
            multiplier = 1.0;
            break;
        case UMM_QUIET:
            multiplier = 0.5;
            break;
        case UMM_NONE:
            multiplier = 0.0;
            break;
        case UMM_FULL_NO_AMBIENT:
            multiplier = is_ambient ? 0.0 : 1.0;
            break;
        case UMM_QUIET_NO_AMBIENT:
            multiplier = is_ambient ? 0.0 : 0.5;
            break;
        default:
            multiplier = 1.0;
            break;
        }
    }

    Music_SetVolume(g_Config.audio.music_volume * multiplier);
}

static void M_EnsureEnvironment(void)
{
    if (g_Camera.pos.room_num == NO_ROOM) {
        return;
    }

    if (Room_Get(g_Camera.pos.room_num)->flags & RF_UNDERWATER) {
        M_AdjustMusicVolume(true);
        Sound_Effect(SFX_UNDERWATER, nullptr, SPM_ALWAYS);
        g_Camera.underwater = true;
    } else {
        M_AdjustMusicVolume(false);
        if (g_Camera.underwater) {
            Sound_StopEffect(SFX_UNDERWATER, nullptr);
            g_Camera.underwater = false;
        }
    }
}

static void M_Move(const GAME_VECTOR *const ideal, const int32_t speed)
{
    g_Camera.pos.x += (ideal->x - g_Camera.pos.x) / speed;
    g_Camera.pos.z += (ideal->z - g_Camera.pos.z) / speed;
    g_Camera.pos.y += (ideal->y - g_Camera.pos.y) / speed;
    g_Camera.pos.room_num = ideal->room_num;

    Camera_SetChunky(false);

    const SECTOR *sector = Room_GetSector(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z, &g_Camera.pos.room_num);
    int32_t height =
        Room_GetHeight(sector, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
        - GROUND_SHIFT;

    if (g_Camera.pos.y >= height && ideal->y >= height) {
        LOS_Check(&g_Camera.target, &g_Camera.pos);
        sector = Room_GetSector(
            g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z,
            &g_Camera.pos.room_num);
        height = Room_GetHeight(
                     sector, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
            - GROUND_SHIFT;
    }

    int32_t ceiling =
        Room_GetCeiling(sector, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
        + GROUND_SHIFT;
    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (g_Camera.bounce) {
        if (g_Camera.bounce > 0) {
            g_Camera.pos.y += g_Camera.bounce;
            g_Camera.target.y += g_Camera.bounce;
            g_Camera.bounce = 0;
        } else {
            int32_t shake;
            shake = (Random_GetControl() - 0x4000) * g_Camera.bounce / 0x7FFF;
            g_Camera.pos.x += shake;
            g_Camera.target.y += shake;
            shake = (Random_GetControl() - 0x4000) * g_Camera.bounce / 0x7FFF;
            g_Camera.pos.y += shake;
            g_Camera.target.y += shake;
            shake = (Random_GetControl() - 0x4000) * g_Camera.bounce / 0x7FFF;
            g_Camera.pos.z += shake;
            g_Camera.target.z += shake;
            g_Camera.bounce += 5;
        }
    }

    if (g_Camera.pos.y > height) {
        g_Camera.shift = height - g_Camera.pos.y;
    } else if (g_Camera.pos.y < ceiling) {
        g_Camera.shift = ceiling - g_Camera.pos.y;
    } else {
        g_Camera.shift = 0;
    }
}

static bool M_BadPosition(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    return y >= Room_GetHeight(sector, x, y, z)
        || y <= Room_GetCeiling(sector, x, y, z);
}

static int32_t M_ShiftClamp(GAME_VECTOR *const pos, const int32_t clamp)
{
    const int32_t x = pos->x;
    const int32_t y = pos->y;
    const int32_t z = pos->z;

    const SECTOR *const sector = Room_GetSector(x, y, z, &pos->room_num);

    const BOX_INFO *const box = Box_GetBox(sector->box);
    if (z < box->left + clamp
        && M_BadPosition(x, y, z - clamp, pos->room_num)) {
        pos->z = box->left + clamp;
    } else if (
        z > box->right - clamp
        && M_BadPosition(x, y, z + clamp, pos->room_num)) {
        pos->z = box->right - clamp;
    }

    if (x < box->top + clamp && M_BadPosition(x - clamp, y, z, pos->room_num)) {
        pos->x = box->top + clamp;
    } else if (
        x > box->bottom - clamp
        && M_BadPosition(x + clamp, y, z, pos->room_num)) {
        pos->x = box->bottom - clamp;
    }

    int32_t height = Room_GetHeight(sector, x, y, z) - clamp;
    int32_t ceiling = Room_GetCeiling(sector, x, y, z) + clamp;

    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (y > height) {
        return height - y;
    } else if (pos->y < ceiling) {
        return ceiling - y;
    }

    return 0;
}

static void M_SmartShift(
    GAME_VECTOR *const ideal,
    void (*shift)(
        int32_t *const x, int32_t *const y, const int32_t target_x,
        const int32_t target_y, const int32_t left, const int32_t top,
        const int32_t right, const int32_t bottom))
{
    LOS_Check(&g_Camera.target, ideal);

    const ROOM *room = Room_Get(g_Camera.target.room_num);
    int32_t z_sector = (g_Camera.target.z - room->pos.z) >> WALL_SHIFT;
    int32_t x_sector = (g_Camera.target.x - room->pos.x) >> WALL_SHIFT;

    const int16_t item_box = Room_GetUnitSector(room, x_sector, z_sector)->box;
    const BOX_INFO *box = Box_GetBox(item_box);

    room = Room_Get(ideal->room_num);
    z_sector = (ideal->z - room->pos.z) >> WALL_SHIFT;
    x_sector = (ideal->x - room->pos.x) >> WALL_SHIFT;

    int16_t camera_box = Room_GetUnitSector(room, x_sector, z_sector)->box;
    if (camera_box != NO_BOX
        && (ideal->z < box->left || ideal->z > box->right || ideal->x < box->top
            || ideal->x > box->bottom)) {
        box = Box_GetBox(camera_box);
    }

    int32_t left = box->left;
    int32_t right = box->right;
    int32_t top = box->top;
    int32_t bottom = box->bottom;

    int32_t test = (ideal->z - WALL_L) | (WALL_L - 1);
    const bool bad_left =
        M_BadPosition(ideal->x, ideal->y, test, ideal->room_num);
    if (!bad_left) {
        camera_box = Room_GetUnitSector(room, x_sector, z_sector - 1)->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->left < left) {
            left = Box_GetBox(camera_box)->left;
        }
    }

    test = (ideal->z + WALL_L) & (~(WALL_L - 1));
    const bool bad_right =
        M_BadPosition(ideal->x, ideal->y, test, ideal->room_num);
    if (!bad_right) {
        camera_box = Room_GetUnitSector(room, x_sector, z_sector + 1)->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->right > right) {
            right = Box_GetBox(camera_box)->right;
        }
    }

    test = (ideal->x - WALL_L) | (WALL_L - 1);
    const bool bad_top =
        M_BadPosition(test, ideal->y, ideal->z, ideal->room_num);
    if (!bad_top) {
        camera_box = Room_GetUnitSector(room, x_sector - 1, z_sector)->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->top < top) {
            top = Box_GetBox(camera_box)->top;
        }
    }

    test = (ideal->x + WALL_L) & (~(WALL_L - 1));
    const bool bad_bottom =
        M_BadPosition(test, ideal->y, ideal->z, ideal->room_num);
    if (!bad_bottom) {
        camera_box = Room_GetUnitSector(room, x_sector + 1, z_sector)->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->bottom > bottom) {
            bottom = Box_GetBox(camera_box)->bottom;
        }
    }

    left += STEP_L;
    right -= STEP_L;
    top += STEP_L;
    bottom -= STEP_L;

    int32_t noclip = 1;
    if (ideal->z < left && bad_left) {
        noclip = 0;
        if (ideal->x < g_Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                left, top, right, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                left, bottom, right, top);
        }
    } else if (ideal->z > right && bad_right) {
        noclip = 0;
        if (ideal->x < g_Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                right, top, left, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                right, bottom, left, top);
        }
    }

    if (noclip) {
        if (ideal->x < top && bad_top) {
            noclip = 0;
            if (ideal->z < g_Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    top, left, bottom, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    top, right, bottom, left);
            }
        } else if (ideal->x > bottom && bad_bottom) {
            noclip = 0;
            if (ideal->z < g_Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    bottom, left, top, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    bottom, right, top, left);
            }
        }
    }

    if (!noclip) {
        Room_GetSector(ideal->x, ideal->y, ideal->z, &ideal->room_num);
    }
}

static void M_Clip(
    int32_t *const x, int32_t *const y, const int32_t target_x,
    const int32_t target_y, const int32_t left, const int32_t top,
    const int32_t right, const int32_t bottom)
{
    const int32_t x_diff = *x - target_x;
    const int32_t y_diff = *y - target_y;

    if ((right > left) != (target_x < left)) {
        if (x_diff) {
            *y = target_y + (left - target_x) * y_diff / x_diff;
        }
        *x = left;
    }

    if ((bottom > top && target_y > top && *y < top)
        || (bottom < top && target_y < top && (*y) > top)) {
        if (y_diff) {
            *x = target_x + (top - target_y) * x_diff / y_diff;
        }
        *y = top;
    }
}

static void M_Shift(
    int32_t *const x, int32_t *const y, const int32_t target_x,
    const int32_t target_y, const int32_t left, const int32_t top,
    const int32_t right, const int32_t bottom)
{
    const int32_t tl_square = SQUARE(target_x - left) + SQUARE(target_y - top);
    const int32_t bl_square =
        SQUARE(target_x - left) + SQUARE(target_y - bottom);
    const int32_t tr_square = SQUARE(target_x - right) + SQUARE(target_y - top);

    int32_t shift;
    if (g_Camera.target_square < tl_square) {
        *x = left;
        shift = g_Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = Math_Sqrt(shift);
        *y = target_y + ((top < bottom) ? -shift : shift);
    } else if (tl_square > MIN_SQUARE) {
        *x = left;
        *y = top;
    } else if (g_Camera.target_square < bl_square) {
        *x = left;
        shift = g_Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = Math_Sqrt(shift);
        *y = target_y + ((top < bottom) ? shift : -shift);
    } else if (bl_square > MIN_SQUARE) {
        *x = left;
        *y = bottom;
    } else if (g_Camera.target_square < tr_square) {
        shift = g_Camera.target_square - SQUARE(target_y - top);
        if (shift < 0) {
            return;
        }

        shift = Math_Sqrt(shift);
        *x = target_x + ((left < right) ? shift : -shift);
        *y = top;
    } else {
        *x = right;
        *y = top;
    }
}

void Camera_Initialise(void)
{
    Matrix_ResetStack();
    Camera_ResetPosition();
    Camera_Update();
}

void Camera_Reset(void)
{
    g_Camera.pos.room_num = NO_ROOM;
}

void Camera_ResetPosition(void)
{
    ASSERT(g_LaraItem != nullptr);
    g_Camera.shift = g_LaraItem->pos.y - WALL_L;

    g_Camera.target.x = g_LaraItem->pos.x;
    g_Camera.target.y = g_Camera.shift;
    g_Camera.target.z = g_LaraItem->pos.z;
    g_Camera.target.room_num = g_LaraItem->room_num;

    g_Camera.pos.x = g_Camera.target.x;
    g_Camera.pos.y = g_Camera.target.y;
    g_Camera.pos.z = g_Camera.target.z - 100;
    g_Camera.pos.room_num = g_Camera.target.room_num;

    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
    g_Camera.item = nullptr;

    g_Camera.type = CAM_CHASE;
    g_Camera.flags = CF_NORMAL;
    g_Camera.bounce = 0;
    g_Camera.num = NO_CAMERA;
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
}

void Camera_Update(void)
{
    if (g_Camera.type == CAM_PHOTO_MODE) {
        Camera_UpdatePhotoMode();
        return;
    }

    if (g_Camera.type == CAM_CINEMATIC) {
        M_LoadCutsceneFrame();
        return;
    }

    if (g_Camera.flags != CF_NO_CHUNKY) {
        Camera_SetChunky(true);
    }

    const bool fixed_camera = g_Camera.item
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY);
    const ITEM *const item = fixed_camera ? g_Camera.item : g_LaraItem;

    const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);

    int32_t y = item->pos.y;
    if (!fixed_camera) {
        y += bounds->max.y + ((bounds->min.y - bounds->max.y) * 3 >> 2);
    } else {
        y += (bounds->min.y + bounds->max.y) / 2;
    }

    if (g_Camera.item && !fixed_camera) {
        bounds = Item_GetBoundsAccurate(g_Camera.item);
        const int16_t shift = Math_Sqrt(
            SQUARE(g_Camera.item->pos.z - item->pos.z)
            + SQUARE(g_Camera.item->pos.x - item->pos.x));
        int16_t angle = Math_Atan(
                            g_Camera.item->pos.z - item->pos.z,
                            g_Camera.item->pos.x - item->pos.x)
            - item->rot.y;
        int16_t tilt = Math_Atan(
            shift,
            y - (g_Camera.item->pos.y + (bounds->min.y + bounds->max.y) / 2));
        angle >>= 1;
        tilt >>= 1;

        if (angle > -MAX_HEAD_ROTATION && angle < MAX_HEAD_ROTATION
            && tilt > MIN_HEAD_TILT_CAM && tilt < MAX_HEAD_TILT_CAM) {
            int16_t change = angle - g_Lara.head_rot.y;
            if (change > HEAD_TURN) {
                g_Lara.head_rot.y += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                g_Lara.head_rot.y -= HEAD_TURN;
            } else {
                g_Lara.head_rot.y += change;
            }

            change = tilt - g_Lara.head_rot.x;
            if (change > HEAD_TURN) {
                g_Lara.head_rot.x += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                g_Lara.head_rot.x -= HEAD_TURN;
            } else {
                g_Lara.head_rot.x += change;
            }

            g_Lara.torso_rot.y = g_Lara.head_rot.y;
            g_Lara.torso_rot.x = g_Lara.head_rot.x;

            g_Camera.type = CAM_LOOK;
            g_Camera.item->looked_at = 1;
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
                g_Camera.type == CAM_LOOK ? LOOK_SPEED : COMBAT_SPEED;
        }

        g_Camera.fixed_camera = false;

        if (g_Camera.type == CAM_LOOK) {
            M_Look(item);
        } else {
            M_Combat(item);
        }
    } else {
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
            const int16_t shift = (bounds->min.z + bounds->max.z) / 2;
            g_Camera.target.z += Math_Cos(item->rot.y) * shift >> W2V_SHIFT;
            g_Camera.target.x += Math_Sin(item->rot.y) * shift >> W2V_SHIFT;
        }

        g_Camera.target.room_num = item->room_num;

        if (g_Camera.fixed_camera != fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.fixed_camera = true;
            g_Camera.speed = 1;
        } else {
            g_Camera.target.y += (y - g_Camera.target.y) / 4;
            g_Camera.fixed_camera = false;
        }

        const SECTOR *const sector = Room_GetSector(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            &g_Camera.target.room_num);
        if (g_Camera.target.y > Room_GetHeight(
                sector, g_Camera.target.x, g_Camera.target.y,
                g_Camera.target.z)) {
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

    // should we clear the manual camera
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
        g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
        g_Camera.flags = CF_NORMAL;
    }

    Camera_SetChunky(false);
}

void Camera_UpdateCutscene(void)
{
    const CINE_DATA *const cine_data = Camera_GetCineData();
    if (cine_data->frame_count == 0) {
        return;
    }

    const CINE_FRAME *const ref = Camera_GetCurrentCineFrame();
    const int32_t c = Math_Cos(cine_data->position.rot.y);
    const int32_t s = Math_Sin(cine_data->position.rot.y);
    const XYZ_32 *const pos = &cine_data->position.pos;
    g_Camera.target.x = pos->x + ((c * ref->tx + s * ref->tz) >> W2V_SHIFT);
    g_Camera.target.y = pos->y + ref->ty;
    g_Camera.target.z = pos->z + ((c * ref->tz - s * ref->tx) >> W2V_SHIFT);
    g_Camera.pos.x = pos->x + ((s * ref->cz + c * ref->cx) >> W2V_SHIFT);
    g_Camera.pos.y = pos->y + ref->cy;
    g_Camera.pos.z = pos->z + ((c * ref->cz - s * ref->cx) >> W2V_SHIFT);
    g_Camera.roll = ref->roll;
    g_Camera.shift = 0;

    Viewport_SetFOV(ref->fov);
}

void Camera_RefreshFromTrigger(const TRIGGER *const trigger)
{
    int16_t target_ok = 2;
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

    if (g_Camera.item != nullptr) {
        if (!target_ok
            || (target_ok == 2 && g_Camera.item->looked_at
                && g_Camera.item != g_Camera.last_item)) {
            g_Camera.item = nullptr;
        }
    }
}

void Camera_MoveManual(void)
{
    const int16_t camera_delta = (const int)DEG_90 / (const int)LOGIC_FPS
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
    M_EnsureEnvironment();
    Matrix_LookAt(
        g_Camera.interp.result.pos.x,
        g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
        g_Camera.interp.result.pos.z, g_Camera.interp.result.target.x,
        g_Camera.interp.result.target.y, g_Camera.interp.result.target.z,
        g_Camera.roll);
}
