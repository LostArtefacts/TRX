#include <trx/game/objects/vehicles/jeep.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/camera.h>
#include <trx/game/fx/debris.h>
#include <trx/game/gun.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects/traps/scaled_spikes.h>
#include <trx/game/objects/vehicles/common.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_BRAKE_ON_BITS  0b00000010'01111111'11111111
#define M_BRAKE_OFF_BITS 0b00000001'10111111'11111111
#define M_BRAKE_GLOW     ((RGB_888) { 64, 0, 0 })
#define M_RADIUS         100
#define M_WIDTH          550
#define M_DEPTH          600
#define M_COLL_ROOMS     22
#define M_COLL_DISTANCE  (WALL_L * 2) // = 2048
#define M_HIT_SPEED      10922
#define M_SKID_SPEED     21844
#define M_HIGH_SPEED     (WALL_L * 16) // = 16384
#define M_MIN_SPEED      (-M_HIGH_SPEED) // = -16384
#define M_MAX_SPEED      (M_HIGH_SPEED * 2) // =32768
#define M_LOW_ACCEL      (M_HIGH_SPEED + WALL_L * 2) // = 18432
#define M_MID_ACCEL      (M_MAX_SPEED - WALL_L * 4) // = 28672
#define M_HIGH_ACCEL     (M_MAX_SPEED - WALL_L * 2) // = 30720
#define M_MAX_DECEL      (STEP_L * 3) // = 768
#define M_MIN_DECEL      STEP_L
#define M_MIN_PITCH      (-0x8000)
#define M_MAX_PITCH      0xA000
#define M_MAX_TURN       (DEG_1 * 5) // = 910
#define M_HIGH_TURN_RATE (DEG_1 * 4 / 3) // = 242
#define M_MID_TURN_RATE  DEG_1
#define M_LOW_TURN_RATE  (DEG_1 / 2) // = 91
#define M_TURN_1         (DEG_1 * 3 / 2) // = 273
#define M_TURN_2         (DEG_1 * 75) // = 13650
#define M_TURN_3         (DEG_1 * 90) // = 16380
#define M_CAM_DISTANCE   (WALL_L * 2) // = 2048
#define M_CAM_ELEVATION  (DEG_1 * -30) // = -5460
#define M_CAM_TURN       (DEG_1 * 179) // = 32578
// clang-format on

typedef struct {
    XYZ_32 fl_pos;
    XYZ_32 fr_pos;
    XYZ_32 fm_pos;
    int32_t fl_height;
    int32_t fr_height;
    int32_t fm_height;
} M_TILT_ARGS;

typedef enum {
    M_MOUNT_NONE,
    M_MOUNT_LEFT,
    M_MOUNT_RIGHT,
    M_MOUNT_START,
} M_MOUNT_TYPE;

typedef enum {
    // clang-format off
    M_ANIM_TURN_LEFT          = 4,
    M_ANIM_FALL_FORWARD_START = 6,
    M_ANIM_MOUNT_RIGHT        = 9,
    M_ANIM_HIT_BACK           = 10,
    M_ANIM_HIT_FRONT          = 11,
    M_ANIM_HIT_LEFT           = 12,
    M_ANIM_HIT_RIGHT          = 13,
    M_ANIM_STOP               = 14,
    M_ANIM_TURN_RIGHT         = 16,
    M_ANIM_MOUNT_LEFT         = 18,
    M_ANIM_FALL_BACK_START    = 20,
    M_ANIM_REVERSE_WAIT       = 27,
    M_ANIM_REVERSE            = 30,
    M_ANIM_TURN_LEFT_START    = 32,
    M_ANIM_TURN_RIGHT_START   = 33,
    M_ANIM_REVERSE_LEFT       = 36,
    M_ANIM_REVERSE_RIGHT      = 37,
    M_ANIM_REVERSE_START      = 40,
    M_ANIM_REVERSE_CONTINUE   = 41,
    M_ANIM_REVERSE_END        = 44,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_STOP,
    M_STATE_DRIVE,
    M_STATE_HIT_LEFT,
    M_STATE_HIT_RIGHT,
    M_STATE_HIT_FRONT,
    M_STATE_HIT_BACK,
    M_STATE_SKID,
    M_STATE_TURN_LEFT,
    M_STATE_TURN_RIGHT,
    M_STATE_MOUNT,
    M_STATE_DISMOUNT,
    M_STATE_FALL,
    M_STATE_LAND,
    M_STATE_REVERSE,
    M_STATE_REVERSE_RIGHT,
    M_STATE_REVERSE_LEFT,
    M_STATE_DEATH,
    M_STATE_REVERSE_START,
} M_STATE;

typedef struct {
    int16_t wheel_rots[4];
    int32_t velocity;
    int32_t acceleration;
    int32_t pitch_1;
    int32_t pitch_2;
    int32_t turn_rate;
    int32_t camera_angle;
    int16_t move_angle;
    int16_t extra_rotation;
    int16_t rot_shift;
    int16_t gear;
    int16_t flags;
    MUSIC_SLOT ambience;
    bool requires_key;
} M_PRIV;

static const BITE m_BrakeLight = {
    .pos = { .x = 0, .y = -144, .z = -WALL_L },
    .mesh_num = 11,
};

static const BITE m_Exhaust = {
    .pos = { .x = 80, .y = 0, .z = -500 },
    .mesh_num = 11,
};

static bool m_DontExit = false;
static uint8_t m_ExhaustSmokeVel = 0;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "velocity", &p->velocity));
    MUST(JSON_READ_OPT(io, "acceleration", &p->acceleration));
    MUST(JSON_READ_OPT(io, "pitch_1", &p->pitch_1));
    MUST(JSON_READ_OPT(io, "pitch_2", &p->pitch_2));
    MUST(JSON_READ_OPT(io, "turn_rate", &p->turn_rate));
    MUST(JSON_READ_OPT(io, "camera_angle", &p->camera_angle));
    MUST(JSON_READ_OPT(io, "move_angle", &p->move_angle));
    MUST(JSON_READ_OPT(io, "extra_rotation", &p->extra_rotation));
    MUST(JSON_READ_OPT(io, "rot_shift", &p->rot_shift));
    MUST(JSON_READ_OPT(io, "gear", &p->gear));
    MUST(JSON_READ_OPT(io, "flags", &p->flags));
    MUST(JSON_READ_OPT(io, "ambience", &p->ambience));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "velocity", p->velocity);
    JSONW_WRITE(io, "acceleration", p->acceleration);
    JSONW_WRITE(io, "pitch_1", p->pitch_1);
    JSONW_WRITE(io, "pitch_2", p->pitch_2);
    JSONW_WRITE(io, "turn_rate", p->turn_rate);
    JSONW_WRITE(io, "camera_angle", p->camera_angle);
    JSONW_WRITE(io, "move_angle", p->move_angle);
    JSONW_WRITE(io, "extra_rotation", p->extra_rotation);
    JSONW_WRITE(io, "rot_shift", p->rot_shift);
    JSONW_WRITE(io, "gear", p->gear);
    JSONW_WRITE(io, "flags", p->flags);
    JSONW_WRITE(io, "ambience", p->ambience);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->move_angle = item->rot.y;
    p->ambience = MX_INACTIVE;
    item->mesh_bits = M_BRAKE_OFF_BITS;
    item->extra_rotations = p->wheel_rots;
}

