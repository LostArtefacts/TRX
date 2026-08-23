#include <trx/game/objects/vehicles/skidoo_common.h>

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/anims/walk.h>
#include <trx/game/collision.h>
#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/game_buf.h>
#include <trx/game/gun.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/vehicles/common.h>
#include <trx/game/objects/vehicles/skidoo_armed.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_RADIUS            500
#define M_STATIC_RADIUS     100
#define M_SIDE              260
#define M_FRONT             550
#define M_SNOW              500
#define M_GET_OFF_DIST      330
#define M_TARGET_DIST       (WALL_L * 2)              // = 2048
#define M_ACCELERATION      10
#define M_SLOWDOWN          2
#define M_SLIP              100
#define M_SLIP_SIDE         50
#define M_MAX_BACK          (-30)
#define M_BRAKE             5
#define M_REVERSE           (-5)
#define M_UNDO_TURN         (DEG_1 * 2)               // = 364
#define M_TURN              (DEG_1 / 2 + M_UNDO_TURN) // = 455
#define M_MOMENTUM_TURN     (DEG_1 * 3)               // = 546
#define M_MAX_MOMENTUM_TURN (DEG_1 * 150)             // = 27300
#define M_MIN_BOUNCE        50
#define M_MAX_KICK          (-80)
// clang-format on

typedef enum {
    M_GET_ON_NONE,
    M_GET_ON_LEFT,
    M_GET_ON_RIGHT,
} M_GET_ON_SIDE;

typedef enum {
    M_STATE_SIT,
    M_STATE_GET_ON,
    M_STATE_LEFT,
    M_STATE_RIGHT,
    M_STATE_FALL,
    M_STATE_HIT,
    M_STATE_GET_ON_L,
    M_STATE_GET_OFF_L,
    M_STATE_STILL,
    M_STATE_GET_OFF_R,
    M_STATE_LET_GO,
    M_STATE_DEATH,
    M_STATE_FALLOFF,
    // clang-format on
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_GET_ON_L  = 1,
    M_ANIM_FALL      = 8,
    M_ANIM_HIT_LEFT  = 11,
    M_ANIM_HIT_RIGHT = 12,
    M_ANIM_HIT_FRONT = 13,
    M_ANIM_HIT_BACK  = 14,
    M_ANIM_DEAD      = 15,
    M_ANIM_GET_ON_R  = 18,
    // clang-format on
} M_ANIM;

const BITE g_Skidoo_LeftGun = {
    .pos = { .x = 219, .y = -71, .z = M_FRONT },
    .mesh_num = 0,
};

const BITE g_Skidoo_RightGun = {
    .pos = { .x = -235, .y = -71, .z = M_FRONT },
    .mesh_num = 0,
};

static int32_t M_DoDynamics(
    const int32_t height, const int32_t fall_speed, int32_t *const out_y)
{
    if (height > *out_y) {
        *out_y += fall_speed;
        if (*out_y > height - M_MIN_BOUNCE) {
            *out_y = height;
            return 0;
        }
        return fall_speed + GRAVITY;
    }

    int32_t kick = 4 * (height - *out_y);
    CLAMPL(kick, M_MAX_KICK);
    CLAMPG(*out_y, height);
    return fall_speed + ((kick - fall_speed) >> 3);
}

static bool M_IsArmed(const SKIDOO_INFO *const skidoo_data)
{
    return skidoo_data->track_mesh & SKIDOO_GUN_MESH;
}

static bool M_CheckBaddieCollision(ITEM *const item, ITEM *const skidoo)
{
    if (!item->is_collidable || !item->is_visible || item == Lara_GetItem()
        || item == skidoo) {
        return false;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    const bool is_availanche = item->object_id == O_ROLLING_BALL_2;
    if (obj->collision_func == nullptr
        || (!obj->intelligent && !is_availanche)) {
        return false;
    }

    if (!Item_IsNearby(item, skidoo, M_TARGET_DIST)) {
        return false;
    }

    if (!Item_TestBoundsCollide(item, skidoo, M_RADIUS)) {
        return false;
    }

    if (item->object_id == O_SKIDOO_ARMED) {
        SkidooArmed_Push(item, skidoo, M_RADIUS);
    } else if (is_availanche) {
        if (item->current_anim_state == TRAP_ACTIVATE) {
            Lara_TakeDamage(100, true);
        }
    } else if (
        obj->intelligent && Item_IsInPlay(item)
        && (Item_IsTargetable(item) || item->hit_points == 0)) {
        if (Item_ShouldSpawnBlood(item)) {
            Spawn_BloodBath(
                item->pos.x, skidoo->pos.y - STEP_L, item->pos.z, skidoo->speed,
                skidoo->rot.y, item->room_num, 3);
        }
        if (item->hit_points > 0) {
            Item_TakeFatalDamage(item, skidoo);
        }
    }
    return true;
}

void Skidoo_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->priv == nullptr) {
        item->priv = GameBuf_Alloc(sizeof(SKIDOO_INFO), GBUF_ITEM_DATA);
    }

    SKIDOO_INFO *const skidoo_data = item->priv;
    skidoo_data->skidoo_turn = 0;
    skidoo_data->right_fallspeed = 0;
    skidoo_data->left_fallspeed = 0;
    skidoo_data->extra_rotation = 0;
    skidoo_data->momentum_angle = item->rot.y;
    skidoo_data->track_mesh = 0;
    skidoo_data->pitch = 0;
}

