#include <trx/game/objects/col.h>

#include <trx/config.h>
#include <trx/game/lara.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

void Object_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Col_ItemPush(item, coll, false, true);
    }
}

void Object_Collision_Trap(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsInPlay(item)) {
        if (Lara_TestBoundsCollide(item, coll->radius)) {
            Collide_TestCollision(item, lara_item);
        }
    } else if (item->is_visible) {
        Object_Collision(item_num, lara_item, coll);
    }
}

void Object_Collision_BoxTrap(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll,
    const int32_t damage)
{
    if (damage == 0) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (!item->is_visible) {
        return;
    }

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }

    const XYZ_32 lara_pos = lara_item->pos;
    if (!Lara_Col_ItemPush(item, coll, true, true)) {
        return;
    }

    Lara_TakeDamage(damage, false);
    if (!XYZ_32_AreEquivalent(lara_pos, lara_item->pos)
        && Item_IsTriggerActive(item)
        && !g_Config.debug.enable_invulnerability) {
        const XYZ_32 blood_pos = {
            .x = lara_item->pos.x + (Random_GetControl() & 0x3F) - (STEP_L / 8),
            .y = lara_item->pos.y - (Random_GetControl() & 0x1FF) - STEP_L,
            .z = lara_item->pos.z + (Random_GetControl() & 0x3F) - (STEP_L / 8),
        };
        Spawn_Blood(
            blood_pos.x, blood_pos.y, blood_pos.z,
            (damage >> 5) + (Random_GetControl() & 0x3) + 2,
            Random_GetControl() * 2, lara_item->room_num);
    }

    if (!coll->enable_baddie_push) {
        lara_item->pos = lara_pos;
    }
}

void Object_Collision_SphereTrap(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll,
    const int32_t damage, int32_t deadly_mesh_bits,
    const bool ignore_first_sphere)
{
    ITEM *const item = Item_Get(item_num);
    if (!item->is_visible) {
        return;
    }

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }

    int32_t touch_bits = Collide_TestCollision(item, lara_item);
    if (touch_bits == 0) {
        return;
    }

    const int16_t rot = item->rot.y;
    item->rot.y = 0;
    SPHERE spheres[34] = {};
    Collide_GetSpheres(item, spheres, true);
    item->rot.y = rot;

    if (ignore_first_sphere) {
        touch_bits &= ~0x1;
    }

    if (touch_bits == 0) {
        return;
    }

    const SPHERE *sphere = &spheres[0];
    do {
        if ((touch_bits & 0x1) == 0) {
            goto loop_end;
        }

        const XYZ_32 lara_pos = lara_item->pos;
        const BOUNDS_16 bounds = {
            .min.x = sphere->pos.x - item->pos.x - sphere->r,
            .max.x = sphere->pos.x - item->pos.x + sphere->r,
            .min.y = sphere->pos.y - item->pos.y - sphere->r,
            .max.y = sphere->pos.y - item->pos.y + sphere->r,
            .min.z = sphere->pos.z - item->pos.z - sphere->r,
            .max.z = sphere->pos.z - item->pos.z + sphere->r,
        };
        if (!Lara_Col_ItemPushEx(
                item, coll, (deadly_mesh_bits & 0x1) != 0, true, &bounds)) {
            goto loop_end;
        }

        if ((deadly_mesh_bits & 0x1) == 0) {
            goto loop_end;
        }

        Lara_TakeDamage(damage, false);
        if (!XYZ_32_AreEquivalent(lara_pos, lara_item->pos)
            && Item_IsTriggerActive(item)
            && !g_Config.debug.enable_invulnerability) {
            const XYZ_32 blood_pos = {
                .x = lara_item->pos.x + (Random_GetControl() & 0x3F)
                    - (STEP_L / 8),
                .y = sphere->pos.y - (Random_GetControl() & 0x1F)
                    - (STEP_L / 16),
                .z = lara_item->pos.z + (Random_GetControl() & 0x3F)
                    - (STEP_L / 8),
            };
            Spawn_Blood(
                blood_pos.x, blood_pos.y, blood_pos.z,
                (damage >> 5) + (Random_GetControl() & 0x3) + 2,
                Random_GetControl() * 2, lara_item->room_num);
        }

        if (!coll->enable_baddie_push) {
            lara_item->pos = lara_pos;
        }

    loop_end:
        deadly_mesh_bits >>= 1;
        touch_bits >>= 1;
        sphere++;
    } while (touch_bits != 0);
}