static int32_t M_TestHeight(
    const ITEM *const item, const int32_t z_off, const int32_t x_off,
    XYZ_32 *const pos)
{
    *pos = XYZ_32_OffsetLocalYaw(
        item->pos, (XYZ_32) { .x = x_off, .z = z_off }, item->rot.y);

    pos->y = item->pos.y + ((x_off * Math_Sin(item->rot.z)) >> W2V_SHIFT)
        - ((z_off * Math_Sin(item->rot.x)) >> W2V_SHIFT);

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(*pos, &room_num);
    const int32_t ceiling = Room_GetCeiling(sector, *pos);

    if (pos->y < ceiling || ceiling == NO_HEIGHT) {
        return NO_HEIGHT;
    }

    const int32_t height = Room_GetHeight(sector, *pos);
    CLAMPG(pos->y, height);
    return height;
}

static XZ_16 M_GetTilt(
    const ITEM *const item, const int32_t old_y, const M_TILT_ARGS *const args)
{
    XZ_16 tilt = {};
    const int32_t height_diff = (args->fr_pos.y + args->fl_pos.y) >> 1;

    if (args->fm_pos.y < args->fm_height) {
        if (height_diff < (args->fl_height + args->fr_height) >> 1) {
            tilt.x = Math_Atan(137, old_y - item->pos.y);
            const M_PRIV *const p = item->priv;
            if (p->velocity < 0) {
                tilt.x = -tilt.x;
            }
        } else {
            tilt.x = Math_Atan(M_WIDTH, item->pos.y - height_diff);
        }
    } else {
        if (height_diff < (args->fl_height + args->fr_height) >> 1) {
            tilt.x = Math_Atan(M_WIDTH, args->fm_height - item->pos.y);
        } else {
            tilt.x = Math_Atan(M_WIDTH * 2, args->fm_height - height_diff);
        }
    }

    tilt.z = Math_Atan(350, height_diff - args->fl_pos.y);
    return tilt;
}

static void M_StartMounted(ITEM *const item, ITEM *const lara_item)
{
    item->floor = Item_GetHeight(item);

    XYZ_32 fl_pos = {};
    XYZ_32 fr_pos = {};
    XYZ_32 fm_pos = {};

    // clang-format off
    const int32_t fl_height = M_TestHeight(item, +M_WIDTH, -STEP_L, &fl_pos);
    const int32_t fr_height = M_TestHeight(item, +M_WIDTH, +STEP_L, &fr_pos);
    const int32_t fm_height = M_TestHeight(item, -M_DEPTH,       0, &fm_pos);
    // clang-format on

    const XZ_16 tilt = M_GetTilt(
        item, item->pos.y,
        &(M_TILT_ARGS) {
            .fl_pos = fl_pos,
            .fr_pos = fr_pos,
            .fm_pos = fm_pos,
            .fl_height = fl_height,
            .fr_height = fr_height,
            .fm_height = fm_height,
        });
    item->rot.x = tilt.x;
    item->rot.z = tilt.z;
    lara_item->rot = item->rot;

    Lara_Hair_Initialise();
    Interpolation_RememberItem(item);
    Interpolation_RememberItem(lara_item);
    Camera_ResetPosition();
}

static void M_ShowInventory(void)
{
    if (!Inv_HasItem(O_PUZZLE_ITEM_1)) {
        Lara_RefuseInteraction();
        return;
    }

    InvRing_SetRequestedObjectID(O_PUZZLE_ITEM_1);
    const GF_COMMAND gf_cmd = GF_ShowInventory(INV_KEYS_MODE);
    if (gf_cmd.action != GF_NOOP) {
        GF_OverrideCommand(gf_cmd, true);
    }
}

static void M_ActivateBaddies(const ITEM *const item)
{
    // TODO: activate any O_ENEMY_JEEP instances in the same room.
}

static M_MOUNT_TYPE M_GetMountType(
    const ITEM *const item, const ITEM *const lara_item)
{
    const XYZ_32 delta = XYZ_32_Subtract(item->pos, lara_item->pos);
    if (ABS(delta.y) >= STEP_L || !Lara_TestBoundsCollide(item, M_RADIUS)) {
        return M_MOUNT_NONE;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);
    if (height < -MAX_HEIGHT) {
        return M_MOUNT_NONE;
    }

    const int16_t angle = Math_Atan(delta.z, delta.x) - item->rot.y;
    const int16_t lara_angle = lara_item->rot.y - item->rot.y;

    M_MOUNT_TYPE mount = M_MOUNT_NONE;
    if (angle <= -8190 || angle >= 24570) {
        if (lara_angle > -24586 && lara_angle < -8206) {
            mount = M_MOUNT_RIGHT;
        }
    } else if (lara_angle > 8190 && lara_angle < 24570) {
        mount = M_MOUNT_LEFT;
    }

    return mount;
}

static M_MOUNT_TYPE M_TestMount(const int16_t item_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    const ITEM *const item = Item_Get(item_num);

    if (lara_item->fall_speed == 0 && lara_item->room_num == item->room_num
        && ABS(lara_item->rot.y - item->rot.y) < DEG_1
        && XYZ_32_GetDistance(item->pos, lara_item->pos) < M_RADIUS / 2) {
        return M_MOUNT_START;
    }

    if (!Lara_Interact_CanControl(LARA_INTERACT_VEHICLE, item_num)) {
        return M_MOUNT_NONE;
    }

    return M_GetMountType(item, lara_item);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (lara_item->hit_points <= 0 || Lara_Vehicle_IsMounted()) {
        return;
    }

    M_MOUNT_TYPE mount = M_TestMount(item_num);
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if ((mount == M_MOUNT_LEFT || mount == M_MOUNT_RIGHT) && p->requires_key
        && !Lara_Interact_HasActiveTarget(item_num)) {
        M_ShowInventory();
        mount = M_MOUNT_NONE;
    }

    if (mount == M_MOUNT_NONE) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    Lara_Vehicle_SetIndex(item_num);
    item->hit_points = 1;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Gun_IsFlareType(lara->gun_type)) {
        Gun_Flare_Dispose(false);
        lara->gun_type = LGT_UNARMED;
        lara->request_gun_type = LGT_UNARMED;
    }
    lara->gun_status = LGS_HANDS_BUSY;

    if (mount == M_MOUNT_START) {
        Lara_Vehicle_SwitchToAnim(M_ANIM_STOP, 0);
        lara_item->current_anim_state = M_STATE_STOP;
        lara_item->goal_anim_state = M_STATE_STOP;
        M_StartMounted(item, lara_item);
    } else {
        if (mount == M_MOUNT_RIGHT) {
            Lara_Vehicle_SwitchToAnim(M_ANIM_MOUNT_RIGHT, 0);
        } else {
            Lara_Vehicle_SwitchToAnim(M_ANIM_MOUNT_LEFT, 0);
        }
        lara_item->current_anim_state = M_STATE_MOUNT;
        lara_item->goal_anim_state = M_STATE_MOUNT;

        M_ActivateBaddies(item);

        lara_item->pos = item->pos;
        lara_item->rot.y = item->rot.y;
    }

    lara->head_rot.y = 0;
    lara->head_rot.x = 0;
    lara->torso_rot.y = 0;
    lara->torso_rot.x = 0;
    lara->hit_direction = DIR_UNKNOWN;
    lara->interact_target.item_num = NO_ITEM;
    lara->interact_target.is_moving = false;
    Item_Animate(lara_item);

    p->acceleration = 0;
    p->gear = 0;

    p->ambience = Music_GetCurrentLoopedTrack();
    Vehicle_PlayTrackPool(item, "track", MPM_LOOP);
}

