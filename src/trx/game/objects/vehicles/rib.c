#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/camera.h>
#include <trx/game/fx.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vehicles/common.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_RADIUS        500
#define M_COLL_DIST     SQUARE(M_RADIUS * 2) // = 1000000
#define M_FRONT         750
#define M_SIDE          300
#define M_TIP           (M_FRONT + 250)      // = 1000
#define M_TURN          (DEG_1 / 8)          // = 22
#define M_UNDO_TURN     (DEG_1 / 4)          // = 45
#define M_MAX_TURN      (DEG_1 * 4)          // = 728
#define M_MIN_SPEED     20
#define M_MAX_SPEED     110
#define M_FAST_SPEED    185
#define M_SLOW_SPEED    36
#define M_ACCELERATION  5
#define M_REVERSE_SPEED 2
#define M_SHIFT_Y       (-5)
#define M_CAM_ELEVATION (DEG_1 * -20)        // = -3640
#define M_CAM_DISTANCE  (WALL_L * 2)         // = 2048
// clang-format on

typedef enum {
    M_STATE_MOUNT,
    M_STATE_STILL,
    M_STATE_MOVING,
    M_STATE_JUMP_R,
    M_STATE_JUMP_L,
    M_STATE_HIT,
    M_STATE_FALL,
    M_STATE_TURN_R,
    M_STATE_DEATH,
    M_STATE_TURN_L,
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_MOUNT_LEFT  = 0,
    M_ANIM_MOUNT_START = 1,
    M_ANIM_MOUNT_JUMP  = 6,
    M_ANIM_MOUNT_RIGHT = 8,
    M_ANIM_FALL        = 15,
    M_ANIM_DEATH       = 18,
    // clang-format on
} M_ANIM;

typedef enum {
    M_MOUNT_NONE,
    M_MOUNT_LEFT,
    M_MOUNT_RIGHT,
    M_MOUNT_JUMP,
    M_MOUNT_START,
} M_MOUNT_TYPE;

typedef struct {
    int32_t boat_turn;
    int32_t left_fallspeed;
    int32_t right_fallspeed;
    int16_t tilt_angle;
    int16_t extra_rotation;
    int32_t water;
    int32_t pitch;
    int16_t propeller_roll;
} M_PRIV;

static const BITE m_Propeller = {
    .pos = { .x = 0, .y = 0, .z = -80 },
    .mesh_num = 2,
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "boat_turn", &p->boat_turn));
    MUST(JSON_READ_OPT(io, "left_fallspeed", &p->left_fallspeed));
    MUST(JSON_READ_OPT(io, "right_fallspeed", &p->right_fallspeed));
    MUST(JSON_READ_OPT(io, "tilt_angle", &p->tilt_angle));
    MUST(JSON_READ_OPT(io, "extra_rotation", &p->extra_rotation));
    MUST(JSON_READ_OPT(io, "water", &p->water));
    MUST(JSON_READ_OPT(io, "pitch", &p->pitch));
    MUST(JSON_READ_OPT(io, "propeller_roll", &p->propeller_roll));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "boat_turn", p->boat_turn);
    JSONW_WRITE(io, "left_fallspeed", p->left_fallspeed);
    JSONW_WRITE(io, "right_fallspeed", p->right_fallspeed);
    JSONW_WRITE(io, "tilt_angle", p->tilt_angle);
    JSONW_WRITE(io, "extra_rotation", p->extra_rotation);
    JSONW_WRITE(io, "water", p->water);
    JSONW_WRITE(io, "pitch", p->pitch);
    JSONW_WRITE(io, "propeller_roll", p->propeller_roll);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    item->extra_rotations = &p->propeller_roll;
    FX_Wake_Reset();
}

