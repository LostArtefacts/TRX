#include <trx/game/objects/vehicles/mine_cart.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/camera.h>
#include <trx/game/const.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/flare.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vehicles/common.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>
// clang-format off
#define M_MOUNT_DIST          200000
#define M_DISMOUNT_DIST       330
#define M_MAX_COLL_ROOMS      12
#define M_MIN_BEAM_DAMAGE     20
#define M_MESH_FRAME          20
#define M_RADIUS              STEP_L
#define M_ANGLE_CLAMP         (DEG_90 - 1)     // = 16383
#define M_MIN_VELOCITY        (STEP_L / 8)     // = 32
#define M_MAX_VELOCITY        (STEP_L * 63)    // = 16128
#define M_MIN_SPEED           (STEP_L * 10)    // = 2560
#define M_BRAKE_SPEED         (STEP_L * 6)     // = 1536
#define M_STOP_SPEED          (WALL_L * 60)    // = 61440
#define M_GRAVITY             (WALL_L + 1)     // = 1025
#define M_TARGET_DIST         (WALL_L * 2)     // = 2048
#define M_MAX_GRADIENT        (STEP_L / 2)     // = 128
#define M_MAX_ROLL            (DEG_90 / 4)     // = 4096
#define M_TURN_SHIFT          (DEG_90 / 4)     // = 4096
#define M_TURN_DIST           (STEP_L * 14)    // = 3584
#define M_JUMP_DIST           (STEP_L * 9 / 4) // = 576
#define M_JUMP_VELOCITY       (-WALL_L)        // = -1024
#define M_CAM_RIDE_ELEVATION  (-DEG_45)        // = -8190
#define M_CAM_RIDE_DISTANCE   (WALL_L * 2)     // = 2048
#define M_CAM_CRASH_ELEVATION (-DEG_1 * 25)    // = -4550
#define M_CAM_CRASH_DISTANCE  (WALL_L * 4)     // = 4096
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_MOUNT_LEFT       = 0,
    M_ANIM_PREPARE_RIDE     = 5,
    M_ANIM_SWIPE            = 6,
    M_ANIM_PREPARE_DISMOUNT = 7,
    M_ANIM_CRASH            = 23,
    M_ANIM_TOPPLED          = 30,
    M_ANIM_TOPPLE_START     = 31,
    M_ANIM_HIT_BEAM         = 34,
    M_ANIM_MOUNT_RIGHT      = 46,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_MOUNT,
    M_STATE_STOP,
    M_STATE_DISMOUNT_LEFT,
    M_STATE_DISMOUNT_RIGHT,
    M_STATE_IDLE,
    M_STATE_DUCK,
    M_STATE_RIDE,
    M_STATE_RIGHT,
    M_STATE_HUG_RIGHT,
    M_STATE_LEFT,
    M_STATE_HUG_LEFT,
    M_STATE_BRAKE,
    M_STATE_TILT_FORWARD,
    M_STATE_TILT_BACK,
    M_STATE_DEATH,
    M_STATE_CRASH_START,
    M_STATE_CRASH,
    M_STATE_HIT_BEAM,
    M_STATE_SWIPE,
    M_STATE_BRAKING,
} M_STATE;

typedef enum {
    M_SIDE_NONE,
    M_SIDE_LEFT,
    M_SIDE_RIGHT,
} M_SIDE;