int32_t Skidoo_CheckGetOn(const int16_t item_num, COLL_INFO *const coll)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || (lara->water_status != LWS_ABOVE_WATER
            && lara->water_status != LWS_WADE)) {
        return M_GET_ON_NONE;
    }

    ITEM *const item = Item_Get(item_num);
    const int16_t angle = item->rot.y - lara_item->rot.y;

    M_GET_ON_SIDE get_on = M_GET_ON_NONE;
    if (angle > DEG_45 && angle < DEG_135) {
        get_on = M_GET_ON_LEFT;
    } else if (angle > -DEG_135 && angle < -DEG_45) {
        get_on = M_GET_ON_RIGHT;
    }

    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return M_GET_ON_NONE;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return M_GET_ON_NONE;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);
    if (height < -MAX_HEIGHT) {
        return M_GET_ON_NONE;
    }

    return get_on;
}

void Skidoo_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (lara_item->hit_points < 0 || Lara_Vehicle_IsMounted()) {
        return;
    }

    const M_GET_ON_SIDE get_on = Skidoo_CheckGetOn(item_num, coll);
    if (get_on == M_GET_ON_NONE) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    Lara_Vehicle_SetIndex(item_num);
    if (lara->gun_type == LGT_FLARE) {
        Gun_Flare_Dispose(false);
        lara->gun_type = LGT_UNARMED;
        lara->request_gun_type = LGT_UNARMED;
    }

    const M_ANIM anim_idx =
        get_on == M_GET_ON_LEFT ? M_ANIM_GET_ON_L : M_ANIM_GET_ON_R;
    Lara_Vehicle_SwitchToAnim(anim_idx, 0);
    lara_item->current_anim_state = M_STATE_GET_ON;
    lara->gun_status = LGS_ARMLESS;
    lara->hit_direction = DIR_UNKNOWN;

    ITEM *const item = Item_Get(item_num);
    lara_item->pos.x = item->pos.x;
    lara_item->pos.y = item->pos.y;
    lara_item->pos.z = item->pos.z;
    lara_item->rot.y = item->rot.y;
    item->hit_points = 1;
}

void Skidoo_BaddieCollision(ITEM *const skidoo)
{
    int16_t roomies[12];
    const int32_t roomies_count =
        Room_GetAdjoiningRooms(skidoo->room_num, roomies, 12);

    for (int32_t i = 0; i < roomies_count; i++) {
        const ROOM *const room = Room_Get(roomies[i]);
        int16_t item_num = room->item_num;
        while (item_num != NO_ITEM) {
            ITEM *item = Item_Get(item_num);
            M_CheckBaddieCollision(item, skidoo);
            item_num = item->next_item;
        }
    }
}

