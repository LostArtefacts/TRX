#include "game/lara.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/spawn.h"
#include "version.h"

#define M_DAMAGE (g_TRVersion == 1 ? 100 : 50)

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    const bool working =
        g_TRVersion != 1 || item->current_anim_state == TRAP_WORKING;
    if (g_TRVersion == 1) {
        if (Item_IsTriggerActive(item)) {
            if (item->current_anim_state == TRAP_SET) {
                item->goal_anim_state = TRAP_WORKING;
            }
        } else {
            if (item->current_anim_state == TRAP_WORKING) {
                item->goal_anim_state = TRAP_SET;
            }
        }
    }

    if (working && item->touch_bits != 0) {
        Lara_TakeDamage(M_DAMAGE, true);

        const ITEM *const lara_item = Lara_GetItem();
        const XYZ_32 pos = {
            .x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256,
            .z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256,
            .y = lara_item->pos.y - Random_GetControl() / 44,
        };
        Spawn_Blood(
            pos.x, pos.y, pos.z, lara_item->speed,
            lara_item->rot.y + (Random_GetControl() - 0x4000) / 8,
            lara_item->room_num);
    }

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &item->room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func =
        g_TRVersion == 1 ? Object_Collision_Trap : Object_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_PENDULUM_1, M_Setup)
REGISTER_OBJECT(O_PENDULUM_2, M_Setup)
