#include <trx/game/game_flow.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

static void M_ElectrocuteSophia(ITEM *const item)
{
    Room_TestTriggers(item);

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const other_item = Item_Get(i);
        if (other_item->object_id == O_SOPHIA) {
            // TODO
            // other_item->item_flags[2] = 1;
            break;
        }
    }
}

static bool M_CanTakeDamage(const ITEM *const item)
{
    return Item_TestAnimEqual(item, 0);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->hit_points <= 0 && Item_TestAnimEqual(item, 0)) {
        Item_SwitchToAnim(item, 1, 0);

        XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, 420, item->rot.y + DEG_180);
        pos.y -= 768;
        Sparks_TriggerExplosionSparks(pos, 2, 0, 0, item->room_num);
        Sparks_TriggerExplosionSmoke(pos, 0, item->room_num);
        M_ElectrocuteSophia(item);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->can_take_damage_func = M_CanTakeDamage;

    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_FUSE_BOX, M_Setup)
