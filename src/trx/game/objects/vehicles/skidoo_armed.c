#include <trx/core/math.h>
#include <trx/game/collision.h>
#include <trx/game/lara.h>
#include <trx/game/objects/creatures/skidoo_driver.h>
#include <trx/game/pathing.h>

#define M_ARMED_RADIUS (WALL_L / 3) // = 341

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
        Lara_Col_ItemPush(
            item, coll, item->speed > 0 ? coll->enable_hit : false, false);
    }

    if (!Lara_Vehicle_IsMounted() && item->speed > 0) {
        Lara_TakeDamage(100, true);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;

    obj->radius = M_ARMED_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;
    obj->lot_setup = LOT_Setup(LOT_SETUP_JUMPER);

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(SKIDOO_DRIVER_HITPOINTS));
}

void SkidooArmed_Push(
    const ITEM *const item, ITEM *const lara_item, const int32_t radius)
{
    const XYZ_32 local = XYZ_32_UnrotateYaw(
        XYZ_32_Subtract(lara_item->pos, item->pos), item->rot.y);
    int32_t rx = local.x;
    int32_t rz = local.z;

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

    const XYZ_32 pushed = XYZ_32_OffsetLocalYaw(
        item->pos, (XYZ_32) { .x = rx, .z = rz }, item->rot.y);
    lara_item->pos.x = pushed.x;
    lara_item->pos.z = pushed.z;
}

REGISTER_OBJECT(O_SKIDOO_ARMED, M_Setup)
