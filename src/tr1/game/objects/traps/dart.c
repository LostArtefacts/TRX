#include "game/effects.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/spawn.h"
#include "global/vars.h"

#define DART_DAMAGE 50

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = Object_Collision;
    obj->control_func = M_Control;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->touch_bits) {
        Lara_TakeDamage(DART_DAMAGE, true);
        Spawn_Blood(
            item->pos.x, item->pos.y, item->pos.z, g_LaraItem->speed,
            g_LaraItem->rot.y, g_LaraItem->room_num);
    }

    int32_t old_x = item->pos.x;
    int32_t old_z = item->pos.z;
    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (item->pos.y >= item->floor) {
        Item_Kill(item_num);
        int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *effect = Effect_Get(effect_num);
            effect->pos.x = old_x;
            effect->pos.y = item->pos.y;
            effect->pos.z = old_z;
            effect->speed = 0;
            effect->counter = 6;
            effect->frame_num = -3 * Random_GetControl() / 0x8000;
            effect->object_id = O_RICOCHET_1;
        }
    }
}

REGISTER_OBJECT(O_DART, M_Setup)
