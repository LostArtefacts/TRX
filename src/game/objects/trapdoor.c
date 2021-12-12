#include "game/objects/trapdoor.h"

#include "game/control.h"
#include "global/vars.h"

void SetupTrapDoor(OBJECT_INFO *obj)
{
    obj->control = TrapDoorControl;
    obj->floor = TrapDoorFloor;
    obj->ceiling = TrapDoorCeiling;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void TrapDoorControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (TriggerActive(item)) {
        if (item->current_anim_state == DOOR_CLOSED) {
            item->goal_anim_state = DOOR_OPEN;
        }
    } else if (item->current_anim_state == DOOR_OPEN) {
        item->goal_anim_state = DOOR_CLOSED;
    }
    AnimateItem(item);
}

void TrapDoorFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int32_t *height)
{
    if (!OnTrapDoor(item, x, z)) {
        return;
    }
    if (y <= item->pos.y && item->current_anim_state == DOOR_CLOSED
        && item->pos.y < *height) {
        *height = item->pos.y;
    }
}

void TrapDoorCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int32_t *height)
{
    if (!OnTrapDoor(item, x, z)) {
        return;
    }
    if (y > item->pos.y && item->current_anim_state == DOOR_CLOSED
        && item->pos.y > *height) {
        *height = (int16_t)item->pos.y + STEP_L;
    }
}

int32_t OnTrapDoor(ITEM_INFO *item, int32_t x, int32_t z)
{
    x >>= WALL_SHIFT;
    z >>= WALL_SHIFT;
    int32_t tx = item->pos.x >> WALL_SHIFT;
    int32_t tz = item->pos.z >> WALL_SHIFT;
    if (item->pos.y_rot == 0 && x == tx && (z == tz || z == tz + 1)) {
        return 1;
    } else if (
        item->pos.y_rot == -PHD_180 && x == tx && (z == tz || z == tz - 1)) {
        return 1;
    } else if (
        item->pos.y_rot == PHD_90 && z == tz && (x == tx || x == tx + 1)) {
        return 1;
    } else if (
        item->pos.y_rot == -PHD_90 && z == tz && (x == tx || x == tx - 1)) {
        return 1;
    }
    return 0;
}