static M_MOUNT_TYPE M_CheckMount(
    const int16_t item_num, const COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->gun_status != LGS_ARMLESS) {
        return M_MOUNT_NONE;
    }

    ITEM *const item = Item_Get(item_num);
    ITEM *const lara_item = Lara_GetItem();
    const XYZ_32 delta = XYZ_32_Subtract(lara_item->pos, item->pos);
    if (XYZ_32_UnrotateYaw(delta, item->rot.y).z > WALL_L / 2) {
        return M_MOUNT_NONE;
    }

    M_MOUNT_TYPE result = M_MOUNT_NONE;
    const int16_t angle = item->rot.y - lara_item->rot.y;
    if (lara->water_status == LWS_SURFACE || lara->water_status == LWS_WADE) {
        if (!g_Input.action || lara_item->gravity || item->speed) {
            return M_MOUNT_NONE;
        }

        if (angle > DEG_45 && angle < DEG_135) {
            result = M_MOUNT_RIGHT;
        } else if (angle > -DEG_135 && angle < -DEG_45) {
            result = M_MOUNT_LEFT;
        }
    } else if (lara->water_status == LWS_ABOVE_WATER) {
        if (lara_item->fall_speed > 0) {
            if (lara_item->pos.y + 512 > item->pos.y) {
                result = M_MOUNT_JUMP;
            }
        } else if (lara_item->fall_speed == 0) {
            if (angle > -DEG_135 && angle < DEG_135) {
                result = XYZ_32_AreEquivalent(lara_item->pos, item->pos)
                    ? M_MOUNT_START
                    : M_MOUNT_JUMP;
            }
        }
    }

    if (result != M_MOUNT_NONE) {
        if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
            return M_MOUNT_NONE;
        }

        if (!Collide_TestCollision(item, lara_item)) {
            return M_MOUNT_NONE;
        }
    }

    return result;
}

static bool M_CheckDismount(const M_MOUNT_TYPE type)
{
    const ITEM *const item = Lara_Vehicle_GetItem();
    int16_t angle = item->rot.y;
    if (type == M_MOUNT_RIGHT) {
        angle += DEG_90;
    } else {
        angle -= DEG_90;
    }

    const XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, angle, WALL_L);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const HEIGHT_TYPE height_type = Room_GetHeight(sector, pos);

    if (height_type == HT_BIG_SLOPE || height_type == HT_DIAGONAL
        || height - item->pos.y < -WALL_L / 2) {
        return false;
    }

    const int32_t ceiling = Room_GetCeiling(sector, pos);
    if (ceiling - item->pos.y > -LARA_HEIGHT) {
        return false;
    }
    if (height - ceiling < LARA_HEIGHT) {
        return false;
    }

    return true;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (lara_item->hit_points < 0 || Lara_Vehicle_IsMounted()) {
        return;
    }

    const M_MOUNT_TYPE mount_type = M_CheckMount(item_num, coll);
    if (mount_type == M_MOUNT_NONE) {
        coll->enable_baddie_push = true;
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    Lara_Vehicle_SetIndex(item_num);
    M_ANIM boat_anim_idx;
    switch (mount_type) {
    case M_MOUNT_RIGHT:
        boat_anim_idx = M_ANIM_MOUNT_RIGHT;
        break;
    case M_MOUNT_LEFT:
        boat_anim_idx = M_ANIM_MOUNT_LEFT;
        break;
    case M_MOUNT_JUMP:
        boat_anim_idx = M_ANIM_MOUNT_JUMP;
        break;
    default:
        boat_anim_idx = M_ANIM_MOUNT_START;
        break;
    }

    Lara_Vehicle_SwitchToAnim(boat_anim_idx, 0);
    lara_item->current_anim_state = M_STATE_MOUNT;
    lara_item->goal_anim_state = M_STATE_MOUNT;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->water_status = LWS_ABOVE_WATER;

    ITEM *const item = Item_Get(item_num);
    lara_item->pos.x = item->pos.x;
    lara_item->pos.y = item->pos.y + M_SHIFT_Y;
    lara_item->pos.z = item->pos.z;
    lara_item->rot.x = 0;
    lara_item->rot.y = item->rot.y;
    lara_item->rot.z = 0;
    lara_item->gravity = 0;
    lara_item->speed = 0;
    lara_item->fall_speed = 0;

    if (lara_item->room_num != item->room_num) {
        Item_UpdateRoom(lara->item_num, item->room_num);
    }

    Item_Animate(lara_item);

    if (!Item_IsInPlay(item)) {
        Item_AddSimulated(item_num);
    }

    Vehicle_PlayTrackPool(item, "track", MPM_ONCE);
}

