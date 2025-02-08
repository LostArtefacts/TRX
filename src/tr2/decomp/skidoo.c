#include "decomp/skidoo.h"

#include "decomp/decomp.h"
#include "decomp/flares.h"
#include "game/collide.h"
#include "game/creature.h"
#include "game/effects.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/look.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/objects/vehicles/skidoo_armed.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define SKIDOO_RADIUS 500
#define SKIDOO_SIDE 260
#define SKIDOO_FRONT 550
#define SKIDOO_SNOW 500
#define SKIDOO_GET_OFF_DIST 330
#define SKIDOO_TARGET_DIST (WALL_L * 2) // = 2048

#define SKIDOO_ACCELERATION 10
#define SKIDOO_SLOWDOWN 2

#define SKIDOO_SLIP 100
#define SKIDOO_SLIP_SIDE 50

#define SKIDOO_MAX_BACK -30
#define SKIDOO_BRAKE 5
#define SKIDOO_REVERSE (-5)
#define SKIDOO_UNDO_TURN (DEG_1 * 2) // = 364
#define SKIDOO_TURN (DEG_1 / 2 + SKIDOO_UNDO_TURN) // = 455
#define SKIDOO_MOMENTUM_TURN (DEG_1 * 3) // = 546
#define SKIDOO_MAX_MOMENTUM_TURN (DEG_1 * 150) // = 27300

#define LF_SKIDOO_EXIT_END 59
#define LF_SKIDOO_LET_GO_END 17

typedef enum {
    SKIDOO_GET_ON_NONE = 0,
    SKIDOO_GET_ON_LEFT = 1,
    SKIDOO_GET_ON_RIGHT = 2,
} SKIDOO_GET_ON_SIDE;

typedef enum {
    // clang-format off
    LARA_STATE_SKIDOO_SIT       = 0,
    LARA_STATE_SKIDOO_GET_ON    = 1,
    LARA_STATE_SKIDOO_LEFT      = 2,
    LARA_STATE_SKIDOO_RIGHT     = 3,
    LARA_STATE_SKIDOO_FALL      = 4,
    LARA_STATE_SKIDOO_HIT       = 5,
    LARA_STATE_SKIDOO_GET_ON_L  = 6,
    LARA_STATE_SKIDOO_GET_OFF_L = 7,
    LARA_STATE_SKIDOO_STILL     = 8,
    LARA_STATE_SKIDOO_GET_OFF_R = 9,
    LARA_STATE_SKIDOO_LET_GO    = 10,
    LARA_STATE_SKIDOO_DEATH     = 11,
    LARA_STATE_SKIDOO_FALLOFF   = 12,
    // clang-format on
} LARA_SKIDOO_STATE;

typedef enum {
    // clang-format off
    LA_SKIDOO_GET_ON_L = 1,
    LA_SKIDOO_FALL = 8,
    LA_SKIDOO_HIT_LEFT = 11,
    LA_SKIDOO_HIT_RIGHT = 12,
    LA_SKIDOO_HIT_FRONT = 13,
    LA_SKIDOO_HIT_BACK = 14,
    LA_SKIDOO_DEAD = 15,
    LA_SKIDOO_GET_ON_R = 18,
    // clang-format on
} LARA_ANIM_SKIDOO;

BITE g_Skidoo_LeftGun = {
    .pos = { .x = 219, .y = -71, .z = SKIDOO_FRONT },
    .mesh_num = 0,
};

BITE g_Skidoo_RightGun = {
    .pos = { .x = -235, .y = -71, .z = SKIDOO_FRONT },
    .mesh_num = 0,
};

static bool M_IsNearby(const ITEM *item_1, const ITEM *item_2);
static bool M_IsArmed(const SKIDOO_INFO *const skidoo_data);
static bool M_CheckBaddieCollision(ITEM *item, ITEM *skidoo);

static bool M_IsNearby(const ITEM *const item_1, const ITEM *const item_2)
{
    const int32_t dx = item_1->pos.x - item_2->pos.x;
    const int32_t dy = item_1->pos.y - item_2->pos.y;
    const int32_t dz = item_1->pos.z - item_2->pos.z;
    return dx > -SKIDOO_TARGET_DIST && dx < SKIDOO_TARGET_DIST
        && dy > -SKIDOO_TARGET_DIST && dy < SKIDOO_TARGET_DIST
        && dz > -SKIDOO_TARGET_DIST && dz < SKIDOO_TARGET_DIST;
}

