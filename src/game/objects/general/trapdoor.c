#include "game/objects/general/trapdoor.h"

#include "game/items.h"
#include "global/const.h"

static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);

static bool M_IsItemOnTop(const ITEM *item, int32_t x, int32_t z)
{
    int32_t tx = item->pos.x >> WALL_SHIFT;
    int32_t tz = item->pos.z >> WALL_SHIFT;
    x >>= WALL_SHIFT;
    z >>= WALL_SHIFT;

    if (item->rot.y == 0 && x == tx && (z == tz || z == tz + 1)) {
        return true;
    } else if (item->rot.y == -PHD_180 && x == tx && (z == tz || z == tz - 1)) {
        return true;
    } else if (item->rot.y == PHD_90 && z == tz && (x == tx || x == tx + 1)) {
        return true;
    } else if (item->rot.y == -PHD_90 && z == tz && (x == tx || x == tx - 1)) {
        return true;
    }
    return false;
}

void TrapDoor_Setup(OBJECT *obj)
{
    obj->control = TrapDoor_Control;
    obj->floor_height_func = TrapDoor_GetFloorHeight;
    obj->ceiling_height_func = TrapDoor_GetCeilingHeight;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void TrapDoor_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DOOR_CLOSED) {
            item->goal_anim_state = DOOR_OPEN;
        }
    } else if (item->current_anim_state == DOOR_OPEN) {
        item->goal_anim_state = DOOR_CLOSED;
    }
    Item_Animate(item);
}

int16_t TrapDoor_GetFloorHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }

    if (item->current_anim_state == DOOR_OPEN || y > item->pos.y
        || item->pos.y >= height) {
        return height;
    }

    return item->pos.y;
}

int16_t TrapDoor_GetCeilingHeight(
    const ITEM *item, const int32_t x, const int32_t y, const int32_t z,
    const int16_t height)
{
    if (!M_IsItemOnTop(item, x, z)) {
        return height;
    }

    if (item->current_anim_state == DOOR_OPEN || y <= item->pos.y
        || item->pos.y <= height) {
        return height;
    }

    return item->pos.y + STEP_L;
}