int32_t Skidoo_TestHeight(
    const ITEM *const item, const int32_t z_off, const int32_t x_off,
    XYZ_32 *const out_pos)
{
    *out_pos = XYZ_32_OffsetLocalYaw(
        item->pos, (XYZ_32) { .x = x_off, .z = z_off }, item->rot.y);

    // The height a ski sits at follows the skidoo's pitch and roll, which the
    // yaw turn above says nothing about.
    out_pos->y = item->pos.y
        + ((x_off * Math_Sin(item->rot.z) - z_off * Math_Sin(item->rot.x))
           >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(*out_pos, &room_num);

    const SKIDOO_INFO *const p = item->priv;
    const int32_t height = Room_GetHeight(sector, *out_pos);
    if (height == NO_HEIGHT || !p->test_static_collision) {
        return height;
    }

    COLL_INFO coll = {};
    coll.radius = M_STATIC_RADIUS;
    if (Collide_CollideStaticObjects(
            &coll, *out_pos, item->room_num, LARA_HEIGHT)) {
        return NO_HEIGHT;
    }
    return height;
}

void Skidoo_DoSnowEffect(const ITEM *const skidoo)
{
    if (!Object_Get(O_SNOW_SPRITE)->loaded) {
        return;
    }

    const int16_t effect_num = Effect_Create(skidoo->room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    const int32_t x = (M_SIDE * (Random_GetDraw() - 0x4000)) >> 14;
    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = XYZ_32_OffsetLocalYaw(
        skidoo->pos, (XYZ_32) { .x = -x, .z = -M_SNOW }, skidoo->rot.y);
    effect->pos.y =
        skidoo->pos.y + ((Math_Sin(skidoo->rot.x) * M_SNOW) >> W2V_SHIFT);
    effect->room_num = skidoo->room_num;
    effect->object_id = O_SNOW_SPRITE;
    effect->frame_num = 0;
    effect->speed = 0;
    if (skidoo->speed < 64) {
        effect->fall_speed =
            (Random_GetDraw() * (ABS(skidoo->speed) - 64)) >> 15;
    } else {
        effect->fall_speed = 0;
    }

    g_MatrixPtr->_23 = 0;
    g_WMatrixPtr->_23 = 0;

    Output_CalculateLight(effect->pos, effect->room_num);
    effect->shade = Output_GetLightAdder() - 512;
    CLAMPL(effect->shade, 0);
}

int32_t Skidoo_Dynamics(ITEM *const skidoo)
{
    SKIDOO_INFO *const skidoo_data = skidoo->priv;

    XYZ_32 fl_old;
    XYZ_32 bl_old;
    XYZ_32 br_old;
    XYZ_32 fr_old;
    int32_t hfl_old = Skidoo_TestHeight(skidoo, M_FRONT, -M_SIDE, &fl_old);
    int32_t hfr_old = Skidoo_TestHeight(skidoo, M_FRONT, M_SIDE, &fr_old);
    int32_t hbl_old = Skidoo_TestHeight(skidoo, -M_FRONT, -M_SIDE, &bl_old);
    int32_t hbr_old = Skidoo_TestHeight(skidoo, -M_FRONT, M_SIDE, &br_old);

    XYZ_32 old = {
        .z = skidoo->pos.z,
        .x = skidoo->pos.x,
        .y = skidoo->pos.y,
    };

    CLAMPG(bl_old.y, hbl_old);
    CLAMPG(br_old.y, hbr_old);
    CLAMPG(fl_old.y, hfl_old);
    CLAMPG(fr_old.y, hfr_old);

    if (skidoo->pos.y <= skidoo->floor - STEP_L) {
        skidoo->rot.y += skidoo_data->extra_rotation + skidoo_data->skidoo_turn;
    } else {
        if (skidoo_data->skidoo_turn < -M_UNDO_TURN) {
            skidoo_data->skidoo_turn += M_UNDO_TURN;
        } else if (skidoo_data->skidoo_turn > M_UNDO_TURN) {
            skidoo_data->skidoo_turn -= M_UNDO_TURN;
        } else {
            skidoo_data->skidoo_turn = 0;
        }
        skidoo->rot.y += skidoo_data->skidoo_turn + skidoo_data->extra_rotation;

        int16_t rot = skidoo->rot.y - skidoo_data->momentum_angle;
        if (rot < -M_MOMENTUM_TURN) {
            if (rot < -M_MAX_MOMENTUM_TURN) {
                rot = -M_MAX_MOMENTUM_TURN;
                skidoo_data->momentum_angle = skidoo->rot.y - rot;
            } else {
                skidoo_data->momentum_angle -= M_MOMENTUM_TURN;
            }
        } else if (rot > M_MOMENTUM_TURN) {
            if (rot > M_MAX_MOMENTUM_TURN) {
                rot = M_MAX_MOMENTUM_TURN;
                skidoo_data->momentum_angle = skidoo->rot.y - rot;
            } else {
                skidoo_data->momentum_angle += M_MOMENTUM_TURN;
            }
        } else {
            skidoo_data->momentum_angle = skidoo->rot.y;
        }
    }

    skidoo->pos = XYZ_32_OffsetYaw(
        skidoo->pos, skidoo_data->momentum_angle, skidoo->speed);

    int32_t slip;
    slip = (M_SLIP * Math_Sin(skidoo->rot.x)) >> W2V_SHIFT;
    if (ABS(slip) > M_SLIP / 2) {
        skidoo->pos = XYZ_32_Subtract(
            skidoo->pos,
            XYZ_32_RotateYaw((XYZ_32) { .z = slip }, skidoo->rot.y));
    }

    slip = (M_SLIP_SIDE * Math_Sin(skidoo->rot.z)) >> W2V_SHIFT;
    if (ABS(slip) > M_SLIP_SIDE / 2) {
        // Sideways, along the skidoo's own x, read off a forward turn so
        // that each component rounds as it did.
        const XYZ_32 roll_slip =
            XYZ_32_RotateYaw((XYZ_32) { .z = slip }, skidoo->rot.y);
        skidoo->pos.x += roll_slip.z;
        skidoo->pos.z -= roll_slip.x;
    }

    XYZ_32 moved = {
        .x = skidoo->pos.x,
        .z = skidoo->pos.z,
    };
    if (!skidoo->trigger.spent) {
        Skidoo_BaddieCollision(skidoo);
    }

    int32_t rot = 0;

    XYZ_32 br;
    XYZ_32 fl;
    XYZ_32 bl;
    XYZ_32 fr;
    const int32_t hbl = Skidoo_TestHeight(skidoo, -M_FRONT, -M_SIDE, &bl);
    if (hbl < bl_old.y - STEP_L) {
        rot = Vehicle_DoShift(skidoo, &bl, &bl_old);
    }
    const int32_t hbr = Skidoo_TestHeight(skidoo, -M_FRONT, M_SIDE, &br);
    if (hbr < br_old.y - STEP_L) {
        rot += Vehicle_DoShift(skidoo, &br, &br_old);
    }
    const int32_t hfl = Skidoo_TestHeight(skidoo, M_FRONT, -M_SIDE, &fl);
    if (hfl < fl_old.y - STEP_L) {
        rot += Vehicle_DoShift(skidoo, &fl, &fl_old);
    }
    const int32_t hfr = Skidoo_TestHeight(skidoo, M_FRONT, M_SIDE, &fr);
    if (hfr < fr_old.y - STEP_L) {
        rot += Vehicle_DoShift(skidoo, &fr, &fr_old);
    }

    int16_t room_num = skidoo->room_num;
    const SECTOR *const sector = Room_GetSector(skidoo->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, skidoo->pos);
    if (height < skidoo->pos.y - STEP_L) {
        Vehicle_DoShift(skidoo, &skidoo->pos, &old);
    }

    skidoo_data->extra_rotation = rot;

    int32_t collide = Vehicle_GetCollisionAnim(skidoo, &moved);
    if (collide != 0) {
        const XYZ_32 delta = XYZ_32_Subtract(skidoo->pos, old);
        const int32_t new_speed =
            XYZ_32_UnrotateYaw(delta, skidoo_data->momentum_angle).z;

        if (skidoo == Lara_Vehicle_GetItem()
            && skidoo->speed > SKIDOO_MAX_SPEED + M_ACCELERATION
            && new_speed < skidoo->speed - M_ACCELERATION) {
            Lara_TakeDamage((skidoo->speed - new_speed) / 2, true);
        }

        if (skidoo->speed > 0 && new_speed < skidoo->speed) {
            skidoo->speed = new_speed < 0 ? 0 : new_speed;
        } else if (skidoo->speed < 0 && new_speed > skidoo->speed) {
            skidoo->speed = new_speed > 0 ? 0 : new_speed;
        }

        if (skidoo->speed < M_MAX_BACK) {
            skidoo->speed = M_MAX_BACK;
        }
    }

    return collide;
}

int32_t Skidoo_UserControl(
    ITEM *const skidoo, const int32_t height, int32_t *const out_pitch)
{
    SKIDOO_INFO *const skidoo_data = skidoo->priv;

    bool drive = false;

    if (skidoo->pos.y >= height - STEP_L) {
        *out_pitch = skidoo->speed + (height - skidoo->pos.y);

        if (skidoo->speed == 0 && g_Input.look) {
            Lara_Look_UpDown();
        }

        if ((g_Input.left && !g_Input.back)
            || (g_Input.right && g_Input.back)) {
            skidoo_data->skidoo_turn -= M_TURN;
            CLAMPL(skidoo_data->skidoo_turn, -SKIDOO_MAX_TURN);
        }

        if ((g_Input.right && !g_Input.back)
            || (g_Input.left && g_Input.back)) {
            skidoo_data->skidoo_turn += M_TURN;
            CLAMPG(skidoo_data->skidoo_turn, SKIDOO_MAX_TURN);
        }

        if (g_Input.back) {
            if (skidoo->speed > 0) {
                skidoo->speed -= M_BRAKE;
            } else {
                if (skidoo->speed > M_MAX_BACK) {
                    skidoo->speed += M_REVERSE;
                }
                drive = true;
            }
        } else if (g_Input.forward) {
            int32_t max_speed;
            if (g_Input.action && !M_IsArmed(skidoo_data)) {
                max_speed = SKIDOO_FAST_SPEED;
            } else if (g_Input.slow) {
                max_speed = SKIDOO_SLOW_SPEED;
            } else {
                max_speed = SKIDOO_MAX_SPEED;
            }

            if (skidoo->speed < max_speed) {
                skidoo->speed +=
                    M_ACCELERATION * skidoo->speed / (2 * max_speed)
                    + M_ACCELERATION / 2;
            } else if (skidoo->speed > max_speed + M_SLOWDOWN) {
                skidoo->speed -= M_SLOWDOWN;
            }

            drive = true;
        } else if (
            skidoo->speed >= 0 && skidoo->speed < SKIDOO_MIN_SPEED
            && (g_Input.left || g_Input.right)) {
            skidoo->speed = SKIDOO_MIN_SPEED;
            drive = true;
        } else if (skidoo->speed > M_SLOWDOWN) {
            skidoo->speed -= M_SLOWDOWN;
            if ((Random_GetDraw() & 0x7F) < skidoo->speed) {
                drive = true;
            }
        } else {
            skidoo->speed = 0;
        }
    } else if (g_Input.forward || g_Input.back) {
        drive = true;
        *out_pitch = skidoo_data->pitch + 50;
    }

    return drive;
}

int32_t Skidoo_CheckGetOffOK(int32_t direction)
{
    ITEM *const skidoo = Lara_Vehicle_GetItem();

    int16_t rot;
    if (direction == M_STATE_GET_OFF_L) {
        rot = skidoo->rot.y + DEG_90;
    } else {
        rot = skidoo->rot.y - DEG_90;
    }

    const XYZ_32 pos = XYZ_32_OffsetYaw(skidoo->pos, rot, -M_GET_OFF_DIST);

    int16_t room_num = skidoo->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const HEIGHT_TYPE height_type = Room_GetHeightType();

    if (height_type == HT_BIG_SLOPE || height_type == HT_DIAGONAL
        || height == NO_HEIGHT) {
        return false;
    }

    if (ABS(height - skidoo->pos.y) > WALL_L / 2) {
        return false;
    }

    const int32_t ceiling = Room_GetCeiling(sector, pos);
    if (ceiling - skidoo->pos.y > -LARA_HEIGHT) {
        return false;
    }
    if (height - ceiling < LARA_HEIGHT) {
        return false;
    }

    return true;
}

void Skidoo_Animation(
    ITEM *const skidoo, const int32_t collide, const int32_t dead)
{
    const SKIDOO_INFO *const skidoo_data = skidoo->priv;
    ITEM *const lara_item = Lara_GetItem();

    if (skidoo->pos.y != skidoo->floor && skidoo->fall_speed > 0
        && lara_item->current_anim_state != M_STATE_FALL && !dead) {
        Lara_Vehicle_SwitchToAnim(M_ANIM_FALL, 0);
        lara_item->goal_anim_state = M_STATE_FALL;
        lara_item->current_anim_state = M_STATE_FALL;
        return;
    }

    if (collide != 0 && !dead
        && lara_item->current_anim_state != M_STATE_FALL) {
        if (lara_item->current_anim_state != M_STATE_HIT) {
            if (collide == M_ANIM_HIT_FRONT) {
                Sound_Effect(SFX_CLATTER_2, &skidoo->pos, SPM_NORMAL);
            } else {
                Sound_Effect(SFX_CLATTER_1, &skidoo->pos, SPM_NORMAL);
            }
            Lara_Vehicle_SwitchToAnim(collide, 0);
            lara_item->goal_anim_state = M_STATE_HIT;
            lara_item->current_anim_state = M_STATE_HIT;
        }
        return;
    }

    switch (lara_item->current_anim_state) {
    case M_STATE_SIT:
        if (skidoo->speed == 0) {
            lara_item->goal_anim_state = M_STATE_STILL;
        }
        if (dead) {
            lara_item->goal_anim_state = M_STATE_FALLOFF;
        } else if (g_Input.left) {
            lara_item->goal_anim_state = M_STATE_LEFT;
        } else if (g_Input.right) {
            lara_item->goal_anim_state = M_STATE_RIGHT;
        }
        break;

    case M_STATE_LEFT:
        if (!g_Input.left) {
            lara_item->goal_anim_state = M_STATE_SIT;
        }
        break;

    case M_STATE_RIGHT:
        if (!g_Input.right) {
            lara_item->goal_anim_state = M_STATE_SIT;
        }
        break;

    case M_STATE_FALL:
        if (skidoo->fall_speed <= 0 || skidoo_data->left_fallspeed <= 0
            || skidoo_data->right_fallspeed <= 0) {
            Sound_Effect(SFX_CLATTER_3, &skidoo->pos, SPM_NORMAL);
            lara_item->goal_anim_state = M_STATE_SIT;
        } else if (skidoo->fall_speed > DAMAGE_START + DAMAGE_LENGTH) {
            lara_item->goal_anim_state = M_STATE_LET_GO;
        }
        break;

    case M_STATE_STILL: {
        Vehicle_PlayOneShotTrackPool(
            skidoo, M_IsArmed(skidoo_data) ? "battle_track" : "track");

        if (dead) {
            lara_item->goal_anim_state = M_STATE_DEATH;
            return;
        }

        lara_item->goal_anim_state = M_STATE_STILL;

        if (g_Input.jump) {
            if (g_Input.right && Skidoo_CheckGetOffOK(M_STATE_GET_OFF_R)) {
                lara_item->goal_anim_state = M_STATE_GET_OFF_R;
                skidoo->speed = 0;
            } else if (
                g_Input.left && Skidoo_CheckGetOffOK(M_STATE_GET_OFF_L)) {
                lara_item->goal_anim_state = M_STATE_GET_OFF_L;
                skidoo->speed = 0;
            }
        } else if (g_Input.left) {
            lara_item->goal_anim_state = M_STATE_LEFT;
        } else if (g_Input.right) {
            lara_item->goal_anim_state = M_STATE_RIGHT;
        } else if (g_Input.back || g_Input.forward) {
            lara_item->goal_anim_state = M_STATE_SIT;
        }
        break;
    }

    default:
        break;
    }
}

void Skidoo_Explode(const ITEM *const skidoo)
{
    const int16_t effect_num = Effect_Create(skidoo->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->pos.x = skidoo->pos.x;
        effect->pos.y = skidoo->pos.y;
        effect->pos.z = skidoo->pos.z;
        effect->speed = 0;
        effect->frame_num = 0;
        effect->counter = 0;
        effect->object_id = O_EXPLOSION_1;
    }

    Item_Shatter(Item_GetIndex(skidoo), ~(SKIDOO_GUN_MESH - 1), 0);
    Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_NORMAL);
    Lara_Vehicle_SetIndex(NO_ITEM);
}

bool Skidoo_CheckGetOff(void)
{
    ITEM *const skidoo = Lara_Vehicle_GetItem();
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if ((lara_item->current_anim_state == M_STATE_GET_OFF_R
         || lara_item->current_anim_state == M_STATE_GET_OFF_L)
        && Item_TestFrameEqual(lara_item, -1)) {
        if (lara_item->current_anim_state == M_STATE_GET_OFF_L) {
            lara_item->rot.y += DEG_90;
        } else {
            lara_item->rot.y -= DEG_90;
        }
        Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);
        lara_item->goal_anim_state = LS(LS_STOP);
        lara_item->current_anim_state = LS(LS_STOP);
        lara_item->pos = XYZ_32_Subtract(
            lara_item->pos,
            XYZ_32_RotateYaw(
                (XYZ_32) { .z = M_GET_OFF_DIST }, lara_item->rot.y));
        lara_item->rot.x = 0;
        lara_item->rot.z = 0;
        Lara_Vehicle_SetIndex(NO_ITEM);
        lara->gun_status = LGS_ARMLESS;
        return true;
    }

    if (lara_item->current_anim_state == M_STATE_LET_GO
        && (skidoo->pos.y == skidoo->floor
            || Item_TestFrameEqual(lara_item, -1))) {
        Item_SwitchToAnim(lara_item, LA(LA_FREEFALL), 0);
        lara_item->current_anim_state = M_STATE_GET_OFF_R;
        if (skidoo->pos.y == skidoo->floor) {
            if (g_Config.debug.enable_invulnerability) {
                lara_item->goal_anim_state = LS(LS_STOP);
                lara_item->current_anim_state = LS(LS_STOP);
                Item_SwitchToAnim(lara_item, LA(LA_FREEFALL_LAND), 0);
            } else {
                lara_item->goal_anim_state = M_STATE_STILL;
                lara_item->fall_speed = DAMAGE_START + DAMAGE_LENGTH;
            }
            lara_item->speed = 0;
            Skidoo_Explode(skidoo);
        } else {
            lara_item->goal_anim_state = M_STATE_GET_OFF_R;
            lara_item->pos.y -= 200;
            lara_item->fall_speed = skidoo->fall_speed;
            lara_item->speed = skidoo->speed;
            if (!g_Config.debug.enable_invulnerability) {
                Sound_Effect(SFX_LARA_FALL, &lara_item->pos, SPM_NORMAL);
            }
        }
        lara_item->rot.x = 0;
        lara_item->rot.z = 0;
        lara_item->gravity = true;
        lara->gun_status = LGS_ARMLESS;
        lara->move_angle = skidoo->rot.y;
        skidoo->trigger.spent = true;
        skidoo->is_collidable = 0;
        return false;
    }

    return true;
}