static bool M_IsArmed(const SKIDOO_INFO *const skidoo_data)
{
    return skidoo_data->track_mesh & SKIDOO_GUN_MESH;
}

static bool M_CheckBaddieCollision(ITEM *const item, ITEM *const skidoo)
{
    if (!item->collidable || item->status == IS_INVISIBLE || item == g_LaraItem
        || item == skidoo) {
        return false;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    const bool is_availanche = item->object_id == O_ROLLING_BALL_2;
    if (obj->collision == nullptr || (!obj->intelligent && !is_availanche)) {
        return false;
    }

    if (!M_IsNearby(item, skidoo)) {
        return false;
    }

    if (!Item_TestBoundsCollide(item, skidoo, SKIDOO_RADIUS)) {
        return false;
    }

    if (item->object_id == O_SKIDOO_ARMED) {
        SkidooArmed_Push(item, skidoo, SKIDOO_RADIUS);
    } else if (is_availanche) {
        if (item->current_anim_state == TRAP_ACTIVATE) {
            Lara_TakeDamage(100, true);
        }
    } else if (obj->intelligent && item->status == IS_ACTIVE) {
        Spawn_BloodBath(
            item->pos.x, skidoo->pos.y - STEP_L, item->pos.z, skidoo->speed,
            skidoo->rot.y, item->room_num, 3);
        Gun_HitTarget(item, nullptr, item->hit_points);
    }
    return true;
}

void Skidoo_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    SKIDOO_INFO *const skidoo_data =
        GameBuf_Alloc(sizeof(SKIDOO_INFO), GBUF_ITEM_DATA);
    skidoo_data->skidoo_turn = 0;
    skidoo_data->right_fallspeed = 0;
    skidoo_data->left_fallspeed = 0;
    skidoo_data->extra_rotation = 0;
    skidoo_data->momentum_angle = item->rot.y;
    skidoo_data->track_mesh = 0;
    skidoo_data->pitch = 0;

    item->data = skidoo_data;
}