static bool M_UserControl(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    bool no_turn = true;

    if (item->pos.y < p->water - STEP_L / 2 || p->water == NO_HEIGHT) {
        return no_turn;
    }

    const bool input_left = g_Input.left || g_Input.step_left;
    const bool input_right = g_Input.right || g_Input.step_right;

    if ((g_Input.roll || g_Input.look) && item->speed == 0) {
        if (!input_left && !input_right) {
            item->speed = 0;
        } else if (!g_Input.roll) {
            item->speed = M_MIN_SPEED;
        }

        if (g_Input.look && item->speed == 0) {
            Lara_Look_UpDown();
        }

        return no_turn;
    }

    if ((input_left && !g_Input.jump) || (input_right && g_Input.jump)) {
        if (p->boat_turn > 0) {
            p->boat_turn -= M_UNDO_TURN;
        } else {
            p->boat_turn -= M_TURN;
            CLAMPL(p->boat_turn, -M_MAX_TURN);
        }
        no_turn = false;
    } else if ((input_right && !g_Input.jump) || (input_left && g_Input.jump)) {
        if (p->boat_turn < 0) {
            p->boat_turn += M_UNDO_TURN;
        } else {
            p->boat_turn += M_TURN;
            CLAMPG(p->boat_turn, M_MAX_TURN);
        }
        no_turn = false;
    }

    if (g_Input.jump) {
        if (item->speed > 0) {
            item->speed -= M_ACCELERATION;
        } else if (item->speed > -M_MIN_SPEED) {
            item->speed -= M_REVERSE_SPEED;
        }
    } else if (g_Input.action) {
        int16_t max_speed;
        if (g_Input.sprint) {
            max_speed = M_FAST_SPEED;
        } else {
            max_speed = g_Input.slow ? M_SLOW_SPEED : M_MAX_SPEED;
        }

        if (item->speed < max_speed) {
            item->speed = M_ACCELERATION * item->speed / (2 * max_speed)
                + item->speed + 2;
        } else if (item->speed > max_speed + 1) {
            item->speed--;
        }
    } else if (
        item->speed >= 0 && item->speed < M_MIN_SPEED
        && (input_left || input_right)) {
        if (item->speed == 0 && !g_Input.roll) {
            item->speed = M_MIN_SPEED;
        }
    } else if (item->speed > 1) {
        item->speed--;
    } else {
        item->speed = 0;
    }

    return no_turn;
}

static void M_Animate(const ITEM *const item, const M_ANIM collide_anim)
{
    const M_PRIV *const p = item->priv;
    ITEM *const lara_item = Lara_GetItem();

    if (lara_item->hit_points <= 0) {
        if (lara_item->current_anim_state != M_STATE_DEATH) {
            Lara_Vehicle_SwitchToAnim(M_ANIM_DEATH, 0);
            lara_item->current_anim_state = M_STATE_DEATH;
            lara_item->goal_anim_state = M_STATE_DEATH;
        }
        return;
    }

    if (item->pos.y < p->water - STEP_L / 2 && item->fall_speed > 0) {
        if (lara_item->current_anim_state != M_STATE_FALL) {
            Lara_Vehicle_SwitchToAnim(M_ANIM_FALL, 0);
            lara_item->current_anim_state = M_STATE_FALL;
            lara_item->goal_anim_state = M_STATE_FALL;
        }
        return;
    }

    if (collide_anim != 0) {
        if (lara_item->current_anim_state != M_STATE_HIT) {
            Lara_Vehicle_SwitchToAnim(collide_anim, 0);
            lara_item->current_anim_state = M_STATE_HIT;
            lara_item->goal_anim_state = M_STATE_HIT;
        }
        return;
    }

    const bool input_left = g_Input.left || g_Input.step_left;
    const bool input_right = g_Input.right || g_Input.step_right;

    switch (lara_item->current_anim_state) {
    case M_STATE_STILL:
        if (g_Input.roll && item->speed == 0) {
            if (input_right && M_CheckDismount(M_MOUNT_RIGHT)) {
                lara_item->goal_anim_state = M_STATE_JUMP_R;
            } else if (input_left && M_CheckDismount(M_MOUNT_LEFT)) {
                lara_item->goal_anim_state = M_STATE_JUMP_L;
            }
        }

        if (item->speed > 0) {
            lara_item->goal_anim_state = M_STATE_MOVING;
        }
        break;

    case M_STATE_MOVING:
        if (item->speed <= 0) {
            lara_item->goal_anim_state = M_STATE_STILL;
        } else if (input_right) {
            lara_item->goal_anim_state = M_STATE_TURN_R;
        } else if (input_left) {
            lara_item->goal_anim_state = M_STATE_TURN_L;
        }
        break;

    case M_STATE_FALL:
        lara_item->goal_anim_state = M_STATE_MOVING;
        break;

    case M_STATE_TURN_R:
        if (item->speed <= 0) {
            lara_item->goal_anim_state = M_STATE_STILL;
        } else if (!input_right) {
            lara_item->goal_anim_state = M_STATE_MOVING;
        }
        break;

    case M_STATE_TURN_L:
        if (item->speed <= 0) {
            lara_item->goal_anim_state = M_STATE_STILL;
        } else if (!input_left) {
            lara_item->goal_anim_state = M_STATE_MOVING;
        }
        break;
    }
}

