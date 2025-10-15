#include "game/lara.h"
#include "game/objects/common.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/spawn.h"

#define SPIKE_CEILING_DAMAGE 20
#define SPIKE_CEILING_SPEED 1

static void M_Move(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const int32_t y = item->pos.y + 5;
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, y, item->pos.z, &room_num);
    if (Room_GetHeight(sector, item->pos.x, y, item->pos.z) < y + WALL_L) {
        item->status = IS_DEACTIVATED;
    } else {
        item->pos.y = y;
        Item_UpdateRoom(item_num, room_num);
    }
    Sound_Effect(SFX_DOOR_SLIDE, &item->pos, SPM_NORMAL);
}

static void M_HitLara(ITEM *const item)
{
    Lara_TakeDamage(SPIKE_CEILING_DAMAGE, true);

    const ITEM *const lara_item = Lara_GetItem();
    Spawn_BloodBath(
        lara_item->pos.x, item->pos.y + LARA_HEIGHT, lara_item->pos.z,
        SPIKE_CEILING_SPEED, item->rot.y, lara_item->room_num, 3);
    item->touch_bits = 0;

    Sound_Effect(SFX_LARA_FLESH_WOUND, &item->pos, SPM_NORMAL);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    const bool is_trigger_active = Item_IsTriggerActive(item);
    if (is_trigger_active && item->status != IS_DEACTIVATED) {
        M_Move(item_num);
    }

    if (item->touch_bits) {
        M_HitLara(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->save_position = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_CEILING_SPIKES, M_Setup)
