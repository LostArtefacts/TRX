#include "game/game_buf.h"
#include "game/lara.h"
#include "game/math.h"
#include "game/objects/common.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/spawn.h"

#define SPIKE_WALL_DAMAGE 20
#define SPIKE_WALL_SPEED 1

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    GAME_VECTOR *const data =
        GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_ITEM_DATA);
    data->pos = item->pos;
    data->room_num = item->room_num;
    item->data = data;
}

static void M_Move(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const int32_t z =
        item->pos.z + (SPIKE_WALL_SPEED * Math_Cos(item->rot.y) >> WALL_SHIFT);
    const int32_t x =
        item->pos.x + (SPIKE_WALL_SPEED * Math_Sin(item->rot.y) >> WALL_SHIFT);

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, item->pos.y, z, &room_num);

    if (Room_GetHeight(sector, x, item->pos.y, z) != item->pos.y) {
        item->status = IS_DEACTIVATED;
    } else {
        item->pos.z = z;
        item->pos.x = x;
        Item_UpdateRoom(item_num, room_num);
    }

    Sound_Effect(SFX_DOOR_SLIDE, &item->pos, SPM_NORMAL);
}

static void M_Reset(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->status = IS_INACTIVE;

    const GAME_VECTOR *const data = item->data;
    item->pos = data->pos;
    if (item->room_num != data->room_num) {
        Item_RemoveDrawn(item_num);
        ROOM *const room = Room_Get(data->room_num);
        item->next_item = room->item_num;
        room->item_num = item_num;
        item->room_num = data->room_num;
    }

    Item_RemoveActive(item_num);
}

static void M_HitLara(ITEM *const item)
{
    Lara_TakeDamage(SPIKE_WALL_DAMAGE, true);

    const ITEM *const lara_item = Lara_GetItem();
    Spawn_BloodBath(
        lara_item->pos.x, lara_item->pos.y - WALL_L / 2, lara_item->pos.z,
        SPIKE_WALL_SPEED, item->rot.y, lara_item->room_num, 3);
    item->touch_bits = 0;

    Sound_Effect(SFX_LARA_FLESH_WOUND, &item->pos, SPM_NORMAL);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_IsTriggerActive(item)) {
        M_Reset(item_num);
    } else if (item->status != IS_DEACTIVATED) {
        M_Move(item_num);
    }

    if (item->touch_bits) {
        M_HitLara(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->save_position = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_SPIKE_WALL, M_Setup)