int32_t Skidoo_CheckGetOn(const int16_t item_num, COLL_INFO *const coll)
{
    if (!g_Input.action || g_Lara.gun_status != LGS_ARMLESS
        || g_LaraItem->gravity) {
        return SKIDOO_GET_ON_NONE;
    }

    ITEM *const item = Item_Get(item_num);
    const int16_t angle = item->rot.y - g_LaraItem->rot.y;

    SKIDOO_GET_ON_SIDE get_on = SKIDOO_GET_ON_NONE;
    if (angle > DEG_45 && angle < DEG_135) {
        get_on = SKIDOO_GET_ON_LEFT;
    } else if (angle > -DEG_135 && angle < -DEG_45) {
        get_on = SKIDOO_GET_ON_RIGHT;
    }

    if (!Item_TestBoundsCollide(item, g_LaraItem, coll->radius)) {
        return SKIDOO_GET_ON_NONE;
    }

    if (!Collide_TestCollision(item, g_LaraItem)) {
        return SKIDOO_GET_ON_NONE;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (height < -32000) {
        return SKIDOO_GET_ON_NONE;
    }

    return get_on;
}

void Skidoo_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (lara_item->hit_points < 0 || g_Lara.skidoo != NO_ITEM) {
        return;
    }

    const SKIDOO_GET_ON_SIDE get_on = Skidoo_CheckGetOn(item_num, coll);
    if (get_on == SKIDOO_GET_ON_NONE) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    g_Lara.skidoo = item_num;
    if (g_Lara.gun_type == LGT_FLARE) {
        Flare_Create(false);
        Flare_UndrawMeshes();
        g_Lara.flare_control_left = 0;
        g_Lara.gun_type = LGT_UNARMED;
        g_Lara.request_gun_type = LGT_UNARMED;
    }

    const LARA_ANIM_SKIDOO anim_idx =
        get_on == SKIDOO_GET_ON_LEFT ? LA_SKIDOO_GET_ON_L : LA_SKIDOO_GET_ON_R;
    Item_SwitchToObjAnim(lara_item, anim_idx, 0, O_LARA_SKIDOO);
    lara_item->current_anim_state = LARA_STATE_SKIDOO_GET_ON;
    g_Lara.gun_status = LGS_ARMLESS;
    g_Lara.hit_direction = -1;

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
    const int32_t sx = Math_Sin(item->rot.x);
    const int32_t sz = Math_Sin(item->rot.z);
    const int32_t cy = Math_Cos(item->rot.y);
    const int32_t sy = Math_Sin(item->rot.y);
    out_pos->x = item->pos.x + ((x_off * cy + z_off * sy) >> W2V_SHIFT);
    out_pos->y = item->pos.y + ((x_off * sz - z_off * sx) >> W2V_SHIFT);
    out_pos->z = item->pos.z + ((z_off * cy - x_off * sy) >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(out_pos->x, out_pos->y, out_pos->z, &room_num);
    return Room_GetHeight(sector, out_pos->x, out_pos->y, out_pos->z);
}

void Skidoo_DoSnowEffect(const ITEM *const skidoo)
{
    const int16_t effect_num = Effect_Create(skidoo->room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    const int32_t sx = Math_Sin(skidoo->rot.x);
    const int32_t sy = Math_Sin(skidoo->rot.y);
    const int32_t cy = Math_Cos(skidoo->rot.y);
    const int32_t x = (SKIDOO_SIDE * (Random_GetDraw() - 0x4000)) >> 14;
    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos.x = skidoo->pos.x - ((sy * SKIDOO_SNOW + cy * x) >> W2V_SHIFT);
    effect->pos.y = skidoo->pos.y + ((sx * SKIDOO_SNOW) >> W2V_SHIFT);
    effect->pos.z = skidoo->pos.z - ((cy * SKIDOO_SNOW - sy * x) >> W2V_SHIFT);
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

    Output_CalculateLight(effect->pos, effect->room_num);
    effect->shade = g_LsAdder - 512;
    CLAMPL(effect->shade, 0);
}

int32_t Skidoo_Dynamics(ITEM *const skidoo)
{
    SKIDOO_INFO *const skidoo_data = skidoo->data;

    XYZ_32 fl_old;
    XYZ_32 bl_old;
    XYZ_32 br_old;
    XYZ_32 fr_old;
    int32_t hfl_old =
        Skidoo_TestHeight(skidoo, SKIDOO_FRONT, -SKIDOO_SIDE, &fl_old);
    int32_t hfr_old =
        Skidoo_TestHeight(skidoo, SKIDOO_FRONT, SKIDOO_SIDE, &fr_old);
    int32_t hbl_old =
        Skidoo_TestHeight(skidoo, -SKIDOO_FRONT, -SKIDOO_SIDE, &bl_old);
    int32_t hbr_old =
        Skidoo_TestHeight(skidoo, -SKIDOO_FRONT, SKIDOO_SIDE, &br_old);

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
        if (skidoo_data->skidoo_turn < -SKIDOO_UNDO_TURN) {
            skidoo_data->skidoo_turn += SKIDOO_UNDO_TURN;
        } else if (skidoo_data->skidoo_turn > SKIDOO_UNDO_TURN) {
            skidoo_data->skidoo_turn -= SKIDOO_UNDO_TURN;
        } else {
            skidoo_data->skidoo_turn = 0;
        }
        skidoo->rot.y += skidoo_data->skidoo_turn + skidoo_data->extra_rotation;

        int16_t rot = skidoo->rot.y - skidoo_data->momentum_angle;
        if (rot < -SKIDOO_MOMENTUM_TURN) {
            if (rot < -SKIDOO_MAX_MOMENTUM_TURN) {
                rot = -SKIDOO_MAX_MOMENTUM_TURN;
                skidoo_data->momentum_angle = skidoo->rot.y - rot;
            } else {
                skidoo_data->momentum_angle -= SKIDOO_MOMENTUM_TURN;
            }
        } else if (rot > SKIDOO_MOMENTUM_TURN) {
            if (rot > SKIDOO_MAX_MOMENTUM_TURN) {
                rot = SKIDOO_MAX_MOMENTUM_TURN;
                skidoo_data->momentum_angle = skidoo->rot.y - rot;
            } else {
                skidoo_data->momentum_angle += SKIDOO_MOMENTUM_TURN;
            }
        } else {
            skidoo_data->momentum_angle = skidoo->rot.y;
        }
    }

    skidoo->pos.z +=
        (skidoo->speed * Math_Cos(skidoo_data->momentum_angle)) >> W2V_SHIFT;
    skidoo->pos.x +=
        (skidoo->speed * Math_Sin(skidoo_data->momentum_angle)) >> W2V_SHIFT;

    int32_t slip;
    slip = (SKIDOO_SLIP * Math_Sin(skidoo->rot.x)) >> W2V_SHIFT;
    if (ABS(slip) > SKIDOO_SLIP / 2) {
        skidoo->pos.z -= (slip * Math_Cos(skidoo->rot.y)) >> W2V_SHIFT;
        skidoo->pos.x -= (slip * Math_Sin(skidoo->rot.y)) >> W2V_SHIFT;
    }

    slip = (SKIDOO_SLIP_SIDE * Math_Sin(skidoo->rot.z)) >> W2V_SHIFT;
    if (ABS(slip) > SKIDOO_SLIP_SIDE / 2) {
        skidoo->pos.z -= (slip * Math_Sin(skidoo->rot.y)) >> W2V_SHIFT;
        skidoo->pos.x += (slip * Math_Cos(skidoo->rot.y)) >> W2V_SHIFT;
    }

    XYZ_32 moved = {
        .x = skidoo->pos.x,
        .z = skidoo->pos.z,
    };
    if (!(skidoo->flags & IF_ONE_SHOT)) {
        Skidoo_BaddieCollision(skidoo);
    }

    int32_t rot = 0;

    XYZ_32 br;
    XYZ_32 fl;
    XYZ_32 bl;
    XYZ_32 fr;
    const int32_t hbl =
        Skidoo_TestHeight(skidoo, -SKIDOO_FRONT, -SKIDOO_SIDE, &bl);
    if (hbl < bl_old.y - STEP_L) {
        rot = DoShift(skidoo, &bl, &bl_old);
    }
    const int32_t hbr =
        Skidoo_TestHeight(skidoo, -SKIDOO_FRONT, SKIDOO_SIDE, &br);
    if (hbr < br_old.y - STEP_L) {
        rot += DoShift(skidoo, &br, &br_old);
    }
    const int32_t hfl =
        Skidoo_TestHeight(skidoo, SKIDOO_FRONT, -SKIDOO_SIDE, &fl);
    if (hfl < fl_old.y - STEP_L) {
        rot += DoShift(skidoo, &fl, &fl_old);
    }
    const int32_t hfr =
        Skidoo_TestHeight(skidoo, SKIDOO_FRONT, SKIDOO_SIDE, &fr);
    if (hfr < fr_old.y - STEP_L) {
        rot += DoShift(skidoo, &fr, &fr_old);
    }

    int16_t room_num = skidoo->room_num;
    const SECTOR *const sector =
        Room_GetSector(skidoo->pos.x, skidoo->pos.y, skidoo->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, skidoo->pos.x, skidoo->pos.y, skidoo->pos.z);
    if (height < skidoo->pos.y - STEP_L) {
        DoShift(skidoo, &skidoo->pos, &old);
    }

    skidoo_data->extra_rotation = rot;

    int32_t collide = GetCollisionAnim(skidoo, &moved);
    if (collide != 0) {
        const int32_t c = Math_Cos(skidoo_data->momentum_angle);
        const int32_t s = Math_Sin(skidoo_data->momentum_angle);
        const int32_t dx = skidoo->pos.x - old.x;
        const int32_t dz = skidoo->pos.z - old.z;
        const int32_t new_speed = (s * dx + c * dz) >> W2V_SHIFT;

        if (skidoo == Item_Get(g_Lara.skidoo)
            && skidoo->speed > SKIDOO_MAX_SPEED + SKIDOO_ACCELERATION
            && new_speed < skidoo->speed - SKIDOO_ACCELERATION) {
            Lara_TakeDamage((skidoo->speed - new_speed) / 2, true);
        }

        if (skidoo->speed > 0 && new_speed < skidoo->speed) {
            skidoo->speed = new_speed < 0 ? 0 : new_speed;
        } else if (skidoo->speed < 0 && new_speed > skidoo->speed) {
            skidoo->speed = new_speed > 0 ? 0 : new_speed;
        }

        if (skidoo->speed < SKIDOO_MAX_BACK) {
            skidoo->speed = SKIDOO_MAX_BACK;
        }
    }

    return collide;
}

int32_t Skidoo_UserControl(
    ITEM *const skidoo, const int32_t height, int32_t *const out_pitch)
{
    SKIDOO_INFO *const skidoo_data = skidoo->data;

    bool drive = false;

    if (skidoo->pos.y >= height - STEP_L) {
        *out_pitch = skidoo->speed + (height - skidoo->pos.y);

        if (skidoo->speed == 0 && g_Input.look) {
            Lara_LookUpDown();
        }

        if ((g_Input.left && !g_Input.back)
            || (g_Input.right && g_Input.back)) {
            skidoo_data->skidoo_turn -= SKIDOO_TURN;
            CLAMPL(skidoo_data->skidoo_turn, -SKIDOO_MAX_TURN);
        }

        if ((g_Input.right && !g_Input.back)
            || (g_Input.left && g_Input.back)) {
            skidoo_data->skidoo_turn += SKIDOO_TURN;
            CLAMPG(skidoo_data->skidoo_turn, SKIDOO_MAX_TURN);
        }

        if (g_Input.back) {
            if (skidoo->speed > 0) {
                skidoo->speed -= SKIDOO_BRAKE;
            } else {
                if (skidoo->speed > SKIDOO_MAX_BACK) {
                    skidoo->speed += SKIDOO_REVERSE;
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
                    SKIDOO_ACCELERATION * skidoo->speed / (2 * max_speed)
                    + SKIDOO_ACCELERATION / 2;
            } else if (skidoo->speed > max_speed + SKIDOO_SLOWDOWN) {
                skidoo->speed -= SKIDOO_SLOWDOWN;
            }

            drive = true;
        } else if (
            skidoo->speed >= 0 && skidoo->speed < SKIDOO_MIN_SPEED
            && (g_Input.left || g_Input.right)) {
            skidoo->speed = SKIDOO_MIN_SPEED;
            drive = true;
        } else if (skidoo->speed > SKIDOO_SLOWDOWN) {
            skidoo->speed -= SKIDOO_SLOWDOWN;
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
    ITEM *const skidoo = Item_Get(g_Lara.skidoo);

    int16_t rot;
    if (direction == LARA_STATE_SKIDOO_GET_OFF_L) {
        rot = skidoo->rot.y + DEG_90;
    } else {
        rot = skidoo->rot.y - DEG_90;
    }

    const int32_t c = Math_Cos(rot);
    const int32_t s = Math_Sin(rot);
    const int32_t x = skidoo->pos.x - ((SKIDOO_GET_OFF_DIST * s) >> W2V_SHIFT);
    const int32_t z = skidoo->pos.z - ((SKIDOO_GET_OFF_DIST * c) >> W2V_SHIFT);
    const int32_t y = skidoo->pos.y;

    int16_t room_num = skidoo->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int32_t height = Room_GetHeight(sector, x, y, z);

    if (g_HeightType == HT_BIG_SLOPE || height == NO_HEIGHT) {
        return false;
    }

    if (ABS(height - skidoo->pos.y) > WALL_L / 2) {
        return false;
    }

    const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
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
    const SKIDOO_INFO *const skidoo_data = skidoo->data;

    if (skidoo->pos.y != skidoo->floor && skidoo->fall_speed > 0
        && g_LaraItem->current_anim_state != LARA_STATE_SKIDOO_FALL && !dead) {
        Item_SwitchToObjAnim(g_LaraItem, LA_SKIDOO_FALL, 0, O_LARA_SKIDOO);
        g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_FALL;
        g_LaraItem->current_anim_state = LARA_STATE_SKIDOO_FALL;
        return;
    }

    if (collide != 0 && !dead
        && g_LaraItem->current_anim_state != LARA_STATE_SKIDOO_FALL) {
        if (g_LaraItem->current_anim_state != LARA_STATE_SKIDOO_HIT) {
            if (collide == LA_SKIDOO_HIT_FRONT) {
                Sound_Effect(SFX_CLATTER_2, &skidoo->pos, SPM_NORMAL);
            } else {
                Sound_Effect(SFX_CLATTER_1, &skidoo->pos, SPM_NORMAL);
            }
            Item_SwitchToObjAnim(g_LaraItem, collide, 0, O_LARA_SKIDOO);
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_HIT;
            g_LaraItem->current_anim_state = LARA_STATE_SKIDOO_HIT;
        }
        return;
    }

    switch (g_LaraItem->current_anim_state) {
    case LARA_STATE_SKIDOO_SIT:
        if (skidoo->speed == 0) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_STILL;
        }
        if (dead) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_FALLOFF;
        } else if (g_Input.left) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_LEFT;
        } else if (g_Input.right) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_RIGHT;
        }
        break;

    case LARA_STATE_SKIDOO_LEFT:
        if (!g_Input.left) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_SIT;
        }
        break;

    case LARA_STATE_SKIDOO_RIGHT:
        if (!g_Input.right) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_SIT;
        }
        break;

    case LARA_STATE_SKIDOO_FALL:
        if (skidoo->fall_speed <= 0 || skidoo_data->left_fallspeed <= 0
            || skidoo_data->right_fallspeed <= 0) {
            Sound_Effect(SFX_CLATTER_3, &skidoo->pos, SPM_NORMAL);
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_SIT;
        } else if (skidoo->fall_speed > DAMAGE_START + DAMAGE_LENGTH) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_LET_GO;
        }
        break;

    case LARA_STATE_SKIDOO_STILL: {
        const int32_t music_track =
            M_IsArmed(skidoo_data) ? MX_BATTLE_THEME : MX_SKIDOO_THEME;
        const uint16_t music_flags = Music_GetTrackFlags(music_track);
        if (!(music_flags & IF_ONE_SHOT)) {
            Music_Play(music_track, MPM_ALWAYS);
            Music_SetTrackFlags(music_track, music_flags | IF_ONE_SHOT);
        }

        if (dead) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_DEATH;
            return;
        }

        g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_STILL;

        if (g_Input.jump) {
            if (g_Input.right
                && Skidoo_CheckGetOffOK(LARA_STATE_SKIDOO_GET_OFF_R)) {
                g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_GET_OFF_R;
                skidoo->speed = 0;
            } else if (
                g_Input.left
                && Skidoo_CheckGetOffOK(LARA_STATE_SKIDOO_GET_OFF_L)) {
                g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_GET_OFF_L;
                skidoo->speed = 0;
            }
        } else if (g_Input.left) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_LEFT;
        } else if (g_Input.right) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_RIGHT;
        } else if (g_Input.back || g_Input.forward) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_SIT;
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
        effect->object_id = O_EXPLOSION;
    }

    Item_Explode(g_Lara.skidoo, ~(SKIDOO_GUN_MESH - 1), 0);
    Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_NORMAL);
    g_Lara.skidoo = NO_ITEM;
}

