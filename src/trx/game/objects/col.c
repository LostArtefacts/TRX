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
    const XYZ_32 shift = XYZ_32_Subtract(lara_pos, lara_item->pos);
    if ((shift.x != 0 || shift.y != 0 || shift.z != 0)
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