static bool M_CanDismount(const ITEM *const item)
{
    const XYZ_32 pos =
        XYZ_32_OffsetYaw(item->pos, item->rot.y + DEG_90, STEP_L * 2);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const HEIGHT_TYPE height_type = Room_GetHeightType();

    if (height_type == HT_BIG_SLOPE || height_type == HT_DIAGONAL) {
        return false;
    }

    if (height == NO_HEIGHT || ABS(height - item->pos.y) > STEP_L * 2) {
        return false;
    }

    const int32_t ceiling = Room_GetCeiling(sector, pos);
    return ceiling - item->pos.y <= -LARA_HEIGHT
        && height - ceiling >= LARA_HEIGHT;
}

static bool M_IsUsable(const int16_t item_num)
{
    return M_GetMountType(Item_Get(item_num), Lara_GetItem()) != M_MOUNT_NONE;
}

static void M_Explode(ITEM *const item)
{
    if (Room_Get(item->room_num)->flags.underwater) {
        Sparks_TriggerUnderwaterExplosion(item);
    } else {
        Sparks_TriggerExplosionSparks(item->pos, 3, -2, 0, item->room_num);
        for (int32_t i = 0; i < 3; i++) {
            Sparks_TriggerExplosionSparks(item->pos, 3, -1, 0, item->room_num);
        }
    }

    const int16_t vehicle_item_num = Lara_Vehicle_GetIndex();
    Item_Shatter(
        vehicle_item_num, (ITEM_SHATTER_ARGS) { .mesh_bits = ~ITEM_MESH(0) });
    Item_Destroy(vehicle_item_num);
    Item_SetFinished(item, true);
    Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_NORMAL);
    Sound_Effect(SFX_EXPLOSION_2, nullptr, SPM_NORMAL);
    Lara_Vehicle_SetIndex(NO_ITEM);
}