typedef struct {
    int32_t speed;
    int32_t mid_pos;
    int32_t front_pos;
    int16_t y_velocity;
    int16_t gradient;
    uint8_t stop_delay;
    M_SIDE dismount_side;
    struct {
        XZ_32 pos;
        int16_t angle;
        int16_t length;
        M_SIDE side;
    } turn;
    struct {
        bool control;
        bool dead;
        bool stopped;
        bool suppress_anim;
    } flags;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "speed", &p->speed));
    MUST(JSON_READ_OPT(io, "mid_pos", &p->mid_pos));
    MUST(JSON_READ_OPT(io, "front_pos", &p->front_pos));
    MUST(JSON_READ_OPT(io, "y_velocity", &p->y_velocity));
    MUST(JSON_READ_OPT(io, "gradient", &p->gradient));
    MUST(JSON_READ_OPT(io, "stop_delay", &p->stop_delay));
    if (SHOULD(JSON_PUSH(io, "turn"))) {
        MUST(JSON_READ_OPT(io, "pos.x", &p->turn.pos.x));
        MUST(JSON_READ_OPT(io, "pos.z", &p->turn.pos.z));
        MUST(JSON_READ_OPT(io, "angle", &p->turn.angle));
        MUST(JSON_READ_OPT(io, "length", &p->turn.length));
        MUST(JSON_READ_OPT(io, "side", &p->turn.side));
        MUST(JSON_POP(io));
    }
    if (SHOULD(JSON_PUSH(io, "flags"))) {
        MUST(JSON_READ_OPT(io, "control", &p->flags.control));
        MUST(JSON_READ_OPT(io, "dead", &p->flags.dead));
        MUST(JSON_READ_OPT(io, "stopped", &p->flags.stopped));
        MUST(JSON_READ_OPT(io, "suppress_anim", &p->flags.suppress_anim));
        MUST(JSON_POP(io));
    }
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "speed", p->speed);
    JSONW_WRITE(io, "mid_pos", p->mid_pos);
    JSONW_WRITE(io, "front_pos", p->front_pos);
    JSONW_WRITE(io, "y_velocity", p->y_velocity);
    JSONW_WRITE(io, "gradient", p->gradient);
    JSONW_WRITE(io, "stop_delay", p->stop_delay);
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "pos.x", p->turn.pos.x);
    JSONW_WRITE(io, "pos.z", p->turn.pos.z);
    JSONW_WRITE(io, "angle", p->turn.angle);
    JSONW_WRITE(io, "length", p->turn.length);
    JSONW_WRITE(io, "side", p->turn.side);
    JSONW_POP_AND_SET(io, "turn");
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "control", p->flags.control);
    JSONW_WRITE(io, "dead", p->flags.dead);
    JSONW_WRITE(io, "stopped", p->flags.stopped);
    JSONW_WRITE(io, "suppress_anim", p->flags.suppress_anim);
    JSONW_POP_AND_SET(io, "flags");
}

static M_SIDE M_CheckMount(ITEM *const item, COLL_INFO *const coll)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS
        || lara_item->gravity) {
        return M_SIDE_NONE;
    }

    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return M_SIDE_NONE;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return M_SIDE_NONE;
    }

    const XYZ_32 delta = {
        .x = lara_item->pos.x - item->pos.x,
        .z = lara_item->pos.z - item->pos.z,
        .y = 0,
    };
    if (XYZ_32_GetLength2(delta) > M_MOUNT_DIST) {
        return M_SIDE_NONE;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);
    if (height < -MAX_HEIGHT) {
        return M_SIDE_NONE;
    }

    const int16_t angle =
        (int16_t)Math_Atan(
            lara_item->pos.z - item->pos.z, lara_item->pos.x - item->pos.x)
        - item->rot.y;
    return angle > -0x1FFE && angle < 0x5FFA ? M_SIDE_RIGHT : M_SIDE_LEFT;
}