int32_t Skidoo_CheckGetOff(void)
{
    ITEM *const skidoo = Item_Get(g_Lara.skidoo);

    if ((g_LaraItem->current_anim_state == LARA_STATE_SKIDOO_GET_OFF_R
         || g_LaraItem->current_anim_state == LARA_STATE_SKIDOO_GET_OFF_L)
        && Item_TestFrameEqual(g_LaraItem, LF_SKIDOO_EXIT_END)) {
        if (g_LaraItem->current_anim_state == LARA_STATE_SKIDOO_GET_OFF_L) {
            g_LaraItem->rot.y += DEG_90;
        } else {
            g_LaraItem->rot.y -= DEG_90;
        }
        Item_SwitchToAnim(g_LaraItem, LA_STAND_STILL, 0);
        g_LaraItem->goal_anim_state = LS_STOP;
        g_LaraItem->current_anim_state = LS_STOP;
        g_LaraItem->pos.x -=
            (SKIDOO_GET_OFF_DIST * Math_Sin(g_LaraItem->rot.y)) >> W2V_SHIFT;
        g_LaraItem->pos.z -=
            (SKIDOO_GET_OFF_DIST * Math_Cos(g_LaraItem->rot.y)) >> W2V_SHIFT;
        g_LaraItem->rot.x = 0;
        g_LaraItem->rot.z = 0;
        g_Lara.skidoo = NO_ITEM;
        g_Lara.gun_status = LGS_ARMLESS;
        return true;
    }

    if (g_LaraItem->current_anim_state == LARA_STATE_SKIDOO_LET_GO
        && (skidoo->pos.y == skidoo->floor
            || Item_TestFrameEqual(g_LaraItem, LF_SKIDOO_LET_GO_END))) {
        Item_SwitchToAnim(g_LaraItem, LA_FREEFALL, 0);
        g_LaraItem->current_anim_state = LARA_STATE_SKIDOO_GET_OFF_R;
        if (skidoo->pos.y == skidoo->floor) {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_STILL;
            g_LaraItem->fall_speed = DAMAGE_START + DAMAGE_LENGTH;
            g_LaraItem->speed = 0;
            Skidoo_Explode(skidoo);
        } else {
            g_LaraItem->goal_anim_state = LARA_STATE_SKIDOO_GET_OFF_R;
            g_LaraItem->pos.y -= 200;
            g_LaraItem->fall_speed = skidoo->fall_speed;
            g_LaraItem->speed = skidoo->speed;
            Sound_Effect(SFX_LARA_FALL, &g_LaraItem->pos, SPM_NORMAL);
        }
        g_LaraItem->rot.x = 0;
        g_LaraItem->rot.z = 0;
        g_LaraItem->gravity = 1;
        g_Lara.gun_status = LGS_ARMLESS;
        g_Lara.move_angle = skidoo->rot.y;
        skidoo->flags |= IF_ONE_SHOT;
        skidoo->collidable = 0;
        return false;
    }

    return true;
}

