#include "game/objects/general/trapdoor.h"

#include "game/items.h"
#include "global/const.h"

#include <stdbool.h>

static bool TrapDoor_StandingOn(ITEM_INFO *item, int32_t x, int32_t z);

static bool TrapDoor_StandingOn(ITEM_INFO *item, int32_t x, int32_t z)
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

void TrapDoor_Setup(OBJECT_INFO *obj)
{
    obj->control = TrapDoor_Control;
    obj->floor_height_routine = TrapDoor_AlterFloorHeight;
    obj->ceiling_height_routine = TrapDoor_AlterCeilingHeight;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void TrapDoor_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DOOR_CLOSED) {
            item->goal_anim_state = DOOR_OPEN;
        }
    } else if (item->current_anim_state == DOOR_OPEN) {
        item->goal_anim_state = DOOR_CLOSED;
    }
    Item_Animate(item);
}

void TrapDoor_AlterFloorHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (!TrapDoor_StandingOn(item, x, z)) {
        return;
    }

    if (item->current_anim_state == DOOR_OPEN || y > item->pos.y
        || item->pos.y >= *height) {
        return;
    }

    *height = item->pos.y;
}

void TrapDoor_AlterCeilingHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height)
{
    if (!TrapDoor_StandingOn(item, x, z)) {
        return;
    }

    if (item->current_anim_state == DOOR_OPEN || y <= item->pos.y
        || item->pos.y <= *height) {
        return;
    }

    *height = item->pos.y + STEP_L;
}