void Skidoo_Guns(void)
{
    WEAPON_INFO *const weapon = Gun_Registry_Get(LGT_SKIDOO);
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    Gun_GetNewTarget(weapon);
    Gun_AimWeapon(weapon, &lara->right_arm);

    if (!g_Input.action) {
        return;
    }

    int16_t angles[2];
    angles[0] = lara->right_arm.rot.y + lara_item->rot.y;
    angles[1] = lara->right_arm.rot.x;

    if (!Gun_FireWeapon(LGT_SKIDOO, lara->target, lara_item, angles)) {
        return;
    }

    lara->right_arm.flash_gun = weapon->flash.time;
    Sound_Effect(weapon->sample_num, &lara_item->pos, SPM_NORMAL);
    Gun_AddDynamicLight();

    ITEM *const skidoo = Lara_Vehicle_GetItem();
    Creature_Effect(skidoo, &g_Skidoo_LeftGun, Spawn_GunShot);
    Creature_Effect(skidoo, &g_Skidoo_RightGun, Spawn_GunShot);
}

bool Skidoo_Control(void)
{
    ITEM *const lara_item = Lara_GetItem();
    ITEM *const skidoo = Lara_Vehicle_GetItem();
    SKIDOO_INFO *const skidoo_data = skidoo->priv;
    int32_t collide = Skidoo_Dynamics(skidoo);

    XYZ_32 fl;
    XYZ_32 fr;
    const int32_t hfl = Skidoo_TestHeight(skidoo, M_FRONT, -M_SIDE, &fl);
    const int32_t hfr = Skidoo_TestHeight(skidoo, M_FRONT, M_SIDE, &fr);

    int16_t room_num = skidoo->room_num;
    const SECTOR *const sector = Room_GetSector(skidoo->pos, &room_num);
    int32_t height = Room_GetHeight(sector, skidoo->pos);

    bool dead = false;
    if (lara_item->hit_points <= 0) {
        dead = true;
        g_Input.back = 0;
        g_Input.forward = 0;
        g_Input.left = 0;
        g_Input.right = 0;
    } else if (lara_item->current_anim_state == M_STATE_LET_GO) {
        dead = true;
        collide = 0;
    }

    int32_t drive;
    int32_t pitch;
    if (skidoo->trigger.spent) {
        drive = 0;
        collide = 0;
    } else {
        switch (lara_item->current_anim_state) {
        case M_STATE_GET_ON:
        case M_STATE_GET_OFF_L:
        case M_STATE_GET_OFF_R:
        case M_STATE_LET_GO:
            drive = -1;
            collide = 0;
            break;

        default:
            drive = Skidoo_UserControl(skidoo, height, &pitch);
            break;
        }
    }

    const int32_t old_track_mesh = skidoo_data->track_mesh;
    if (drive > 0) {
        skidoo_data->track_mesh = (skidoo_data->track_mesh & 3) == 1 ? 2 : 1;
        skidoo_data->pitch += (pitch - skidoo_data->pitch) >> 2;

        const int32_t pitch_delta =
            (SKIDOO_MAX_SPEED - skidoo_data->pitch) * 100;

        Sound_Effect(
            SFX_SKIDOO_MOVING, &skidoo->pos,
            SPM_PITCH | ((SOUND_DEFAULT_PITCH - pitch_delta) << 8));
    } else {
        skidoo_data->track_mesh = 0;
        if (!drive) {
            Sound_Effect(SFX_SKIDOO_IDLE, &skidoo->pos, SPM_NORMAL);
        }
        skidoo_data->pitch = 0;
    }
    skidoo_data->track_mesh |= old_track_mesh & SKIDOO_GUN_MESH;

    skidoo->floor = height;

    skidoo_data->left_fallspeed =
        M_DoDynamics(hfl, skidoo_data->left_fallspeed, &fl.y);
    skidoo_data->right_fallspeed =
        M_DoDynamics(hfr, skidoo_data->right_fallspeed, &fr.y);
    skidoo->fall_speed =
        M_DoDynamics(height, skidoo->fall_speed, &skidoo->pos.y);

    height = (fr.y + fl.y) / 2;
    const int16_t x_rot = Math_Atan(M_FRONT, skidoo->pos.y - height);
    const int16_t z_rot = Math_Atan(M_SIDE, height - fl.y);
    skidoo->rot.x += (x_rot - skidoo->rot.x) >> 1;
    skidoo->rot.z += (z_rot - skidoo->rot.z) >> 1;

    Room_GetSector(
        (XYZ_32) { skidoo->pos.x, skidoo->pos.y - 16, skidoo->pos.z },
        &room_num);
    if (skidoo->trigger.spent) {
        Vehicle_TestTriggers(lara_item, skidoo);
        Item_UpdateRoom(Item_GetIndex(skidoo), room_num);
        if (skidoo->pos.y == skidoo->floor) {
            Skidoo_Explode(skidoo);
        }
        return false;
    }

    Skidoo_Animation(skidoo, collide, dead);
    Item_UpdateRoom(Item_GetIndex(skidoo), room_num);
    Item_UpdateRoom(Item_GetIndex(lara_item), room_num);

    if (lara_item->current_anim_state == M_STATE_FALLOFF) {
        lara_item->rot.x = 0;
        lara_item->rot.z = 0;
    } else {
        lara_item->pos = skidoo->pos;
        lara_item->rot.y = skidoo->rot.y;
        if (drive >= 0) {
            lara_item->rot.x = skidoo->rot.x;
            lara_item->rot.z = skidoo->rot.z;
        } else {
            lara_item->rot.x = 0;
            lara_item->rot.z = 0;
        }
    }
    Vehicle_TestTriggers(lara_item, skidoo);

    Item_Animate(lara_item);
    if (!dead && drive >= 0 && M_IsArmed(skidoo_data)) {
        Skidoo_Guns();
    }

    if (dead) {
        Item_SwitchToObjAnim(skidoo, M_ANIM_DEAD, 0, O_SKIDOO_FAST);
    } else {
        Lara_Vehicle_SyncItemAnim();
    }

    if (skidoo->speed != 0 && skidoo->floor == skidoo->pos.y) {
        Skidoo_DoSnowEffect(skidoo);
        if (skidoo->speed < SKIDOO_SLOW_SPEED) {
            Skidoo_DoSnowEffect(skidoo);
        }
    }

    return Skidoo_CheckGetOff();
}

