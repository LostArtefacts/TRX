#include "game/objects/creatures/skidoo_driver.h"

#include "game/items.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <assert.h>

void SkidooDriver_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_SKIDOO_DRIVER];
    if (!obj->loaded) {
        return;
    }

    obj->initialise = SkidooDriver_Initialise;
    obj->control = SkidooDriver_Control;

    obj->hit_points = 1;

    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void __cdecl SkidooDriver_Initialise(const int16_t item_num)
{
    ITEM *const skidoo_driver = Item_Get(item_num);

    const int16_t skidoo_item_num = Item_Create();
    assert(skidoo_item_num != NO_ITEM);

    ITEM *const skidoo = Item_Get(skidoo_item_num);
    skidoo->object_id = O_SKIDOO_ARMED;
    skidoo->pos.x = skidoo_driver->pos.x;
    skidoo->pos.y = skidoo_driver->pos.y;
    skidoo->pos.z = skidoo_driver->pos.z;
    skidoo->rot.y = skidoo_driver->rot.y;
    skidoo->room_num = skidoo_driver->room_num;
    skidoo->flags = IF_ONE_SHOT;
    skidoo->shade_1 = -1;
    Item_Initialise(skidoo_item_num);

    skidoo_driver->data = (void *)(intptr_t)skidoo_item_num;
    g_LevelItemCount++;
}