static int32_t M_TestWaterHeight(
    const ITEM *const item, const int32_t z_off, const int32_t x_off,
    XYZ_32 *const pos)
{
    *pos = XYZ_32_OffsetLocalYaw(
        item->pos, (XYZ_32) { .x = x_off, .z = z_off }, item->rot.y);

    // The height the hull sits at follows the boat's pitch and roll, which the
    // yaw turn above says nothing about.
    pos->y = item->pos.y + ((x_off * Math_Sin(item->rot.z)) >> W2V_SHIFT)
        - ((z_off * Math_Sin(item->rot.x)) >> W2V_SHIFT);

    int16_t room_num = item->room_num;
    Room_GetSector(*pos, &room_num);
    int32_t height = Room_GetWaterHeight(*pos, room_num);

    if (height == NO_HEIGHT) {
        const SECTOR *const sector = Room_GetSector(*pos, &room_num);
        height = Room_GetHeight(sector, *pos);
        if (height == NO_HEIGHT) {
            return height;
        }
    }

    return height + M_SHIFT_Y;
}

static void M_DoShift(const int32_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    int16_t next_item_num = Room_Get(item->room_num)->item_num;

    while (next_item_num != NO_ITEM) {
        const ITEM *const next_item = Item_Get(next_item_num);
        if ((next_item->object_id == O_RIB || next_item->object_id == O_BOAT)
            && next_item_num != Lara_Vehicle_GetIndex()
            && next_item_num != item_num) {
            const int32_t dx = next_item->pos.x - item->pos.x;
            const int32_t dz = next_item->pos.z - item->pos.z;
            const int32_t dist = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
            if (dist < M_COLL_DIST) {
                item->pos.x = next_item->pos.x - M_COLL_DIST * dx / dist;
                item->pos.z = next_item->pos.z - M_COLL_DIST * dz / dist;
            }
            break;
        }

        next_item_num = next_item->next_item;
    }
}

static int32_t M_DoDynamics(
    const int32_t height, int32_t fall_speed, int32_t *const y_pos)
{
    if (height > *y_pos) {
        *y_pos += fall_speed;
        if (*y_pos <= height) {
            fall_speed += GRAVITY;
        } else {
            *y_pos = height;
            fall_speed = 0;
        }
    } else {
        fall_speed += (height - fall_speed - *y_pos) >> 3;
        CLAMPG(*y_pos, height);
    }

    return fall_speed;
}

static inline int32_t M_DoCornerShift(
    ITEM *const item, const int32_t x_off, const int32_t z_off,
    const XYZ_32 *const old_pos)
{
    XYZ_32 pos;
    const int32_t h = M_TestWaterHeight(item, x_off, z_off, &pos);
    if (h < old_pos->y - STEP_L / 2) {
        return Vehicle_DoShift(item, &pos, old_pos);
    }
    return 0;
}

