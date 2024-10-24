#include "game/objects/general/trapdoor.h"

#include "game/items.h"

typedef enum {
    TRAPDOOR_STATE_CLOSED,
    TRAPDOOR_STATE_OPEN,
} TRAPDOOR_STATE;

int32_t __cdecl Trapdoor_IsItemOnTop(
    const ITEM *const item, const int32_t x, const int32_t z)
{
    // trapdoor sector
    const XZ_32 obj = {
        .x = item->pos.x >> WALL_SHIFT,
        .z = item->pos.z >> WALL_SHIFT,
    };

    // test sector
    const XZ_32 test = {
        .x = x >> WALL_SHIFT,
        .z = z >> WALL_SHIFT,
    };

    switch (item->rot.y) {
    case 0:
        return test.x == obj.x && (test.z == obj.z || test.z == obj.z + 1);

    case -PHD_180:
        return test.x == obj.x && (test.z == obj.z || test.z == obj.z - 1);

    case PHD_90:
        return test.z == obj.z && (test.x == obj.x || test.x == obj.x + 1);

    case -PHD_90:
        return test.z == obj.z && (test.x == obj.x || test.x == obj.x - 1);
    }

    return false;
}

void __cdecl Trapdoor_Floor(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_height)
{
    if (!Trapdoor_IsItemOnTop(item, x, z)) {
        return;
    } else if (item->current_anim_state != TRAPDOOR_STATE_CLOSED) {
        return;
    } else if (y > item->pos.y || item->pos.y > *out_height) {
        return;
    }
    *out_height = item->pos.y;
}

void __cdecl Trapdoor_Ceiling(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_height)
{
    if (!Trapdoor_IsItemOnTop(item, x, z)) {
        return;
    } else if (item->current_anim_state != TRAPDOOR_STATE_CLOSED) {
        return;
    } else if (y <= item->pos.y || item->pos.y <= *out_height) {
        return;
    } else {
        *out_height = item->pos.y + STEP_L;
    }
}

void __cdecl Trapdoor_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == TRAPDOOR_STATE_CLOSED) {
            item->goal_anim_state = TRAPDOOR_STATE_OPEN;
        }
    } else {
        if (item->current_anim_state == TRAPDOOR_STATE_OPEN) {
            item->goal_anim_state = TRAPDOOR_STATE_CLOSED;
        }
    }
    Item_Animate(item);
}