static void M_Animate(
    ITEM *const item, const int32_t hit_wall, const bool killed)
{
    ITEM *const lara_item = Lara_GetItem();
    M_PRIV *const p = item->priv;

    const int16_t state = lara_item->current_anim_state;
    if (item->pos.y != item->floor && state != M_STATE_FALL
        && state != M_STATE_LAND && !killed) {
        if (p->gear == 1) {
            Lara_Vehicle_SwitchToAnim(M_ANIM_FALL_BACK_START, 0);
        } else {
            Lara_Vehicle_SwitchToAnim(M_ANIM_FALL_FORWARD_START, 0);
        }
        lara_item->current_anim_state = M_STATE_FALL;
        lara_item->goal_anim_state = M_STATE_FALL;
    } else if (
        hit_wall && p->velocity > M_HIT_SPEED && !killed
        && state != M_STATE_HIT_FRONT && state != M_STATE_HIT_BACK
        && state != M_STATE_HIT_LEFT && state != M_STATE_HIT_RIGHT
        && state != M_STATE_FALL) {
        switch (hit_wall) {
        case 13:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_FRONT, 0);
            lara_item->current_anim_state = M_STATE_HIT_FRONT;
            lara_item->goal_anim_state = M_STATE_HIT_FRONT;
            break;

        case 14:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_BACK, 0);
            lara_item->current_anim_state = M_STATE_HIT_BACK;
            lara_item->goal_anim_state = M_STATE_HIT_BACK;
            break;

        case 11:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_LEFT, 0);
            lara_item->current_anim_state = M_STATE_HIT_LEFT;
            lara_item->goal_anim_state = M_STATE_HIT_LEFT;
            break;

        default:
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_RIGHT, 0);
            lara_item->current_anim_state = M_STATE_HIT_RIGHT;
            lara_item->goal_anim_state = M_STATE_HIT_RIGHT;
            break;
        }
    } else {
        switch (state) {
        case M_STATE_STOP:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_DEATH;
            } else if (
                g_Input.jump && g_Input.left && p->velocity == 0
                && !m_DontExit) {
                if (M_CanDismount(item)) {
                    lara_item->goal_anim_state = M_STATE_DISMOUNT;
                }
            } else if (g_InputDB.slow) {
                if (p->gear != 0) {
                    p->gear--;
                }
            } else if (g_InputDB.sprint) {
                if (p->gear < 1) {
                    p->gear++;
                }
                if (p->gear == 1) {
                    lara_item->goal_anim_state = M_STATE_REVERSE_START;
                }
            } else if (g_Input.action && g_Input.jump) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            } else if (g_Input.step_left || g_Input.left) {
                lara_item->goal_anim_state = M_STATE_TURN_LEFT;
            } else if (g_Input.step_right || g_Input.right) {
                lara_item->goal_anim_state = M_STATE_TURN_RIGHT;
            }
            break;

        case M_STATE_DRIVE:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (
                (p->velocity & 0xFFFFFF00) != 0 || g_Input.action
                || g_Input.jump) {
                if (g_Input.jump) {
                    if (p->velocity > M_SKID_SPEED) {
                        lara_item->goal_anim_state = M_STATE_SKID;
                    } else {
                        lara_item->goal_anim_state = M_STATE_STOP;
                    }
                } else if (g_Input.step_left || g_Input.left) {
                    lara_item->goal_anim_state = M_STATE_TURN_LEFT;
                } else if (g_Input.step_right || g_Input.right) {
                    lara_item->goal_anim_state = M_STATE_TURN_RIGHT;
                }
            } else {
                lara_item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_HIT_LEFT:
        case M_STATE_HIT_RIGHT:
        case M_STATE_HIT_FRONT:
        case M_STATE_HIT_BACK:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (g_Input.action || g_Input.jump) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            }
            break;

        case M_STATE_SKID:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if ((p->velocity & 0xFFFFFF00) != 0) {
                if (g_Input.step_left || g_Input.left) {
                    lara_item->goal_anim_state = M_STATE_TURN_LEFT;
                } else if (g_Input.step_right || g_Input.right) {
                    lara_item->goal_anim_state = M_STATE_TURN_RIGHT;
                }
            } else {
                lara_item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_TURN_LEFT:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (g_InputDB.slow) {
                if (p->gear != 0) {
                    p->gear--;
                }
            } else if (g_InputDB.sprint) {
                if (p->gear < 1) {
                    p->gear++;
                }
                if (p->gear == 1) {
                    Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE_START, 0);
                    lara_item->current_anim_state = M_STATE_REVERSE_LEFT;
                    lara_item->goal_anim_state = M_STATE_REVERSE_LEFT;
                    break;
                }
            } else if (g_Input.step_right || g_Input.right) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            } else if (g_Input.step_left || g_Input.left) {
                lara_item->goal_anim_state = M_STATE_TURN_LEFT;
            } else if (p->velocity != 0) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            } else {
                lara_item->goal_anim_state = M_STATE_STOP;
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_TURN_LEFT)
                && p->velocity == 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_TURN_LEFT_START, 14);
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_TURN_LEFT_START)
                && p->velocity != 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_TURN_LEFT, 0);
            }
            break;

        case M_STATE_TURN_RIGHT:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_STOP;
            } else if (g_InputDB.slow) {
                if (p->gear != 0) {
                    p->gear--;
                }
            } else if (g_InputDB.sprint) {
                if (p->gear < 1) {
                    p->gear++;
                }

                if (p->gear == 1) {
                    Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE_CONTINUE, 0);
                    lara_item->current_anim_state = M_STATE_REVERSE_RIGHT;
                    lara_item->goal_anim_state = M_STATE_REVERSE_RIGHT;
                    break;
                }
            } else if (g_Input.step_left || g_Input.left) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            } else if (g_Input.step_right || g_Input.right) {
                lara_item->goal_anim_state = M_STATE_TURN_RIGHT;
            } else if (p->velocity != 0) {
                lara_item->goal_anim_state = M_STATE_DRIVE;
            } else {
                lara_item->goal_anim_state = M_STATE_STOP;
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_TURN_RIGHT)
                && p->velocity == 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_TURN_RIGHT_START, 14);
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_TURN_RIGHT_START)
                && p->velocity != 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_TURN_RIGHT, 0);
            }
            break;

        case M_STATE_FALL:
            if (item->pos.y == item->floor) {
                lara_item->goal_anim_state = M_STATE_LAND;
            } else if (item->fall_speed > 300) {
                p->flags |= 0x40;
            }
            break;

        case M_STATE_REVERSE:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_REVERSE_START;
            } else if ((ABS(p->velocity) & 0xFFFFFF00) != 0) {
                if (g_Input.step_left || g_Input.left) {
                    lara_item->goal_anim_state = M_STATE_REVERSE_LEFT;
                } else if (g_Input.step_right || g_Input.right) {
                    lara_item->goal_anim_state = M_STATE_REVERSE_RIGHT;
                }
            } else {
                lara_item->goal_anim_state = M_STATE_REVERSE_START;
            }
            break;

        case M_STATE_REVERSE_RIGHT:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_REVERSE_START;
            } else if (g_InputDB.slow) {
                if (p->gear != 0) {
                    p->gear--;
                    if (p->gear == 0) {
                        Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE_END, 0);
                        lara_item->current_anim_state = M_STATE_TURN_RIGHT;
                        lara_item->goal_anim_state = M_STATE_TURN_RIGHT;
                        break;
                    }
                }
            } else if (g_InputDB.sprint) {
                if (p->gear < 1) {
                    p->gear++;
                }
            } else if (g_Input.step_right || g_Input.right) {
                lara_item->goal_anim_state = M_STATE_REVERSE_RIGHT;
            } else {
                lara_item->goal_anim_state = M_STATE_REVERSE;
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_REVERSE)
                && p->velocity == 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE_RIGHT, 14);
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_REVERSE_RIGHT)
                && p->velocity != 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE, 0);
            }
            break;

        case M_STATE_REVERSE_LEFT:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_REVERSE_START;
            } else if (g_InputDB.slow) {
                if (p->gear != 0) {
                    p->gear--;
                    if (p->gear == 0) {
                        Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE_END, 0);
                        lara_item->current_anim_state = M_STATE_TURN_LEFT;
                        lara_item->goal_anim_state = M_STATE_TURN_LEFT;
                        break;
                    }
                }
            } else if (g_InputDB.sprint) {
                if (p->gear < 1) {
                    p->gear++;
                }
            } else if (g_Input.step_left || g_Input.left) {
                lara_item->goal_anim_state = M_STATE_REVERSE_LEFT;
            } else {
                lara_item->goal_anim_state = M_STATE_REVERSE;
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_REVERSE_WAIT)
                && p->velocity == 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE_LEFT, 14);
            }

            if (Lara_Vehicle_TestAnimEqual(M_ANIM_REVERSE_LEFT)
                && p->velocity != 0) {
                Lara_Vehicle_SwitchToAnim(M_ANIM_REVERSE_WAIT, 0);
            }
            break;

        case M_STATE_REVERSE_START:
            if (killed) {
                lara_item->goal_anim_state = M_STATE_STOP;
            }

            if (g_Input.jump && g_Input.left && p->velocity == 0
                && !m_DontExit) {
                if (M_CanDismount(item)) {
                    lara_item->goal_anim_state = M_STATE_DISMOUNT;
                }
            } else if (g_InputDB.slow) {
                if (p->gear != 0) {
                    p->gear--;
                    if (p->gear == 0) {
                        lara_item->goal_anim_state = M_STATE_STOP;
                    }
                }
            } else if (g_InputDB.sprint) {
                if (p->gear < 1) {
                    p->gear++;
                }
            } else if (g_Input.action && !g_Input.jump) {
                lara_item->goal_anim_state = M_STATE_REVERSE;
            } else if (g_Input.step_left || g_Input.left) {
                lara_item->goal_anim_state = M_STATE_REVERSE_LEFT;
            } else if (g_Input.step_right || g_Input.right) {
                lara_item->goal_anim_state = M_STATE_REVERSE_RIGHT;
            }
            break;
        }
    }

    const ROOM *const room = Room_Get(item->room_num);
    if (room->flags.underwater || room->flags.swamp) {
        lara_item->goal_anim_state = M_STATE_FALL;
        Lara_Kill();
        M_Explode(item);
    }
}