static int32_t M_Dynamics(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    item->rot.z -= p->tilt_angle;

    XYZ_32 fl_old;
    XYZ_32 fr_old;
    XYZ_32 bl_old;
    XYZ_32 br_old;
    XYZ_32 f_old;
    const int32_t hfl_old = M_TestWaterHeight(item, M_FRONT, -M_SIDE, &fl_old);
    const int32_t hfr_old = M_TestWaterHeight(item, M_FRONT, M_SIDE, &fr_old);
    const int32_t hbl_old = M_TestWaterHeight(item, -M_FRONT, -M_SIDE, &bl_old);
    const int32_t hbr_old = M_TestWaterHeight(item, -M_FRONT, M_SIDE, &br_old);
    const int32_t hf_old = M_TestWaterHeight(item, M_TIP, 0, &f_old);
    CLAMPG(bl_old.y, hbl_old);
    CLAMPG(br_old.y, hbr_old);
    CLAMPG(fl_old.y, hfl_old);
    CLAMPG(fr_old.y, hfr_old);
    CLAMPG(f_old.y, hf_old);

    XYZ_32 old_pos = item->pos;

    item->rot.y += p->extra_rotation + p->boat_turn;
    p->tilt_angle = p->boat_turn * 6;
    item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, item->speed);

    if (item->speed < 0) {
        p->propeller_roll += DEG_1 * 33;
    } else {
        p->propeller_roll += DEG_1 * (3 * item->speed + 2);
    }

    int32_t slip = (Math_Sin(item->rot.z) * 30) >> W2V_SHIFT;
    if (slip == 0 && item->rot.z != 0) {
        slip = item->rot.z > 0 ? 1 : -1;
    }
    // Sideways, along the boat's beam. Both slides read their components off
    // a forward turn so that each rounds as it did: a slide runs frame after
    // frame, and a unit a frame would tell.
    const XYZ_32 roll_slip =
        XYZ_32_RotateYaw((XYZ_32) { .z = slip }, item->rot.y);
    item->pos.x += roll_slip.z;
    item->pos.z -= roll_slip.x;

    slip = (Math_Sin(item->rot.x) * 10) >> W2V_SHIFT;
    if (slip == 0 && item->rot.x != 0) {
        slip = item->rot.x > 0 ? 1 : -1;
    }
    item->pos = XYZ_32_Subtract(
        item->pos, XYZ_32_RotateYaw((XYZ_32) { .z = slip }, item->rot.y));

    XYZ_32 moved = {
        .x = item->pos.x,
        .y = 0,
        .z = item->pos.z,
    };
    M_DoShift(item_num);

    int32_t rot = 0;
    rot += M_DoCornerShift(item, -M_FRONT, -M_SIDE, &bl_old);
    rot += M_DoCornerShift(item, -M_FRONT, +M_SIDE, &br_old);
    rot += M_DoCornerShift(item, +M_FRONT, -M_SIDE, &fl_old);
    rot += M_DoCornerShift(item, +M_FRONT, +M_SIDE, &fr_old);
    if (slip == 0) {
        M_DoCornerShift(item, M_TIP, 0, &f_old);
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    int32_t height = Room_GetWaterHeight(item->pos, room_num);
    if (height == NO_HEIGHT) {
        height = Room_GetHeight(sector, item->pos);
    }

    if (height < item->pos.y - STEP_L / 2) {
        Vehicle_DoShift(item, &item->pos, &old_pos);
    }

    p->extra_rotation = rot;
    const int32_t coll_anim = Vehicle_GetCollisionAnim(item, &moved);

    if (slip != 0 || coll_anim != 0) {
        const XYZ_32 delta = XYZ_32_Subtract(item->pos, old_pos);
        int32_t new_speed = XYZ_32_UnrotateYaw(delta, item->rot.y).z;

        if (Lara_Vehicle_GetIndex() == item_num
            && item->speed > (M_MAX_SPEED + M_ACCELERATION)
            && new_speed < item->speed - 10) {
            Lara_TakeDamage(item->speed, true);
            Sound_Effect(SFX_LARA_INJURY, &Lara_GetItem()->pos, SPM_NORMAL);
            new_speed >>= 1;
            item->speed >>= 1;
        }

        if (slip != 0) {
            if (item->speed <= M_MAX_SPEED + 10) {
                item->speed = new_speed;
            }
        } else if (item->speed > 0 && new_speed < item->speed) {
            item->speed = new_speed;
        } else if (item->speed < 0 && new_speed > item->speed) {
            item->speed = new_speed;
        }
        CLAMPL(item->speed, -M_MIN_SPEED);
    }

    return coll_anim;
}

static void M_Splash(
    const ITEM *const item, const int32_t fall_speed,
    const int32_t water_height)
{
    FX_WATER_SPLASH_SETUP splash_setup = {
        .pos = { .x = item->pos.x, .y = water_height, .z = item->pos.z },
        .inner_xz_off = 64,
        .inner_xz_size = 48,
        .inner_y_size = -384,
        .inner_xz_vel = 160,
        .inner_y_vel = -128 * fall_speed,
        .inner_gravity = 128,
        .inner_friction = 7,
        .middle_xz_off = 96,
        .middle_xz_size = 96,
        .middle_y_size = -256,
        .middle_xz_vel = 224,
        .middle_y_vel = (-64 * fall_speed),
        .middle_gravity = 72,
        .middle_friction = 8,
        .outer_xz_off = 128,
        .outer_xz_size = 128,
        .outer_xz_vel = 272,
        .outer_friction = 9,
    };
    FX_Water_SetupSplash(&splash_setup);
}