bool Skidoo_Draw(const ITEM *const item)
{
    int32_t track_mesh_status = 0;
    const SKIDOO_INFO *const skidoo_data = item->priv;
    if (skidoo_data != nullptr) {
        track_mesh_status = skidoo_data->track_mesh;
    }

    const OBJECT *obj = Object_Get(item->object_id);
    if ((track_mesh_status & SKIDOO_GUN_MESH) != 0) {
        obj = Object_Get(O_SKIDOO_ARMED);
    }

    const OBJECT *const track_obj = Object_Get(O_SKIDOO_TRACK);
    const OBJECT_MESH *track_mesh = nullptr;
    if ((track_mesh_status & 3) == 1) {
        track_mesh = Object_GetMesh(track_obj->mesh_idx + 1);
    } else if ((track_mesh_status & 3) == 2) {
        track_mesh = Object_GetMesh(track_obj->mesh_idx + 7);
    }

    ANIM_FRAME *frames[2];
    int32_t rate;
    const int32_t frac = Item_GetFrames(item, frames, &rate);

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    const CLIP clip = Output_CheckBoundsClip(&frames[0]->bounds);
    if (clip == CLIP_NOT_VISIBLE) {
        Matrix_Pop();
        return false;
    }

    Output_CalculateObjectLighting(item, &frames[0]->bounds);

    ANIM_WALK walk;
    Anim_Walk_Begin(
        &walk,
        &(ANIM_WALK_DESC) {
            .obj = obj,
            .pose = Anim_Pose_FromFrames(frames[0], frames[1], frac, rate),
        });
    while (Anim_Walk_Next(&walk)) {
        if (walk.joint == 1 && track_mesh != nullptr) {
            if (walk.interpolated) {
                Output_DrawObjectMesh_I(track_mesh, clip);
            } else {
                Output_DrawObjectMesh(track_mesh, clip);
            }
        } else {
            Object_DrawMesh(
                obj->mesh_idx + walk.joint, clip, walk.interpolated);
        }
    }
    Anim_Walk_End(&walk);

    Matrix_Pop();
    return true;
}