static bool M_CheckDismount(const M_SIDE side)
{
    const ITEM *const item = Lara_Vehicle_GetItem();

    const int16_t rot = item->rot.y + (side == M_SIDE_LEFT ? DEG_90 : -DEG_90);
    const XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, rot, M_DISMOUNT_DIST);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const HEIGHT_TYPE height_type = Room_GetHeightType();

    if (height_type == HT_BIG_SLOPE || height_type == HT_DIAGONAL
        || height == NO_HEIGHT) {
        return false;
    }

    if (ABS(height - item->pos.y) > WALL_L / 2) {
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

    ITEM *const item = Item_Get(item_num);
    const M_SIDE mount_side = M_CheckMount(item, coll);
    if (mount_side == M_SIDE_NONE) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    Lara_Vehicle_SetIndex(item_num);
    if (Gun_IsFlareType(lara->gun_type)) {
        Gun_Flare_Dispose(false);
        lara->gun_type = LGT_UNARMED;
        lara->request_gun_type = LGT_UNARMED;
    }

    const M_ANIM anim_idx =
        mount_side == M_SIDE_LEFT ? M_ANIM_MOUNT_LEFT : M_ANIM_MOUNT_RIGHT;
    Lara_Vehicle_SwitchToAnim(anim_idx, 0);
    lara_item->current_anim_state = M_STATE_MOUNT;
    lara_item->goal_anim_state = M_STATE_MOUNT;
    lara_item->pos = item->pos;
    lara_item->rot = item->rot;
    lara->gun_status = LGS_HANDS_BUSY;
    lara->hit_direction = DIR_UNKNOWN;

    M_PRIV *const p = item->priv;
    p->speed = 0;
    p->y_velocity = 0;
    p->gradient = 0;
    p->turn.side = M_SIDE_NONE;
    p->dismount_side = M_SIDE_NONE;
    p->flags.control = false;
    p->flags.dead = false;
    p->flags.stopped = false;
    p->flags.suppress_anim = false;

    Vehicle_PlayTrackPool(item, "track", MPM_ONCE);
}

static void M_CheckStrikeSwitch(ITEM *const item)
{
    if (Item_IsInPlay(item)) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item->current_anim_state != M_STATE_SWIPE
        || !Lara_Vehicle_TestAnimEqual(M_ANIM_SWIPE)
        || !Item_TestFrameRange(lara_item, 12, 22)) {
        return;
    }

    Sound_Effect(SFX_SPANNER_CLUNK, &item->pos, SPM_ALWAYS);
    Room_TestTriggers(item);
    Item_AddSimulated(Item_GetIndex(item));
    item->trigger = (ITEM_TRIGGER_STATE) { .mask = TRIGGER_MASK_ALL };
}

static void M_CheckObjectCollision(ITEM *const item, ITEM *const cart)
{
    if (!item->is_collidable || !item->is_visible || item == Lara_GetItem()
        || item == cart) {
        return;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    const bool is_flip_switch = item->object_id == O_ANIMATING_2;
    if (obj->collision_func == nullptr
        || (!obj->intelligent && !is_flip_switch)) {
        return;
    }

    if (!Item_IsNearby(item, cart, M_TARGET_DIST)) {
        return;
    }

    if (!Item_TestBoundsCollide(item, cart, M_RADIUS)) {
        return;
    }

    if (is_flip_switch) {
        M_CheckStrikeSwitch(item);
        return;
    }

    if (Item_ShouldSpawnBlood(item)) {
        Spawn_BloodBath(
            item->pos.x, cart->pos.y - STEP_L, item->pos.z, cart->speed,
            cart->rot.y, item->room_num, 3);
    }
    if (item->hit_points > 0) {
        Item_TakeFatalDamage(item, cart);
    }
}

static void M_ObjectCollision(ITEM *const cart)
{
    int16_t roomies[M_MAX_COLL_ROOMS];
    const int32_t roomies_count =
        Room_GetAdjoiningRooms(cart->room_num, roomies, M_MAX_COLL_ROOMS);

    for (int32_t i = 0; i < roomies_count; i++) {
        const ROOM *const room = Room_Get(roomies[i]);
        int16_t item_num = room->item_num;
        while (item_num != NO_ITEM) {
            ITEM *const item = Item_Get(item_num);
            const int16_t next_item_num = item->next_item;
            M_CheckObjectCollision(item, cart);
            item_num = next_item_num;
        }
    }
}

static int32_t M_GetCollision(
    ITEM *const item, const int16_t angle, const int32_t distance,
    int32_t *const ceiling)
{
    XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, angle, distance);
    pos.y -= LARA_HEIGHT;
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    *ceiling = Room_GetCeiling(sector, pos);

    return height == NO_HEIGHT ? NO_HEIGHT : (height - item->pos.y);
}