static void M_TriggerMist(
    const XYZ_32 pos, const int32_t speed, const int16_t angle,
    const int32_t snow)
{
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 0;
    spark->src_color.g = 0;
    spark->src_color.b = 0;

    if (snow != 0) {
        spark->dst_color.r = 255;
        spark->dst_color.g = 255;
        spark->dst_color.b = 255;
    } else {
        spark->dst_color.r = 64;
        spark->dst_color.g = 64;
        spark->dst_color.b = 64;
    }

    spark->col_fade_speed = (Random_GetControl() & 3) + 4;
    spark->fade_to_black = 12 - (snow << 3);
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 3) + 20;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0xF) - 8;
    spark->pos.y = pos.y + (Random_GetControl() & 0xF) - 8;
    spark->pos.z = pos.z + (Random_GetControl() & 0xF) - 8;
    const XYZ_32 dir = XYZ_32_RotateYaw((XYZ_32) { .z = speed }, angle);
    spark->vel.x = (Random_GetControl() & 0x7F) + (dir.x >> 2) - 64;
    spark->vel.y = 12 * speed;
    spark->vel.z = (Random_GetControl() & 0x7F) + (dir.z >> 2) - 64;
    spark->friction = 3;

    if ((Random_GetControl() & 1) != 0) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = +16 + (Random_GetControl() & 0xF);
        }
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    if (snow != 0) {
        spark->friction = 0;
        spark->scalar = 3;
        spark->vel.y = -spark->vel.y >> 5;
        spark->max_y_vel = 0;
        spark->gravity = (Random_GetControl() & 0x1F) + 32;
        const uint8_t size = (Random_GetControl() & 7) + 16;
        spark->size.width = size;
        spark->src_size.width = spark->size.width;
        spark->dst_size.width = spark->size.width;
        spark->size.height = size;
        spark->src_size.height = spark->size.height;
        spark->dst_size.height = spark->size.height;
    } else {
        spark->scalar = 4;
        spark->max_y_vel = 0;
        spark->gravity = 0;
        const uint8_t size = (Random_GetControl() & 7) + (speed >> 1) + 16;
        spark->dst_size.width = size;
        spark->size.width = spark->dst_size.width >> 2;
        spark->src_size.width = spark->dst_size.width >> 2;
        spark->dst_size.height = size;
        spark->src_size.height = spark->dst_size.height >> 2;
        spark->size.height = spark->dst_size.height >> 2;
    }
    Sparks_FinishSetup(spark);
}

static void M_DoWake(
    const ITEM *const item, const int32_t x_off, const int32_t z_off,
    const int16_t rotate)
{
    const int32_t start_idx = FX_Wake_GetStartIndex();
    if (FX_Wake_GetPoint(start_idx, rotate)->life != 0) {
        return;
    }

    XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, z_off);
    pos = XYZ_32_OffsetYaw(pos, item->rot.y + DEG_90, x_off);
    pos.y += STEP_L / 2;
    int16_t room_num = item->room_num;
    Room_GetSector(pos, &room_num);
    const int32_t water_height = Room_GetWaterHeight(pos, room_num);
    if (water_height == NO_HEIGHT) {
        return;
    }

    int16_t angle1;
    int16_t angle2;
    if (item->speed >= 0) {
        if (rotate) {
            angle1 = item->rot.y + (DEG_1 * 160);
            angle2 = item->rot.y + (DEG_1 * 140);
        } else {
            angle1 = item->rot.y - (DEG_1 * 160);
            angle2 = item->rot.y - (DEG_1 * 140);
        }
    } else {
        if (rotate) {
            angle1 = item->rot.y + (DEG_1 * 20);
            angle2 = item->rot.y + (DEG_1 * 40);
        } else {
            angle1 = item->rot.y - (DEG_1 * 20);
            angle2 = item->rot.y - (DEG_1 * 40);
        }
    }

    XYZ_32 vel[2] = {
        XYZ_32_OffsetYaw((XYZ_32) {}, angle1, 4),
        XYZ_32_OffsetYaw((XYZ_32) {}, angle2, 10),
    };
    FX_WAKE_POINT *const pt = FX_Wake_GetPoint(start_idx, rotate);
    pt->life = 64;

    for (int32_t i = 0; i < 2; i++) {
        pt->pos[i].x = pos.x;
        pt->pos[i].y = item->pos.y + 32;
        pt->pos[i].z = pos.z;
        pt->prev_pos[i] = pt->pos[i];
        pt->vel[i].x = vel[i].x;
        pt->vel[i].z = vel[i].z;
    }

    if (rotate == 1) {
        FX_Wake_AdvanceStartIndex();
    }
}

