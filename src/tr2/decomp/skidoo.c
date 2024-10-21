#include "decomp/skidoo.h"

#include "game/effects.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/math.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

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
    LARA_STATE_SKIDOO_GET_OFF   = 9,
    LARA_STATE_SKIDOO_LETGO     = 10,
    LARA_STATE_SKIDOO_DEATH     = 11,
    LARA_STATE_SKIDOO_FALLOFF   = 12,
    // clang-format on
} LARA_SKIDOO_STATE;

typedef enum {
    // clang-format off
    LA_SKIDOO_GET_ON_L = 1,
    LA_SKIDOO_GET_ON_R = 18,
    // clang-format on
} LARA_ANIM_SKIDOO;

#define SKIDOO_RADIUS 500
#define SKIDOO_SIDE 260
#define SKIDOO_FRONT 550
#define SKIDOO_SNOW 500

#define SKIDOO_MAX_SPEED 100
#define SKIDOO_ACCELERATION 10
#define SKIDOO_SLIP 100
#define SKIDOO_SLIP_SIDE 50

#define SKIDOO_MAX_BACK -30
#define SKIDOO_UNDO_TURN (PHD_DEGREE * 2) // = 364
#define SKIDOO_MOMENTUM_TURN (PHD_DEGREE * 3) // = 546
#define SKIDOO_MAX_MOMENTUM_TURN (PHD_DEGREE * 150) // = 27300

#define SKIDOO_TARGET_DIST (WALL_L * 2) // = 2048

static bool M_IsNearby(const ITEM *item_1, const ITEM *item_2);
static bool M_CheckBaddieCollision(ITEM *item, const ITEM *skidoo);

static bool M_IsNearby(const ITEM *const item_1, const ITEM *const item_2)
{
    const int32_t dx = item_1->pos.x - item_2->pos.x;
    const int32_t dy = item_1->pos.y - item_2->pos.y;
    const int32_t dz = item_1->pos.z - item_2->pos.z;
    return dx > -SKIDOO_TARGET_DIST && dx < SKIDOO_TARGET_DIST
        && dy > -SKIDOO_TARGET_DIST && dy < SKIDOO_TARGET_DIST
        && dz > -SKIDOO_TARGET_DIST && dz < SKIDOO_TARGET_DIST;
}

static bool M_CheckBaddieCollision(ITEM *const item, const ITEM *const skidoo)
{
    if (!item->collidable || item->status == IS_INVISIBLE || item == g_LaraItem
        || item == skidoo) {
        return false;
    }

    const OBJECT *const object = Object_GetObject(item->object_id);
    const bool is_availanche = item->object_id == O_ROLLING_BALL_2;
    if (object->collision == NULL || (!object->intelligent && !is_availanche)) {
        return false;
    }

    if (!M_IsNearby(item, skidoo)) {
        return false;
    }

    if (!Item_TestBoundsCollide(item, skidoo, SKIDOO_RADIUS)) {
        return false;
    }

    if (item->object_id == O_SKIDOO_ARMED) {
        SkidooDriver_Push(item, skidoo, SKIDOO_RADIUS);
    } else if (is_availanche) {
        if (item->current_anim_state == TRAP_ACTIVATE) {
            Lara_TakeDamage(100, true);
        }
    } else {
        DoLotsOfBlood(
            item->pos.x, skidoo->pos.y - STEP_L, item->pos.z, skidoo->speed,
            skidoo->rot.y, item->room_num, 3);
        item->hit_points = 0;
    }
    return true;
}

void __cdecl Skidoo_Initialise(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];

    SKIDOO_INFO *const skidoo_data =
        game_malloc(sizeof(SKIDOO_INFO), GBUF_SKIDOO_STUFF);
    skidoo_data->skidoo_turn = 0;
    skidoo_data->right_fallspeed = 0;
    skidoo_data->left_fallspeed = 0;
    skidoo_data->extra_rotation = 0;
    skidoo_data->momentum_angle = item->rot.y;
    skidoo_data->track_mesh = 0;
    skidoo_data->pitch = 0;

    item->data = skidoo_data;
}

int32_t __cdecl Skidoo_CheckGetOn(const int16_t item_num, COLL_INFO *const coll)
{
    if (!(g_Input & IN_ACTION) || g_Lara.gun_status != LGS_ARMLESS
        || g_LaraItem->gravity) {
        return SKIDOO_GET_ON_NONE;
    }

    ITEM *const item = &g_Items[item_num];
    const int16_t angle = item->rot.y - g_LaraItem->rot.y;

    SKIDOO_GET_ON_SIDE get_on = SKIDOO_GET_ON_NONE;
    if (angle > PHD_45 && angle < PHD_135) {
        get_on = SKIDOO_GET_ON_LEFT;
    } else if (angle > -PHD_135 && angle < -PHD_45) {
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

void __cdecl Skidoo_Collision(
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
    if (get_on == SKIDOO_GET_ON_LEFT) {
        lara_item->anim_num =
            g_Objects[O_LARA_SKIDOO].anim_idx + LA_SKIDOO_GET_ON_L;
    } else if (get_on == SKIDOO_GET_ON_RIGHT) {
        lara_item->anim_num =
            g_Objects[O_LARA_SKIDOO].anim_idx + LA_SKIDOO_GET_ON_R;
    }
    lara_item->current_anim_state = LARA_STATE_SKIDOO_GET_ON;
    lara_item->frame_num = g_Anims[lara_item->anim_num].frame_base;
    g_Lara.gun_status = LGS_ARMLESS;

    ITEM *const item = &g_Items[item_num];
    lara_item->pos.x = item->pos.x;
    lara_item->pos.y = item->pos.y;
    lara_item->pos.z = item->pos.z;
    lara_item->rot.y = item->rot.y;
    item->hit_points = 1;
}

void __cdecl Skidoo_BaddieCollision(const ITEM *const skidoo)
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

int32_t __cdecl Skidoo_TestHeight(
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

void __cdecl Skidoo_DoSnowEffect(ITEM *const skidoo)
{
    const int16_t fx_num = Effect_Create(skidoo->room_num);
    if (fx_num == NO_ITEM) {
        return;
    }

    const int32_t sx = Math_Sin(skidoo->rot.x);
    const int32_t sy = Math_Sin(skidoo->rot.y);
    const int32_t cy = Math_Cos(skidoo->rot.y);
    const int32_t x = (SKIDOO_SIDE * (Random_GetDraw() - 0x4000)) >> 14;
    FX *const fx = &g_Effects[fx_num];
    fx->pos.x = skidoo->pos.x - ((sy * SKIDOO_SNOW + cy * x) >> W2V_SHIFT);
    fx->pos.y = skidoo->pos.y + ((sx * SKIDOO_SNOW) >> W2V_SHIFT);
    fx->pos.z = skidoo->pos.z - ((cy * SKIDOO_SNOW - sy * x) >> W2V_SHIFT);
    fx->room_num = skidoo->room_num;
    fx->object_id = O_SNOW_SPRITE;
    fx->frame_num = 0;
    fx->speed = 0;
    if (skidoo->speed < 64) {
        fx->fall_speed = (Random_GetDraw() * ABS(skidoo->speed)) >> 15;
    } else {
        fx->fall_speed = 0;
    }

    g_MatrixPtr->_23 = 0;

    S_CalculateLight(fx->pos.x, fx->pos.y, fx->pos.z, fx->room_num);
    fx->shade = g_LsAdder - 512;
    CLAMPL(fx->shade, 0);
}

int32_t __cdecl Skidoo_Dynamics(ITEM *const skidoo)
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