static int32_t M_GetHeight(ITEM *const item, const int32_t x, const int32_t z)
{
    XYZ_32 pos = XYZ_32_OffsetLocalYaw(
        item->pos, (XYZ_32) { .x = x, .z = z }, item->rot.y);

    // The height a wheel sits at follows the cart's pitch and roll, which the
    // yaw turn above says nothing about.
    pos.y = (item->pos.y + ((x * Math_Sin(item->rot.z)) >> W2V_SHIFT))
        - ((z * Math_Sin(item->rot.x)) >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    return Room_GetHeight(sector, pos);
}

static void M_UserControl(ITEM *const item)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    M_PRIV *const p = item->priv;

    switch (lara_item->current_anim_state) {
    case M_STATE_MOUNT:
        if (Lara_Vehicle_TestAnimEqual(M_ANIM_PREPARE_RIDE)
            && Item_TestFrameEqual(lara_item, M_MESH_FRAME)) {
            Lara_Skin_SetExtraEquipment(LM_HAND_R, EXTRA_MESH_SPANNER);
        }
        break;

    case M_STATE_STOP:
        if (Lara_Vehicle_TestAnimEqual(M_ANIM_PREPARE_DISMOUNT)) {
            if (Item_TestFrameEqual(lara_item, M_MESH_FRAME)) {
                Lara_Skin_ClearEquipment(LM_HAND_R);
            }

            if (p->dismount_side == M_SIDE_RIGHT) {
                lara_item->goal_anim_state = M_STATE_DISMOUNT_RIGHT;
            } else {
                lara_item->goal_anim_state = M_STATE_DISMOUNT_LEFT;
            }
        }
        break;

    case M_STATE_DISMOUNT_LEFT:
    case M_STATE_DISMOUNT_RIGHT:
        if (Item_TestFrameEqual(lara_item, -1)) {
            XYZ_32 pos = {
                .x = 0,
                .y = 640,
                .z = 0,
            };
            Lara_GetMeshPos(LM_HIPS, &pos);
            lara_item->pos = pos;
            lara_item->rot.y = item->rot.y;
            if (lara_item->current_anim_state == M_STATE_DISMOUNT_LEFT) {
                lara_item->rot.y += DEG_90;
            } else {
                lara_item->rot.y -= DEG_90;
            }

            Lara_Vehicle_Dismount();
            lara->gun_status = LGS_ARMLESS;
        }
        break;

    case M_STATE_IDLE:
        if (!p->flags.control) {
            Sound_Effect(SFX_MINE_CART_CLUNK_START, &item->pos, SPM_ALWAYS);
            p->stop_delay = 64;
            p->flags.control = true;
        }

        if (g_Input.roll && p->flags.stopped) {
            if (g_Input.left && M_CheckDismount(M_SIDE_LEFT)) {
                lara_item->goal_anim_state = M_STATE_STOP;
                p->dismount_side = M_SIDE_LEFT;
            } else if (g_Input.right && M_CheckDismount(M_SIDE_RIGHT)) {
                lara_item->goal_anim_state = M_STATE_STOP;
                p->dismount_side = M_SIDE_RIGHT;
            }
        }
        if (g_Input.crouch) {
            lara_item->goal_anim_state = M_STATE_DUCK;
        } else if (p->speed > M_MIN_VELOCITY) {
            lara_item->goal_anim_state = M_STATE_RIDE;
        }

        break;

    case M_STATE_DUCK:
        if (g_Input.action) {
            lara_item->goal_anim_state = M_STATE_SWIPE;
        } else if (g_Input.jump) {
            lara_item->goal_anim_state = M_STATE_BRAKE;
        } else if (!g_Input.crouch) {
            lara_item->goal_anim_state = M_STATE_IDLE;
        }
        break;

    case M_STATE_RIDE:
        if (g_Input.action) {
            lara_item->goal_anim_state = M_STATE_SWIPE;
        } else if (g_Input.crouch) {
            lara_item->goal_anim_state = M_STATE_DUCK;
        } else if (g_Input.jump) {
            lara_item->goal_anim_state = M_STATE_BRAKE;
        } else if (p->speed == M_MIN_VELOCITY || p->flags.stopped) {
            lara_item->goal_anim_state = M_STATE_IDLE;
        } else if (p->gradient < -M_MAX_GRADIENT) {
            lara_item->goal_anim_state = M_STATE_TILT_FORWARD;
        } else if (p->gradient > M_MAX_GRADIENT) {
            lara_item->goal_anim_state = M_STATE_TILT_BACK;
        } else if (g_Input.left) {
            lara_item->goal_anim_state = M_STATE_LEFT;
        } else if (g_Input.right) {
            lara_item->goal_anim_state = M_STATE_RIGHT;
        }
        break;

    case M_STATE_RIGHT:
        if (!g_Input.right) {
            lara_item->goal_anim_state = M_STATE_RIDE;
        } else if (g_Input.action) {
            lara_item->goal_anim_state = M_STATE_SWIPE;
        } else if (g_Input.crouch) {
            lara_item->goal_anim_state = M_STATE_DUCK;
        } else if (g_Input.jump) {
            lara_item->goal_anim_state = M_STATE_BRAKE;
        }
        break;

    case M_STATE_LEFT:
        if (!g_Input.left) {
            lara_item->goal_anim_state = M_STATE_RIDE;
        } else if (g_Input.action) {
            lara_item->goal_anim_state = M_STATE_SWIPE;
        } else if (g_Input.crouch) {
            lara_item->goal_anim_state = M_STATE_DUCK;
        } else if (g_Input.jump) {
            lara_item->goal_anim_state = M_STATE_BRAKE;
        }
        break;

    case M_STATE_BRAKE:
        lara_item->goal_anim_state = M_STATE_BRAKING;
        break;

    case M_STATE_TILT_FORWARD:
    case M_STATE_TILT_BACK:
        if (g_Input.action) {
            lara_item->goal_anim_state = M_STATE_SWIPE;
        } else if (g_Input.crouch) {
            lara_item->goal_anim_state = M_STATE_DUCK;
        } else if (g_Input.jump) {
            lara_item->goal_anim_state = M_STATE_BRAKE;
        } else {
            const bool forward =
                lara_item->current_anim_state == M_STATE_TILT_FORWARD;
            if ((forward && p->gradient > -M_MAX_GRADIENT)
                || (!forward && p->gradient < M_MAX_GRADIENT)) {
                lara_item->goal_anim_state = M_STATE_RIDE;
            }
        }
        break;

    case M_STATE_DEATH: {
        g_Camera.target_elevation = M_CAM_RIDE_ELEVATION;
        g_Camera.target_distance = M_CAM_RIDE_DISTANCE;
        int32_t ceiling = 0;
        const int32_t height =
            M_GetCollision(item, item->rot.y, STEP_L * 2, &ceiling);

        if (height > -STEP_L && height < STEP_L) {
            const int32_t time4 = Output_GetTimeInGame() * 4;
            if ((time4 & 7) == 0) {
                Sound_Effect(SFX_QUAD_FRONT_IMPACT, &item->pos, SPM_ALWAYS);
            }
            item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, STEP_L / 2);
        } else if (Lara_Vehicle_TestAnimEqual(M_ANIM_TOPPLED)) {
            p->flags.suppress_anim = true;
            Lara_Kill();
        }
        break;
    }

    case M_STATE_CRASH:
        g_Camera.target_elevation = M_CAM_CRASH_ELEVATION;
        g_Camera.target_distance = M_CAM_CRASH_DISTANCE;
        break;

    case M_STATE_HIT_BEAM:
        if (lara_item->hit_points <= 0 && Item_TestFrameEqual(lara_item, 28)) {
            p->flags.control = false;
            p->flags.suppress_anim = true;
            p->speed = 0;
            item->speed = 0;
        }
        break;

    case M_STATE_SWIPE:
        lara_item->goal_anim_state = M_STATE_RIDE;
        break;

    case M_STATE_BRAKING:
        if (g_Input.crouch) {
            lara_item->goal_anim_state = M_STATE_DUCK;
            Sound_StopEffect(SFX_MINE_CART_SREECH_BRAKE);
        } else if (!g_Input.jump || p->flags.stopped) {
            lara_item->goal_anim_state = M_STATE_RIDE;
            Sound_StopEffect(SFX_MINE_CART_SREECH_BRAKE);
        } else {
            p->speed -= M_BRAKE_SPEED;
            Sound_Effect(
                SFX_MINE_CART_SREECH_BRAKE, &lara_item->pos, SPM_ALWAYS);
        }
        break;

    default:
        break;
    }

    if (Lara_Vehicle_IsMounted() && !p->flags.suppress_anim) {
        Item_Animate(lara_item);
        Lara_Vehicle_SyncItemAnim();
    }

    if (lara_item->current_anim_state == M_STATE_DEATH
        || lara_item->current_anim_state == M_STATE_CRASH
        || lara_item->hit_points <= 0) {
        return;
    }

    if (item->rot.z > M_MAX_ROLL || item->rot.z < -M_MAX_ROLL) {
        Lara_Vehicle_SwitchToAnim(M_ANIM_TOPPLE_START, 0);
        lara_item->current_anim_state = M_STATE_DEATH;
        lara_item->goal_anim_state = M_STATE_DEATH;
        p->flags.control = false;
        p->flags.stopped = true;
        p->flags.dead = true;
        p->speed = 0;
        item->speed = 0;
        return;
    }

    int32_t ceiling = 0;
    const int32_t height =
        M_GetCollision(item, item->rot.y, STEP_L * 2, &ceiling);

    if (height < -STEP_L * 2) {
        Lara_Vehicle_SwitchToAnim(M_ANIM_CRASH, 0);
        lara_item->current_anim_state = M_STATE_CRASH;
        lara_item->goal_anim_state = M_STATE_CRASH;
        p->flags.control = false;
        p->flags.stopped = true;
        p->flags.dead = true;
        p->speed = 0;
        item->speed = 0;
        Lara_Kill();
        return;
    }

    if (lara_item->current_anim_state != M_STATE_DUCK
        && lara_item->current_anim_state != M_STATE_HIT_BEAM) {
        COLL_INFO coll = {
            .radius = 100,
            .quadrant = Math_GetDirection(item->rot.y),
        };
        if (Collide_CollideStaticObjects(
                &coll, item->pos, item->room_num, STEP_L * 3)) {
            Lara_Vehicle_SwitchToAnim(M_ANIM_HIT_BEAM, 0);
            lara_item->current_anim_state = M_STATE_HIT_BEAM;
            lara_item->goal_anim_state = M_STATE_HIT_BEAM;
            Spawn_BloodBath(
                lara_item->pos.x, lara_item->pos.y - STEP_L * 3,
                lara_item->pos.z, item->speed, item->rot.y, lara_item->room_num,
                3);

            int16_t damage = 25 * ((uint16_t)p->speed >> 11);
            CLAMPL(damage, M_MIN_BEAM_DAMAGE);
            Lara_TakeDamage(damage, false);
        }
    }

    if (height > M_JUMP_DIST && p->y_velocity == 0) {
        p->y_velocity = M_JUMP_VELOCITY;
    }

    M_ObjectCollision(item);
}