static void M_ControlWake(const ITEM *const item)
{
    const XYZ_32 pos = {
        .x = item->pos.x,
        .y = item->pos.y + STEP_L / 2,
        .z = item->pos.z,
    };
    int16_t room_num = item->room_num;
    Room_GetSector(pos, &room_num);
    const int32_t water_height = Room_GetWaterHeight(pos, room_num);
    const bool valid_height =
        water_height <= item->pos.y + 32 && water_height != NO_HEIGHT;

    const ITEM *const lara_item = Lara_GetItem();
    const bool leaving = lara_item->current_anim_state == M_STATE_JUMP_R
        || lara_item->current_anim_state == M_STATE_JUMP_L;

    const int32_t time4 = Output_GetTimeInGame() * 4;
    if ((time4 & 0xF) == 0 && valid_height && !leaving) {
        M_DoWake(item, -384, 0, 0);
        M_DoWake(item, 384, 0, 1);
    }

    uint8_t wake_shade = FX_Wake_GetShade();
    if (item->speed == 0 || !valid_height || leaving) {
        if (wake_shade != 0) {
            wake_shade--;
        }
    } else if (wake_shade < 16) {
        wake_shade++;
    }
    FX_Wake_SetShade(wake_shade);
}

static void M_ControlEffects(const ITEM *const item)
{
    XYZ_32 pos = m_Propeller.pos;
    Collide_GetJointAbsPosition(item, &pos, m_Propeller.mesh_num);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t water_height = Room_GetWaterHeight(pos, room_num);

    if (item->speed != 0 && water_height < pos.y && water_height != NO_HEIGHT) {
        M_TriggerMist(pos, ABS(item->speed), item->rot.y + 0x8000, 0);
        if ((Random_GetControl() & 1) == 0) {
            XYZ_32 bubble_pos = {
                .x = pos.x + (Random_GetControl() & 0x3F) - 32,
                .y = pos.y + (Random_GetControl() & 0xF),
                .z = pos.z + (Random_GetControl() & 0x3F) - 32,
            };
            room_num = item->room_num;
            Room_GetSector(bubble_pos, &room_num);
            Spawn_BubbleEx(&bubble_pos, room_num, 16, 8);
        }
        return;
    }

    const int32_t height = Room_GetHeight(sector, pos);
    if (pos.y > height && !(Room_Get(room_num)->flags.underwater)) {
        for (int32_t i = (Random_GetControl() & 3) + 3; i > 0; i--) {
            const int16_t angle = item->rot.y + Random_GetControl() + DEG_90;
            M_TriggerMist(
                pos, ((Random_GetControl() & 0xF) + 96) << 4, angle, 1);
        }
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    bool drive = false;
    bool no_turn = true;
    const int32_t coll_anim = M_Dynamics(item_num);

    XYZ_32 fl;
    XYZ_32 fr;
    const int32_t hfl = M_TestWaterHeight(item, M_FRONT, -M_SIDE, &fl);
    const int32_t hfr = M_TestWaterHeight(item, M_FRONT, M_SIDE, &fr);

    int16_t room_num = item->room_num;
    const SECTOR *sector = Room_GetSector(
        (XYZ_32) {
            item->pos.x,
            item->pos.y + M_SHIFT_Y,
            item->pos.z,
        },
        &room_num);
    int32_t height = Room_GetHeight(sector, item->pos);
    Room_GetCeiling(sector, item->pos);

    ITEM *const lara_item = Lara_GetItem();
    if (Lara_Vehicle_GetIndex() == item_num) {
        Vehicle_TestTriggers(lara_item, item);
    }

    int32_t water_height = Room_GetWaterHeight(item->pos, room_num);
    p->water = water_height;

    if (Lara_Vehicle_GetIndex() == item_num && lara_item->hit_points > 0) {
        switch (lara_item->current_anim_state) {
        case M_STATE_MOUNT:
        case M_STATE_JUMP_R:
        case M_STATE_JUMP_L:
            break;
        default:
            drive = true;
            no_turn = M_UserControl(item);
            break;
        }
    } else if (item->speed > 1) {
        item->speed--;
    } else {
        item->speed = 0;
    }

    if (no_turn) {
        if (p->boat_turn < -M_UNDO_TURN) {
            p->boat_turn += M_UNDO_TURN;
        } else if (p->boat_turn > M_UNDO_TURN) {
            p->boat_turn -= M_UNDO_TURN;
        } else {
            p->boat_turn = 0;
        }
    }

    item->floor = height + M_SHIFT_Y;
    if (p->water == NO_HEIGHT) {
        p->water = height;
    } else {
        p->water += M_SHIFT_Y;
    }

    p->left_fallspeed = M_DoDynamics(hfl, p->left_fallspeed, &fl.y);
    p->right_fallspeed = M_DoDynamics(hfr, p->right_fallspeed, &fr.y);

    const int16_t fall_speed = item->fall_speed;
    item->fall_speed = M_DoDynamics(p->water, item->fall_speed, &item->pos.y);

    if (fall_speed - item->fall_speed > 32 && !item->fall_speed
        && water_height != NO_HEIGHT) {
        M_Splash(item, fall_speed - item->fall_speed, water_height);
    }

    height = fr.y + fl.y;
    if (height >= 0) {
        height >>= 1;
    } else {
        height = -ABS(height) >> 1;
    }

    const int16_t x_rot = Math_Atan(M_FRONT, item->pos.y - height);
    const int16_t z_rot = Math_Atan(M_SIDE, height - fl.y);
    item->rot.x += (x_rot - item->rot.x) / 2;
    item->rot.z += (z_rot - item->rot.z) / 2;

    if (x_rot == 0 && ABS(item->rot.x) < 4) {
        item->rot.x = 0;
    }
    if (z_rot == 0 && ABS(item->rot.z) < 4) {
        item->rot.z = 0;
    }

    if (Lara_Vehicle_GetIndex() == item_num) {
        M_Animate(item, coll_anim);

        if (room_num != item->room_num) {
            Item_UpdateRoom(item_num, room_num);
        }

        item->rot.z += p->tilt_angle;
        lara_item->pos = item->pos;
        lara_item->rot = item->rot;

        sector = Room_GetSector(
            (XYZ_32) {
                lara_item->pos.x,
                lara_item->pos.y + M_SHIFT_Y,
                lara_item->pos.z,
            },
            &room_num);
        Item_UpdateRoom(Item_GetIndex(lara_item), room_num);

        Item_Animate(lara_item);

        if (lara_item->hit_points > 0) {
            Lara_Vehicle_SyncItemAnim();
        }

        g_Camera.target_elevation = M_CAM_ELEVATION;
        g_Camera.target_distance = M_CAM_DISTANCE;
    } else {
        if (room_num != item->room_num) {
            Item_UpdateRoom(item_num, room_num);
        }
        item->rot.z += p->tilt_angle;
    }

    p->pitch += (item->speed - p->pitch) >> 2;
    if (item->speed > 8) {
        Sound_Effect(
            SFX_RIB_MOVING, &item->pos,
            SPM_PITCH | ((0x10000 - (M_MAX_SPEED - p->pitch) * 100) << 8));
    } else if (drive) {
        Sound_Effect(
            SFX_RIB_IDLE, &item->pos,
            SPM_PITCH | ((0x10000 - (M_MAX_SPEED - p->pitch) * 100) << 8));
    }

    if (Lara_Vehicle_GetIndex() != item_num) {
        return;
    }

    if ((lara_item->current_anim_state == M_STATE_JUMP_R
         || lara_item->current_anim_state == M_STATE_JUMP_L)
        && Item_TestFrameEqual(lara_item, -1)) {
        if (lara_item->current_anim_state == M_STATE_JUMP_L) {
            lara_item->rot.y -= DEG_90;
        } else {
            lara_item->rot.y += DEG_90;
        }

        Lara_Vehicle_Dismount();
        Item_SwitchToAnim(lara_item, LA(LA_JUMP_FORWARD), 0);
        lara_item->current_anim_state = LS(LS_JUMP_FORWARD);
        lara_item->goal_anim_state = LS(LS_JUMP_FORWARD);
        lara_item->gravity = true;
        lara_item->fall_speed = -40;
        lara_item->speed = 20;
        lara_item->rot.x = 0;
        lara_item->rot.z = 0;

        room_num = lara_item->room_num;
        XYZ_32 pos = XYZ_32_OffsetYaw(lara_item->pos, lara_item->rot.y, 360);
        pos.y -= 90;
        sector = Room_GetSector(pos, &room_num);

        if (Room_GetHeight(sector, pos) >= pos.y - STEP_L) {
            lara_item->pos.x = pos.x;
            lara_item->pos.z = pos.z;
            Item_UpdateRoom(Item_GetIndex(lara_item), room_num);
        }

        lara_item->pos.y = pos.y;
        Item_SwitchToAnim(item, 0, 0);
    }

    M_ControlWake(item);
    M_ControlEffects(item);
}

static bool M_Draw(const ITEM *const item)
{
    Object_DrawAnimatingItem(item);
    FX_Wake_Draw(item);
    return true;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 1)->rot.z = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "track_1", Music_IDToSlot(MX_RIB_THEME),
            "Random music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_2", -1, "Random music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_3", -1, "Random music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_4", -1, "Random music track pool, slot 4. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "is_heavy", true,
            "Whether or not this vehicle can activate heavy triggers."));
}

REGISTER_OBJECT(O_RIB, M_Setup)
