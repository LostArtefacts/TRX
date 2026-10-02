#include <trx/game/lara.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

#define M_INTACT_BITS (ITEM_MESH(0))

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->mesh_bits = M_INTACT_BITS;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item) || !Lara_Vehicle_IsMounted()) {
        return;
    }

    const ITEM *const vehicle = Lara_Vehicle_GetItem();
    if (!Item_TestBoundsCollide(item, vehicle, WALL_L)) {
        return;
    }

    Sound_Effect(SFX_VEHICLE_HIT_OBJECT, &item->pos, SPM_NORMAL);
    item->mesh_bits = -1;
    Item_Shatter(
        item_num,
        (ITEM_SHATTER_ARGS) {
            .mesh_bits = ~M_INTACT_BITS,
            .gib_flags = GIB_DEBRIS,
        });
    Room_TestTriggers(item);
    Item_Destroy(item_num);
    Item_SetFinished(item, true);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Object_Collision;
    obj->control_func = M_Control;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_SMASHABLE_VEHICLE_WALL, M_Setup)