static MINE_CART_TYPE M_GetFloorType(const ITEM *const item)
{
    const XYZ_32 pos = {
        .x = item->pos.x,
        .y = MAX_HEIGHT,
        .z = item->pos.z,
    };
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    return sector->mine_cart_type;
}

static void M_Move(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (p->stop_delay != 0) {
        p->stop_delay--;
    }

#define L_STOP_POS(p) ((p & (STEP_L * 7 / 2)) == STEP_L * 2)

    const MINE_CART_TYPE floor_type = M_GetFloorType(item);
    if (floor_type == MINE_CART_STOP && p->stop_delay == 0
        && (L_STOP_POS(item->pos.x) || L_STOP_POS(item->pos.z))) {
        if (p->speed < M_STOP_SPEED) {
            p->flags.control = true;
            p->flags.stopped = true;
            p->speed = 0;
            item->speed = 0;
            return;
        }

        p->stop_delay = 16;
    }

#undef L_STOP_POS

    if ((floor_type == MINE_CART_LEFT || floor_type == MINE_CART_RIGHT)
        && p->stop_delay == 0 && p->turn.side == M_SIDE_NONE) {
        uint16_t rot = ((uint16_t)item->rot.y) >> W2V_SHIFT;
        if (floor_type == MINE_CART_LEFT) {
            rot |= 4;
        }

        switch (rot) {
        case 0:
            p->turn.pos.x = ROUND_TO_SECTOR(item->pos.x + M_TURN_SHIFT);
            p->turn.pos.z = ROUND_TO_SECTOR(item->pos.z);
            break;
        case 1:
            p->turn.pos.x = ROUND_TO_SECTOR(item->pos.x);
            p->turn.pos.z = ROUND_TO_SECTOR_END(item->pos.z - M_TURN_SHIFT);
            break;
        case 2:
            p->turn.pos.x = ROUND_TO_SECTOR_END(item->pos.x - M_TURN_SHIFT);
            p->turn.pos.z = ROUND_TO_SECTOR_END(item->pos.z);
            break;
        case 3:
            p->turn.pos.x = ROUND_TO_SECTOR_END(item->pos.x);
            p->turn.pos.z = ROUND_TO_SECTOR(item->pos.z + M_TURN_SHIFT);
            break;
        case 4:
            p->turn.pos.x = ROUND_TO_SECTOR_END(item->pos.x - M_TURN_SHIFT);
            p->turn.pos.z = ROUND_TO_SECTOR(item->pos.z);
            break;
        case 5:
            p->turn.pos.x = ROUND_TO_SECTOR(item->pos.x);
            p->turn.pos.z = ROUND_TO_SECTOR(item->pos.z + M_TURN_SHIFT);
            break;
        case 6:
            p->turn.pos.x = ROUND_TO_SECTOR(item->pos.x + M_TURN_SHIFT);
            p->turn.pos.z = ROUND_TO_SECTOR_END(item->pos.z);
            break;
        case 7:
            p->turn.pos.x = ROUND_TO_SECTOR_END(item->pos.x);
            p->turn.pos.z = ROUND_TO_SECTOR_END(item->pos.z - M_TURN_SHIFT);
            break;
        default:
            break;
        }

        int16_t angle =
            Math_Atan(item->pos.z - p->turn.pos.z, item->pos.x - p->turn.pos.x)
            & M_ANGLE_CLAMP;
        if (rot >= 4 && angle != 0) {
            angle = DEG_90 - angle;
        }

        p->turn.angle = item->rot.y;
        p->turn.length = angle;
        p->turn.side =
            floor_type == MINE_CART_LEFT ? M_SIDE_LEFT : M_SIDE_RIGHT;
    }

    CLAMPL(p->speed, M_MIN_SPEED);
    p->speed += -4 * p->gradient;
    item->speed = (int16_t)(p->speed >> 8);

    if (item->speed < M_MIN_VELOCITY) {
        item->speed = M_MIN_VELOCITY;
        Sound_StopEffect(SFX_MINE_CART_TRACK_LOOP);
        if (p->y_velocity != 0) {
            Sound_StopEffect(SFX_MINE_CART_PULLY_LOOP);
        } else {
            Sound_Effect(SFX_MINE_CART_PULLY_LOOP, &item->pos, SPM_ALWAYS);
        }
    } else {
        Sound_StopEffect(SFX_MINE_CART_PULLY_LOOP);
        if (p->y_velocity != 0) {
            Sound_StopEffect(SFX_MINE_CART_TRACK_LOOP);
        } else {
            Sound_Effect(
                SFX_MINE_CART_TRACK_LOOP, &item->pos,
                ((item->speed << 15) + 0x1000000) | SPM_PITCH | SPM_ALWAYS);
        }
    }

    if (p->turn.side != M_SIDE_NONE) {
        p->turn.length += 3 * item->speed;
        if (p->turn.length > (DEG_1 * 90)) {
            if (p->turn.side == M_SIDE_LEFT) {
                item->rot.y = p->turn.angle - DEG_90;
            } else {
                item->rot.y = p->turn.angle + DEG_90;
            }
            p->turn.side = M_SIDE_NONE;
        } else if (p->turn.side == M_SIDE_LEFT) {
            item->rot.y = p->turn.angle - p->turn.length;
        } else {
            item->rot.y = p->turn.angle + p->turn.length;
        }

        if (p->turn.side != M_SIDE_NONE) {
            // The cart swings around a point off to one side, so the shift
            // is a unit vector out to its left, at the scale the trig table
            // works in. The original took the quadrant apart to get it.
            XYZ_32 shift = XYZ_32_RotateYaw(
                (XYZ_32) { .x = -(1 << W2V_SHIFT) }, item->rot.y);
            if (p->turn.side == M_SIDE_LEFT) {
                shift.x = -shift.x;
                shift.z = -shift.z;
            }

            item->pos.x =
                p->turn.pos.x + ((M_TURN_DIST * shift.x) >> W2V_SHIFT);
            item->pos.z =
                p->turn.pos.z + ((M_TURN_DIST * shift.z) >> W2V_SHIFT);
        }
    } else {
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, item->speed);
    }

    p->mid_pos = M_GetHeight(item, 0, 0);

    if (p->y_velocity == 0) {
        p->front_pos = M_GetHeight(item, 0, STEP_L);
        p->gradient = (int16_t)(p->mid_pos - p->front_pos);
        item->pos.y = p->mid_pos;
    } else if (item->pos.y > p->mid_pos) {
        if (p->y_velocity > 0) {
            Sound_Effect(SFX_QUAD_FRONT_IMPACT, &item->pos, SPM_ALWAYS);
        }

        item->pos.y = p->mid_pos;
        p->y_velocity = 0;
    } else {
        p->y_velocity += M_GRAVITY;
        CLAMPG(p->y_velocity, M_MAX_VELOCITY);
        item->pos.y += p->y_velocity >> 8;
    }

    item->rot.x = p->gradient << 5;

    if (p->turn.side != M_SIDE_NONE) {
        const int16_t angle = item->rot.y & M_ANGLE_CLAMP;
        if (p->turn.side == M_SIDE_RIGHT) {
            item->rot.z = -(item->speed * angle) >> 9;
        } else {
            item->rot.z = (item->speed * (DEG_90 - angle)) >> 9;
        }
    } else {
        item->rot.z -= item->rot.z >> 3;
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->save_anim = true;
    obj->save_flags = true;
    obj->save_position = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "track_1", Music_IDToSlot(MX_MINE_CART_THEME),
            "Random music track pool, slot 1. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_2", -1, "Random music track pool, slot 2. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_3", -1, "Random music track pool, slot 3. -1 = disabled."),
        OBJECT_PROPERTY_STORED(
            "track_4", -1, "Random music track pool, slot 4. -1 = disabled."));
}

bool MineCart_Control(void)
{
    ITEM *const item = Lara_Vehicle_GetItem();
    M_PRIV *const p = item->priv;
    M_UserControl(item);

    if (p->flags.control) {
        M_Move(item);
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    ITEM *const lara_item = Lara_GetItem();
    const bool mounted = Lara_Vehicle_IsMounted();
    if (mounted) {
        lara_item->pos = item->pos;
        lara_item->rot = item->rot;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(lara->item_num, room_num);
    if (mounted) {
        Item_UpdateRoom(Lara_Vehicle_GetIndex(), room_num);
    }

    Room_TestTriggers(lara_item);

    if (!p->flags.dead) {
        g_Camera.target_elevation = M_CAM_RIDE_ELEVATION;
        g_Camera.target_distance = M_CAM_RIDE_DISTANCE;
    }

    return mounted;
}

REGISTER_OBJECT(O_MINE_CART, M_Setup)
