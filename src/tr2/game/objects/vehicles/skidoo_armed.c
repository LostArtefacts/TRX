#include "game/collide.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/objects/creatures/skidoo_driver.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

#define SKIDOO_ARMED_RADIUS (WALL_L / 3) // = 341

static void M_Setup(OBJECT *obj);
static void M_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;

    obj->hit_points = SKIDOO_DRIVER_HITPOINTS;
    obj->radius = SKIDOO_ARMED_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(
            item, lara_item, coll, item->speed > 0 ? coll->enable_hit : false,
            false);
    }

    if (g_Lara.skidoo == NO_ITEM && item->speed > 0) {
        Lara_TakeDamage(100, true);
    }
}

void SkidooArmed_Push(
    const ITEM *const item, ITEM *const lara_item, const int32_t radius)
{
    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    const int32_t cy = Math_Cos(item->rot.y);
    const int32_t sy = Math_Sin(item->rot.y);

    int32_t rx = (cy * dx - sy * dz) >> W2V_SHIFT;
    int32_t rz = (sy * dx + cy * dz) >> W2V_SHIFT;

    const ANIM_FRAME *const best_frame = Item_GetBestFrame(item);
    BOUNDS_16 bounds = {
        .min.x = best_frame->bounds.min.x - radius,
        .max.x = best_frame->bounds.max.x + radius,
        .min.z = best_frame->bounds.min.z - radius,
        .max.z = best_frame->bounds.max.z + radius,
    };

    if (rx < bounds.min.x || rx > bounds.max.x || rz < bounds.min.z
        || rz > bounds.max.z) {
        return;
    }

    const int32_t r = bounds.max.x - rx;
    const int32_t l = rx - bounds.min.x;
    const int32_t t = bounds.max.z - rz;
    const int32_t b = rz - bounds.min.z;
    if (l <= r && l <= t && l <= b) {
        rx -= l;
    } else if (r <= l && r <= t && r <= b) {
        rx += r;
    } else if (t <= l && t <= r && t <= b) {
        rz += t;
    } else {
        rz -= b;
    }

    lara_item->pos.x = item->pos.x + ((rz * sy + rx * cy) >> W2V_SHIFT);
    lara_item->pos.z = item->pos.z + ((rz * cy - rx * sy) >> W2V_SHIFT);
}

REGISTER_OBJECT(O_SKIDOO_ARMED, M_Setup)