static void M_TriggerExhaustSmoke(
    const XYZ_32 pos, const int16_t angle, const int32_t speed,
    const bool moving)
{
    SPARK *const spark =
        Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION, SPARK_CONTEXT_SMOKE);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 0;
    spark->src_color.g = 0;
    spark->src_color.b = 0;

    if (moving) {
        spark->dst_color.r = (16 * speed) >> 5;
        spark->dst_color.g = (16 * speed) >> 5;
        spark->dst_color.b = (32 * speed) >> 5;
    } else {
        spark->dst_color.r = 16;
        spark->dst_color.g = 16;
        spark->dst_color.b = 32;
    }

    spark->col_fade_speed = 4;
    spark->fade_to_black = 4;
    spark->life = (Random_GetControl() & 3) - (speed >> 12) + 20;
    CLAMPL(spark->life, 9);
    spark->s_life = spark->life;

    spark->draw_type = DRAW_BLEND_ADD;
    spark->pos = (XYZ_32) {
        .x = pos.x + (Random_GetControl() & 0xF) - 8,
        .y = pos.y + (Random_GetControl() & 0xF) - 8,
        .z = pos.z + (Random_GetControl() & 0xF) - 8,
    };
    const XYZ_32 dir = XYZ_32_RotateYaw((XYZ_32) { .z = speed }, angle);
    spark->vel = (XYZ_32) {
        .x = -128 + (Random_GetControl() & 0xFF) + (dir.x >> 2),
        .y = -8 - (Random_GetControl() & 7),
        .z = -128 + (Random_GetControl() & 0xFF) + (dir.z >> 2),
    };
    spark->friction = 4;

    if ((Random_GetControl() & 1) != 0) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -24 - (Random_GetControl() & 7);
        } else {
            spark->rot_add = 24 + (Random_GetControl() & 7);
        }
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->scalar = 1;
    spark->gravity = -4 - (Random_GetControl() & 3);
    spark->max_y_vel = -8 - (Random_GetControl() & 7);
    spark->dst_size.width = (Random_GetControl() & 7) + (speed >> 7) + 32;
    spark->src_size.width = spark->dst_size.width >> 1;
    spark->size.width = spark->dst_size.width >> 1;
    spark->dst_size.height = spark->dst_size.width;
    spark->src_size.height = spark->dst_size.height >> 1;
    spark->size.height = spark->dst_size.height >> 1;
    Sparks_FinishSetup(spark);
}

static bool M_CheckDismount()
{
    ITEM *const lara_item = Lara_GetItem();
    if (lara_item->current_anim_state != M_STATE_DISMOUNT
        || !Item_TestFrameEqual(lara_item, -1)) {
        return false;
    }

    Lara_Vehicle_Dismount();
    Lara_GetLaraInfo()->gun_status = LGS_ARMLESS;
    lara_item->rot.y += DEG_90;
    lara_item->pos =
        XYZ_32_OffsetYaw(lara_item->pos, lara_item->rot.y, -STEP_L * 2);

    return true;
}

static void M_ResetAmbience(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->ambience != MX_INACTIVE) {
        Music_PlayBySlot(p->ambience, MPM_LOOP);
    }
}

static int32_t M_DoDynamics(
    const int32_t height, int32_t fall_speed, int32_t *const y_pos)
{
    if (height <= *y_pos) {
        int32_t bounce = height - *y_pos;
        CLAMPL(bounce, -80);
        fall_speed += (bounce - fall_speed) >> 4;
        CLAMPG(*y_pos, height);
    } else {
        *y_pos += fall_speed;

        if (*y_pos <= height - 32) {
            fall_speed += 9;
        } else {
            *y_pos = height;
            if (fall_speed > 150) {
                Lara_TakeDamage(fall_speed - 150, false);
            }
            fall_speed = 0;
        }
    }

    return fall_speed;
}

static int32_t M_UserControl(
    ITEM *const item, const int32_t height, int32_t *const pitch)
{
    ITEM *const lara_item = Lara_GetItem();
    const bool killed = lara_item->hit_points <= 0;
    if (lara_item->current_anim_state == M_STATE_DISMOUNT
        || lara_item->goal_anim_state == M_STATE_DISMOUNT) {
        g_Input = (INPUT_STATE) {};
    }

    M_PRIV *const p = item->priv;
    if (p->acceleration > 16) {
        p->velocity += p->acceleration >> 4;
        p->acceleration = p->acceleration - (p->acceleration >> 3);
    } else {
        p->acceleration = 0;
    }

    if (item->pos.y >= height - STEP_L) {
        if (p->velocity == 0 && g_Input.look) {
            Lara_Look_UpDown();
        }

        int32_t vel = ABS(p->velocity);
        int32_t max_turn;
        int32_t turn;
        if (vel > M_HIGH_SPEED) {
            max_turn = M_MAX_TURN;
            turn = M_HIGH_TURN_RATE;
        } else {
            max_turn = (M_MAX_TURN * vel) >> 14;
            turn = ((60 * vel) >> 14) + DEG_1;
        }

        if (p->velocity > 0) {
            if (g_Input.step_left || g_Input.left) {
                p->turn_rate -= turn;
                CLAMPL(p->turn_rate, -max_turn);
            } else if (g_Input.step_right || g_Input.right) {
                p->turn_rate += turn;
                CLAMPG(p->turn_rate, max_turn);
            }
        } else if (p->velocity < 0) {
            if (g_Input.step_left || g_Input.left) {
                p->turn_rate += turn;
                CLAMPG(p->turn_rate, max_turn);
            } else if (g_Input.step_right || g_Input.right) {
                p->turn_rate -= turn;
                CLAMPL(p->turn_rate, -max_turn);
            }
        }

        if (g_Input.jump && !killed) {
            if (p->velocity > 0) {
                p->velocity -= M_MAX_DECEL;
                CLAMPL(p->velocity, 0);
            } else if (p->velocity < 0) {
                p->velocity += M_MAX_DECEL;
                CLAMPG(p->velocity, 0);
            }
        } else if (g_Input.action && !killed) {
            if (p->gear == 0) {
                if (p->velocity >= M_MAX_SPEED) {
                    p->velocity = M_MAX_SPEED;
                } else if (p->velocity < M_HIGH_SPEED) {
                    p->velocity += ((M_LOW_ACCEL - p->velocity) >> 3) + 8;
                } else if (p->velocity >= M_MID_ACCEL) {
                    p->velocity += ((M_MAX_SPEED - p->velocity) >> 3) + 2;
                } else {
                    p->velocity += ((M_HIGH_ACCEL - p->velocity) >> 4) + 4;
                }
            } else if (p->gear == 1) {
                if (p->velocity <= M_MIN_SPEED) {
                    p->velocity = M_MIN_SPEED;
                } else {
                    p->velocity -= (ABS(M_MIN_SPEED - p->velocity) >> 3) - 2;
                }
            }

            p->velocity -= ABS(item->rot.y - p->move_angle) >> 6;
        }

        if (!g_Input.action || killed) {
            if (p->velocity > M_MIN_DECEL) {
                p->velocity -= M_MIN_DECEL;
            } else if (p->velocity < -M_MIN_DECEL) {
                p->velocity += M_MIN_DECEL;
            } else {
                p->velocity = 0;
            }
        }

        item->speed = p->velocity >> 8;
        vel = p->velocity;
        if (vel < 0) {
            vel >>= 1;
        }

        if (p->pitch_1 > 0xC000) {
            p->pitch_1 = (Random_GetControl() & 0x1FF) + 0xBF00;
        }
        p->pitch_1 += (ABS(vel) - p->pitch_1) >> 3;
    } else if (p->pitch_1 < 0xFFFF) {
        p->pitch_1 += (0xFFFF - p->pitch_1) >> 3;
    }

    if (g_Input.jump && !killed) {
        XYZ_32 pos = m_BrakeLight.pos;
        Collide_GetJointAbsPosition(item, &pos, m_BrakeLight.mesh_num);
        Output_AddDynamicLightRGB(pos, 10, M_BRAKE_GLOW);
        item->mesh_bits = M_BRAKE_ON_BITS;
    } else {
        item->mesh_bits = M_BRAKE_OFF_BITS;
    }

    *pitch = p->pitch_1;
    return 0;
}