void Skidoo_Guns(void)
{
    WEAPON_INFO *const winfo = &g_Weapons[LGT_SKIDOO];
    Gun_GetNewTarget(winfo);
    Gun_AimWeapon(winfo, &g_Lara.right_arm);

    if (!g_Input.action) {
        return;
    }

    int16_t angles[2];
    angles[0] = g_Lara.right_arm.rot.y + g_LaraItem->rot.y;
    angles[1] = g_Lara.right_arm.rot.x;

    if (!Gun_FireWeapon(LGT_SKIDOO, g_Lara.target, g_LaraItem, angles)) {
        return;
    }

    g_Lara.right_arm.flash_gun = winfo->flash_time;
    Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
    Gun_AddDynamicLight();

    ITEM *const skidoo = Item_Get(g_Lara.skidoo);
    Creature_Effect(skidoo, &g_Skidoo_LeftGun, Spawn_GunShot);
    Creature_Effect(skidoo, &g_Skidoo_RightGun, Spawn_GunShot);
}

int32_t Skidoo_Control(void)
{
    ITEM *const skidoo = Item_Get(g_Lara.skidoo);
    SKIDOO_INFO *const skidoo_data = skidoo->data;
    int32_t collide = Skidoo_Dynamics(skidoo);

    XYZ_32 fl;
    XYZ_32 fr;
    const int32_t hfl =
        Skidoo_TestHeight(skidoo, SKIDOO_FRONT, -SKIDOO_SIDE, &fl);
    const int32_t hfr =
        Skidoo_TestHeight(skidoo, SKIDOO_FRONT, SKIDOO_SIDE, &fr);

    int16_t room_num = skidoo->room_num;
    const SECTOR *const sector =
        Room_GetSector(skidoo->pos.x, skidoo->pos.y, skidoo->pos.z, &room_num);
    int32_t height =
        Room_GetHeight(sector, skidoo->pos.x, skidoo->pos.y, skidoo->pos.z);

    bool dead = false;
    if (g_LaraItem->hit_points <= 0) {
        dead = true;
        g_Input.back = 0;
        g_Input.forward = 0;
        g_Input.left = 0;
        g_Input.right = 0;
    } else if (g_LaraItem->current_anim_state == LARA_STATE_SKIDOO_LET_GO) {
        dead = true;
        collide = 0;
    }

    int32_t drive;
    int32_t pitch;
    if (skidoo->flags & IF_ONE_SHOT) {
        drive = 0;
        collide = 0;
    } else {
        switch (g_LaraItem->current_anim_state) {
        case LARA_STATE_SKIDOO_GET_ON:
        case LARA_STATE_SKIDOO_GET_OFF_L:
        case LARA_STATE_SKIDOO_GET_OFF_R:
        case LARA_STATE_SKIDOO_LET_GO:
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
        const int32_t pitch = (SOUND_DEFAULT_PITCH - pitch_delta) << 8;

        Sound_Effect(SFX_SKIDOO_MOVING, &skidoo->pos, SPM_PITCH | pitch);
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
        DoDynamics(hfl, skidoo_data->left_fallspeed, &fl.y);
    skidoo_data->right_fallspeed =
        DoDynamics(hfr, skidoo_data->right_fallspeed, &fr.y);
    skidoo->fall_speed = DoDynamics(height, skidoo->fall_speed, &skidoo->pos.y);

    height = (fr.y + fl.y) / 2;
    const int16_t x_rot = Math_Atan(SKIDOO_FRONT, skidoo->pos.y - height);
    const int16_t z_rot = Math_Atan(SKIDOO_SIDE, height - fl.y);
    skidoo->rot.x += (x_rot - skidoo->rot.x) >> 1;
    skidoo->rot.z += (z_rot - skidoo->rot.z) >> 1;

    if (skidoo->flags & IF_ONE_SHOT) {
        Room_TestTriggers(g_LaraItem);
        if (room_num != skidoo->room_num) {
            Item_NewRoom(g_Lara.skidoo, room_num);
        }
        if (skidoo->pos.y == skidoo->floor) {
            Skidoo_Explode(skidoo);
        }
        return 0;
    }

    Skidoo_Animation(skidoo, collide, dead);
    if (room_num != skidoo->room_num) {
        Item_NewRoom(g_Lara.skidoo, room_num);
        Item_NewRoom(g_Lara.item_num, room_num);
    }

    if (g_LaraItem->current_anim_state == LARA_STATE_SKIDOO_FALLOFF) {
        g_LaraItem->rot.x = 0;
        g_LaraItem->rot.z = 0;
    } else {
        g_LaraItem->pos.x = skidoo->pos.x;
        g_LaraItem->pos.y = skidoo->pos.y;
        g_LaraItem->pos.z = skidoo->pos.z;
        g_LaraItem->rot.y = skidoo->rot.y;
        if (drive >= 0) {
            g_LaraItem->rot.x = skidoo->rot.x;
            g_LaraItem->rot.z = skidoo->rot.z;
        } else {
            g_LaraItem->rot.x = 0;
            g_LaraItem->rot.z = 0;
        }
    }
    Room_TestTriggers(g_LaraItem);

    Item_Animate(g_LaraItem);
    if (!dead && drive >= 0 && M_IsArmed(skidoo_data)) {
        Skidoo_Guns();
    }

    if (dead) {
        Item_SwitchToObjAnim(skidoo, LA_SKIDOO_DEAD, 0, O_SKIDOO_FAST);
    } else {
        const int16_t lara_anim_num =
            Item_GetRelativeObjAnim(g_LaraItem, O_LARA_SKIDOO);
        const int16_t lara_frame_num = Item_GetRelativeFrame(g_LaraItem);
        Item_SwitchToObjAnim(
            skidoo, lara_anim_num, lara_frame_num, O_SKIDOO_FAST);
    }

    if (skidoo->speed != 0 && skidoo->floor == skidoo->pos.y) {
        Skidoo_DoSnowEffect(skidoo);
        if (skidoo->speed < SKIDOO_SLOW_SPEED) {
            Skidoo_DoSnowEffect(skidoo);
        }
    }

    return Skidoo_CheckGetOff();
}

void Skidoo_Draw(const ITEM *const item)
{
    int32_t track_mesh_status = 0;
    const SKIDOO_INFO *const skidoo_data = item->data;
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

    // TODO: merge common code parts down below with Object_DrawAnimatingItem.

    ANIM_FRAME *frames[2];
    int32_t rate;
    const int32_t frac = Item_GetFrames(item, frames, &rate);

    Matrix_Push();
    Matrix_TranslateAbs32(item->pos);
    Matrix_Rot16(item->rot);

    const int32_t clip = Output_GetObjectBounds(&frames[0]->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, &frames[0]->bounds);

    if (frac) {
        Matrix_InitInterpolate(frac, rate);
        Matrix_TranslateRel16_ID(frames[0]->offset, frames[1]->offset);
        Matrix_Rot16_ID(frames[0]->mesh_rots[0], frames[1]->mesh_rots[0]);

        Object_DrawMesh(obj->mesh_idx, clip, true);
        for (int32_t mesh_idx = 1; mesh_idx < obj->mesh_count; mesh_idx++) {
            const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
            if (bone->matrix_pop) {
                Matrix_Pop_I();
            }
            if (bone->matrix_push) {
                Matrix_Push_I();
            }

            Matrix_TranslateRel32_I(bone->pos);
            Matrix_Rot16_ID(
                frames[0]->mesh_rots[mesh_idx], frames[1]->mesh_rots[mesh_idx]);

            if (mesh_idx == 1 && track_mesh != nullptr) {
                Output_DrawObjectMesh_I(track_mesh, clip);
            } else {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, true);
            }
        }
    } else {
        Matrix_TranslateRel16(frames[0]->offset);
        Matrix_Rot16(frames[0]->mesh_rots[0]);

        Object_DrawMesh(obj->mesh_idx, clip, false);
        for (int32_t mesh_idx = 1; mesh_idx < obj->mesh_count; mesh_idx++) {
            const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
            if (bone->matrix_pop) {
                Matrix_Pop();
            }
            if (bone->matrix_push) {
                Matrix_Push();
            }

            Matrix_TranslateRel32(bone->pos);
            Matrix_Rot16(frames[0]->mesh_rots[mesh_idx]);

            if (mesh_idx == 1 && track_mesh != nullptr) {
                Output_DrawObjectMesh(track_mesh, clip);
            } else {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, false);
            }
        }
    }

    Matrix_Pop();
}
