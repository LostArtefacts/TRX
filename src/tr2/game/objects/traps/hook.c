#include "game/creature.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/spawn.h"

#include <libtrx/game/lara/common.h>

#define HOOK_DAMAGE 50

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->touch_bits != 0) {
        const ITEM *const lara_item = Lara_GetItem();
        Lara_TakeDamage(HOOK_DAMAGE, true);
        Spawn_BloodBath(
            lara_item->pos.x, lara_item->pos.y - WALL_L / 2, lara_item->pos.z,
            lara_item->speed, lara_item->rot.y, lara_item->room_num, 3);
    }

    Item_Animate(item);
}

REGISTER_OBJECT(O_HOOK, M_Setup)