static void M_CheckObjectCollision(ITEM *const item, ITEM *const jeep)
{
    if (!item->is_collidable || !item->is_visible || item == Lara_GetItem()
        || item == jeep) {
        return;
    }

    const bool is_enemy_jeep = false; // TODO: O_ENEMY_JEEP
    if (is_enemy_jeep) {
        COLL_INFO coll = {
            .radius = 400,
            .enable_baddie_push = true,
        };
        Object_Collision(Item_GetIndex(item), jeep, &coll);
        return;
    }

    const OBJECT *const obj = Object_Get(item->object_id);

    if (obj->collision_func != nullptr && obj->intelligent
        && Item_TestBoundsCollide(item, jeep, M_WIDTH)) {
        if (Item_ShouldSpawnBlood(item)) {
            Spawn_BloodBath(
                item->pos.x, jeep->pos.y - STEP_L, item->pos.z,
                (Random_GetControl() & 3) + 8, jeep->rot.y, item->room_num, 3);
        }
        Item_TakeFatalDamage(item, jeep);
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (g_Config.debug.enable_invulnerability || lara_item->hit_points <= 0) {
        return;
    }

    if (item->object_id == O_BOUNCING_BOULDER
        && Lara_TestBoundsCollide(item, M_RADIUS)) {
        Spawn_BloodBath(
            lara_item->pos.x, lara_item->pos.y - STEP_L * 2, lara_item->pos.z,
            (Random_GetControl() & 3) + 8, lara_item->rot.y,
            lara_item->room_num, 5);
        Lara_TakeDamage(8, true);
        return;
    }

    if (item->object_id == O_SCALED_SPIKES
        && ScaledSpikes_TestCollision(item)) {
        M_PRIV *const p = jeep->priv;
        p->flags |= 0x40;
    }
}

static void M_ObjectCollision(ITEM *const jeep)
{
    int16_t coll_rooms[M_COLL_ROOMS];
    const int32_t room_count =
        Room_GetAdjoiningRooms(jeep->room_num, coll_rooms, M_COLL_ROOMS);

    for (int32_t i = 0; i < room_count; i++) {
        const ROOM *const room = Room_Get(coll_rooms[i]);
        int16_t item_num = room->item_num;
        while (item_num != NO_ITEM) {
            ITEM *const item = Item_Get(item_num);
            const int16_t next_item_num = item->next_item;
            M_CheckObjectCollision(item, jeep);
            item_num = next_item_num;
        }
    }
}

static void M_Static3DCollision(const ITEM *const jeep, const int32_t height)
{
    const BOUNDS_32 jeep_bounds = {
        .min.x = jeep->pos.x - STEP_L,
        .max.x = jeep->pos.x + STEP_L,
        .min.y = jeep->pos.y - height,
        .max.y = jeep->pos.y,
        .min.z = jeep->pos.z - STEP_L,
        .max.z = jeep->pos.z + STEP_L,
    };

    int16_t coll_rooms[M_COLL_ROOMS];
    const int32_t room_count =
        Room_GetAdjoiningRooms(jeep->room_num, coll_rooms, M_COLL_ROOMS);

    for (int32_t i = 0; i < room_count; i++) {
        const ROOM *const room = Room_Get(coll_rooms[i]);
        for (int32_t j = 0; j < room->num_static_meshes; j++) {
            STATIC_MESH *const mesh = &room->static_meshes[j];
            // TODO: update once shatterable statics are added; base game
            // contains none in the same levels as the jeep.
            // If mesh->shattered, continue.
            if (mesh->static_num < 50 || mesh->static_num > 59) {
                continue;
            }

            const STATIC_OBJECT_3D *const obj =
                Object_Get3DStatic(mesh->static_num);
            if (!obj->collidable) {
                continue;
            }

            BOUNDS_32 mesh_bounds = {
                .min.y = mesh->pos.y + obj->collision_bounds.min.y,
                .max.y = mesh->pos.y + obj->collision_bounds.max.y,
            };
            switch (mesh->rot.y) {
            case -DEG_180:
                mesh_bounds.min.x = mesh->pos.x - obj->collision_bounds.max.x;
                mesh_bounds.max.x = mesh->pos.x - obj->collision_bounds.min.x;
                mesh_bounds.min.z = mesh->pos.z - obj->collision_bounds.max.z;
                mesh_bounds.max.z = mesh->pos.z - obj->collision_bounds.min.z;
                break;
            case -DEG_90:
                mesh_bounds.min.x = mesh->pos.x - obj->collision_bounds.max.z;
                mesh_bounds.max.x = mesh->pos.x - obj->collision_bounds.min.z;
                mesh_bounds.min.z = mesh->pos.z + obj->collision_bounds.min.x;
                mesh_bounds.max.z = mesh->pos.z + obj->collision_bounds.max.x;
                break;
            case DEG_90:
                mesh_bounds.min.x = mesh->pos.x + obj->collision_bounds.min.z;
                mesh_bounds.max.x = mesh->pos.x + obj->collision_bounds.max.z;
                mesh_bounds.min.z = mesh->pos.z - obj->collision_bounds.max.x;
                mesh_bounds.max.z = mesh->pos.z - obj->collision_bounds.min.x;
                break;
            default:
                mesh_bounds.min.x = mesh->pos.x + obj->collision_bounds.min.x;
                mesh_bounds.max.x = mesh->pos.x + obj->collision_bounds.max.x;
                mesh_bounds.min.z = mesh->pos.z + obj->collision_bounds.min.z;
                mesh_bounds.max.z = mesh->pos.z + obj->collision_bounds.max.z;
                break;
            }

            if (!Bounds32_Intersect(&jeep_bounds, &mesh_bounds)) {
                continue;
            }

            FX_Debris_ShatterStatic(mesh, -128, coll_rooms[i], 0);
            Sound_Effect(SFX_HIT_ROCK, &jeep->pos, SPM_NORMAL);
            // TODO: mesh->shattered = true;
        }
    }
}

static int32_t M_Dynamics(ITEM *const item)
{
    m_DontExit = false;

    XYZ_32 fl_pos_1 = {};
    XYZ_32 fr_pos_1 = {};
    XYZ_32 bl_pos_1 = {};
    XYZ_32 br_pos_1 = {};
    XYZ_32 fm_pos_1 = {};

    // clang-format off
    const int32_t fl_height_1 = M_TestHeight(item, +M_WIDTH, -STEP_L, &fl_pos_1);
    const int32_t fr_height_1 = M_TestHeight(item, +M_WIDTH, +STEP_L, &fr_pos_1);
    const int32_t bl_height_1 = M_TestHeight(item, -M_DEPTH, -STEP_L, &bl_pos_1);
    const int32_t br_height_1 = M_TestHeight(item, -M_DEPTH, +STEP_L, &br_pos_1);
    const int32_t fm_height_1 = M_TestHeight(item, -M_DEPTH,       0, &fm_pos_1);
    // clang-format on

    CLAMPG(fl_pos_1.y, fl_height_1);
    CLAMPG(fr_pos_1.y, fr_height_1);
    CLAMPG(bl_pos_1.y, bl_height_1);
    CLAMPG(br_pos_1.y, br_height_1);
    CLAMPG(fm_pos_1.y, fm_height_1);

    XYZ_32 old_pos = item->pos;
    M_PRIV *const p = item->priv;

    if (item->pos.y <= item->floor - 8) {
        if (p->turn_rate < -M_LOW_TURN_RATE) {
            p->turn_rate += M_LOW_TURN_RATE;
        } else if (p->turn_rate > M_LOW_TURN_RATE) {
            p->turn_rate -= M_LOW_TURN_RATE;
        } else {
            p->turn_rate = 0;
        }

        item->rot.y += p->turn_rate + p->extra_rotation;
        p->move_angle += (int16_t)(item->rot.y - p->move_angle) >> 5;
    } else {
        if (p->turn_rate < -DEG_1) {
            p->turn_rate += DEG_1;
        } else if (p->turn_rate > DEG_1) {
            p->turn_rate -= DEG_1;
        } else {
            p->turn_rate = 0;
        }

        item->rot.y += p->turn_rate + p->extra_rotation;
        const int16_t angle = item->rot.y - p->move_angle;
        int16_t vel = 728 - ((3 * p->velocity) >> 11);

        if (!g_Input.action && p->velocity > 0) {
            vel -= vel >> 2;
        }

        if (angle < -M_TURN_1) {
            if (angle < -M_TURN_2) {
                item->pos.y -= 41;
                item->fall_speed = -6 - (Random_GetControl() & 3);
                p->turn_rate = 0;
                p->velocity -= p->velocity >> 3;
            }

            if (angle < -M_TURN_3) {
                p->move_angle = item->rot.y + M_TURN_3;
            } else {
                p->move_angle -= vel;
            }
        } else if (angle > M_TURN_1) {
            if (angle > M_TURN_2) {
                item->pos.y -= 41;
                item->fall_speed = -6 - (Random_GetControl() & 3);
                p->turn_rate = 0;
                p->velocity -= p->velocity >> 3;
            }

            if (angle > M_TURN_3) {
                p->move_angle = item->rot.y - M_TURN_3;
            } else {
                p->move_angle += vel;
            }
        } else {
            p->move_angle = item->rot.y;
        }
    }

    int16_t room_num = item->room_num;
    const SECTOR *sector = Room_GetSector(item->pos, &room_num);
    int32_t height = Room_GetHeight(sector, item->pos);

    int32_t speed;
    if (item->pos.y < height) {
        speed = item->speed;
    } else {
        speed = (item->speed * Math_Cos(item->rot.x)) >> W2V_SHIFT;
    }
    item->pos = XYZ_32_OffsetYaw(item->pos, p->move_angle, speed);

    if (item->pos.y >= height) {
        int16_t angle = (100 * Math_Sin(item->rot.x)) >> W2V_SHIFT;
        if (ABS(angle) > 16) {
            m_DontExit = true;
            if (angle < 0) {
                p->velocity += SQUARE(angle + 16) >> 1;
            } else {
                p->velocity -= SQUARE(angle - 16) >> 1;
            }
        }

        angle = (128 * Math_Sin(item->rot.z)) >> W2V_SHIFT;
        if (ABS(angle) > 32) {
            m_DontExit = true;
            const int16_t angle_90 =
                item->rot.y + (angle < 0 ? -DEG_90 : DEG_90);
            item->pos = XYZ_32_OffsetYaw(item->pos, angle_90, ABS(angle) - 24);
        }
    }

    CLAMP(p->velocity, M_MIN_SPEED, M_MAX_SPEED);

    XYZ_32 new_pos = item->pos;
    if (!item->trigger.spent) {
        M_ObjectCollision(item);
        M_Static3DCollision(item, STEP_L * 2);
    }

    int32_t shift_1 = 0;
    int32_t shift_2 = 0;
    XYZ_32 fl_pos_2 = {};
    XYZ_32 bl_pos_2 = {};
    XYZ_32 fr_pos_2 = {};
    XYZ_32 fm_pos_2 = {};
    XYZ_32 br_pos_2 = {};

    const int32_t fl_height_2 = M_TestHeight(item, M_WIDTH, -STEP_L, &fl_pos_2);
    if (fl_height_2 < fl_pos_1.y - STEP_L) {
        shift_1 = ABS(Vehicle_DoShift(item, &fl_pos_2, &fl_pos_1) << 2);
    }

    const int32_t bl_height_2 =
        M_TestHeight(item, -M_DEPTH, -STEP_L, &bl_pos_2);
    if (bl_height_2 < bl_pos_1.y - STEP_L) {
        if (shift_1 != 0) {
            shift_1 += ABS(Vehicle_DoShift(item, &bl_pos_2, &bl_pos_1) << 2);
        } else {
            shift_1 = -ABS(Vehicle_DoShift(item, &bl_pos_2, &bl_pos_1) << 2);
        }
    }

    const int32_t fr_height_2 = M_TestHeight(item, M_WIDTH, STEP_L, &fr_pos_2);
    if (fr_height_2 < fr_pos_1.y - STEP_L) {
        shift_2 = -ABS(Vehicle_DoShift(item, &fr_pos_2, &fr_pos_1) << 2);
    }

    const int32_t fm_height_2 = M_TestHeight(item, -M_DEPTH, 0, &fm_pos_2);
    if (fm_height_2 < fm_pos_1.y - STEP_L) {
        Vehicle_DoShift(item, &fm_pos_2, &fm_pos_1);
    }

    const int32_t br_height_2 = M_TestHeight(item, -M_DEPTH, STEP_L, &br_pos_2);
    if (br_height_2 < br_pos_1.y - STEP_L) {
        if (shift_2) {
            shift_2 -= ABS(Vehicle_DoShift(item, &br_pos_2, &br_pos_1) << 2);
        } else {
            shift_2 = ABS(Vehicle_DoShift(item, &br_pos_2, &br_pos_1) << 2);
        }
    }

    if (shift_1 == 0) {
        shift_1 = shift_2;
    }

    room_num = item->room_num;
    sector = Room_GetSector(item->pos, &room_num);
    height = Room_GetHeight(sector, item->pos);

    if (height < item->pos.y - STEP_L) {
        Vehicle_DoShift(item, &item->pos, &old_pos);
    }

    if (p->velocity == 0) {
        shift_1 = 0;
    }

    p->rot_shift = (p->rot_shift + shift_1) >> 1;
    if (ABS(p->rot_shift) < 2) {
        p->rot_shift = 0;
    }
    if (ABS(p->rot_shift - p->extra_rotation) < 4) {
        p->extra_rotation = p->rot_shift;
    } else {
        p->extra_rotation += (p->rot_shift - p->extra_rotation) >> 2;
    }

    const int32_t anim = Vehicle_GetCollisionAnim(item, &new_pos);
    if (anim != 0) {
        const XYZ_32 delta = XYZ_32_Subtract(item->pos, old_pos);
        speed = XYZ_32_UnrotateYaw(delta, p->move_angle).z;
        speed <<= 8;

        if (item == Lara_Vehicle_GetItem() && p->velocity == M_MAX_SPEED
            && speed < 0x7FF6) {
            Lara_TakeDamage((M_MAX_SPEED - speed) >> 7, true);
        }

        if (p->velocity > 0 && speed < p->velocity) {
            p->velocity = MAX(speed, 0);
        } else if (p->velocity < 0 && speed > p->velocity) {
            p->velocity = MIN(speed, 0);
        }

        CLAMPL(p->velocity, M_MIN_SPEED);
    }

    return anim;
}

static bool M_Draw(const ITEM *const item)
{
    const bool drawn = Object_DrawAnimatingItem(item);
    if (drawn && Lara_Vehicle_GetItem() == item) {
        // TODO: draw speedometer
    }
    return drawn;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->is_usable_func = M_IsUsable;
    obj->event_func = Vehicle_HandleEvent;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_hitpoints = true;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 8)->rot.x = true;
    Object_GetBone(obj, 9)->rot.x = true;
    Object_GetBone(obj, 11)->rot.x = true;
    Object_GetBone(obj, 12)->rot.x = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "track_1", Music_IDToSlot(MX_JEEP_THEME),
            "Random music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_2", -1, "Random music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_3", -1, "Random music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_4", -1, "Random music track pool, slot 4. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "is_heavy", true,
            "Whether or not this vehicle can activate heavy triggers."),
        OBJECT_PROPERTY(
            M_PRIV, requires_key, true,
            "Whether or not Lara needs a key in order to use the jeep."));
}

void Jeep_Control(void)
{
    ITEM *const lara_item = Lara_GetItem();
    ITEM *const item = Lara_Vehicle_GetItem();
    M_PRIV *const p = item->priv;

    int32_t hit_wall = M_Dynamics(item);

    XYZ_32 fl_pos = {};
    XYZ_32 fr_pos = {};
    XYZ_32 fm_pos = {};

    // clang-format off
    const int32_t fl_height = M_TestHeight(item, +M_WIDTH, -STEP_L, &fl_pos);
    const int32_t fr_height = M_TestHeight(item, +M_WIDTH, +STEP_L, &fr_pos);
    const int32_t fm_height = M_TestHeight(item, -M_DEPTH,       0, &fm_pos);
    // clang-format on

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);

    Vehicle_TestTriggers(lara_item, item);

    const bool killed = lara_item->hit_points <= 0;
    if (killed) {
        g_Input.forward = false;
        g_Input.back = false;
        g_Input.left = false;
        g_Input.right = false;
        g_Input.step_left = false;
        g_Input.step_right = false;
    }

    int32_t driving = -1;
    int32_t pitch = 0;
    if (p->flags != 0) {
        hit_wall = 0;
    } else {
        if (lara_item->current_anim_state != M_STATE_MOUNT) {
            driving = M_UserControl(item, height, &pitch);
        } else {
            driving = -1;
            hit_wall = 0;
        }
    }

    if (p->velocity != 0 || p->acceleration != 0) {
        p->pitch_2 = pitch;
        CLAMP(p->pitch_2, M_MIN_PITCH, M_MAX_PITCH);
        Sound_Effect(
            SFX_JEEP_MOVE, &item->pos,
            (p->pitch_2 << 8) + (SPM_PITCH | 0x1000000));
    } else {
        if (driving != -1) {
            Sound_Effect(SFX_JEEP_IDLE, &item->pos, SPM_NORMAL);
        }
        p->pitch_2 = 0;
    }

    for (int32_t i = 0; i < 4; i++) {
        p->wheel_rots[i] -= p->velocity >> 2;
    }

    const int32_t old_y = item->pos.y;
    item->floor = height;
    item->fall_speed = M_DoDynamics(height, item->fall_speed, &item->pos.y);

    const XZ_16 tilt = M_GetTilt(
        item, old_y,
        &(M_TILT_ARGS) {
            .fl_pos = fl_pos,
            .fr_pos = fr_pos,
            .fm_pos = fm_pos,
            .fl_height = fl_height,
            .fr_height = fr_height,
            .fm_height = fm_height,
        });
    item->rot.x += (tilt.x - item->rot.x) >> 2;
    item->rot.z += (tilt.z - item->rot.z) >> 2;

    if ((p->flags & 0x80) == 0) {
        Item_UpdateRoom(Item_GetIndex(item), room_num);
        Item_UpdateRoom(Item_GetIndex(lara_item), room_num);

        lara_item->pos = item->pos;
        lara_item->rot = item->rot;
        M_Animate(item, hit_wall, killed);
        Item_Animate(lara_item);
        Lara_Vehicle_SyncItemAnim();

        if (p->gear == 0) {
            p->camera_angle -= p->camera_angle >> 3;
        } else if (p->gear == 1) {
            p->camera_angle += (M_CAM_TURN - p->camera_angle) >> 3;
        }
        g_Camera.target_elevation = M_CAM_ELEVATION;
        g_Camera.target_distance = M_CAM_DISTANCE;
        g_Camera.target_angle = p->camera_angle;

        if ((p->flags & 0x40) != 0 && item->pos.y == item->floor) {
            M_Explode(item);
            if (g_Config.debug.enable_invulnerability) {
                Lara_Vehicle_Dismount();
                Lara_GetLaraInfo()->gun_status = LGS_ARMLESS;
                Item_SwitchToAnim(lara_item, LA(LA_FREEFALL_LAND), 0);
            } else {
                Lara_Kill();
                lara_item->trigger.spent = true;
            }
            return;
        }
    }

    if (lara_item->current_anim_state == M_STATE_MOUNT
        || lara_item->current_anim_state == M_STATE_DISMOUNT) {
        m_ExhaustSmokeVel = 0;
    } else {
        XYZ_32 pos = m_Exhaust.pos;
        Collide_GetJointAbsPosition(item, &pos, m_Exhaust.mesh_num);

        if (item->speed > 32) {
            if (item->speed < 64) {
                M_TriggerExhaustSmoke(
                    pos, item->rot.y + DEG_180, 64 - item->speed, 1);
            }
        } else {
            int32_t smoke_vel = 0;
            if (m_ExhaustSmokeVel < 16) {
                smoke_vel =
                    ((Random_GetControl() & 7) + (Random_GetControl() & 0x10)
                     + 2 * m_ExhaustSmokeVel)
                    << 6;
                m_ExhaustSmokeVel++;
            } else if ((Random_GetControl() & 3) == 0) {
                smoke_vel =
                    ((Random_GetControl() & 0xF) + (Random_GetControl() & 0x10))
                    << 6;
            }

            M_TriggerExhaustSmoke(pos, item->rot.y + DEG_180, smoke_vel, 0);
        }
    }

    if (M_CheckDismount()) {
        M_ResetAmbience(item);
    }
}

REGISTER_OBJECT(O_JEEP, M_Setup)
