#include "game/objects/traps/movable_block.h"

#include "game/const.h"
#include "game/game_buf.h"
#include "game/items.h"
#include "game/items/walkable.h"
#include "game/objects/vars.h"
#include "game/pathing.h"
#include "vector.h"

#include <stdlib.h>

static int32_t M_CompareBlock(const void *item_idx1, const void *item_idx2);
static bool M_IsValidFloorShiftState(const ITEM *item);

static int32_t M_CompareBlock(const void *item_idx1, const void *item_idx2)
{
    const ITEM *const item1 = Item_Get(*(int16_t *)item_idx1);
    const ITEM *const item2 = Item_Get(*(int16_t *)item_idx2);
    if (item1->pos.y == item2->pos.y) {
        return 0;
    }
    return item1->pos.y < item2->pos.y ? 1 : -1;
}

static bool M_IsValidFloorShiftState(const ITEM *const item)
{
    return (item->status == IS_INACTIVE
            || (item->status == IS_ACTIVE && !(bool)(intptr_t)item->priv))
        && (item->flags & IF_KILLED) == 0
        && item->pos.y >= Item_GetHeight(item);
}

// TODO: make private once M_Setup can be migrated
void MovableBlock_Initialise(const int16_t item_num)
{
    // Ensure the block is snapped to the grid, otherwise the snapping occurs
    // during collision tests and can appear jarring. Additional angles are
    // stored to preserve item appearance in spite of control angle changes.
    ITEM *const item = Item_Get(item_num);
    MOVABLE_BLOCK_INFO *const data =
        GameBuf_Alloc(sizeof(MOVABLE_BLOCK_INFO), GBUF_ITEM_DATA);
    item->data = data;
    data->original_rot =
        (((item->rot.y + DEG_180) / DEG_90) * DEG_90) - DEG_180;
    MovableBlock_UpdateRotation(item, data->original_rot);
    data->gravity_frames = 0;
    data->is_push_pull = false;
    MovableBlock_UpdateBox(item, true);
    MovableBlock_SetLinked(item);
}

// TODO: make private
void MovableBlock_UpdateRotation(ITEM *const item, const int16_t rot_y)
{
    item->rot.y = rot_y;
    MOVABLE_BLOCK_INFO *const data = item->data;
    data->counter_rot[0] = data->original_rot - rot_y;
}

// TODO: make private
void MovableBlock_UpdateBox(const ITEM *const item, const bool blocked)
{
    // TODO Might be other cases...
    if (blocked
        && (item->status == IS_ACTIVE || item->status == IS_INVISIBLE
            || (item->flags & IF_KILLED) != 0)) {
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (sector->floor.height == item->pos.y && sector->box != NO_BOX) {
        BOX_INFO *const box = Box_GetBox(sector->box);
        if ((box->overlap_index & BOX_BLOCKABLE) != 0) {
            if (blocked) {
                box->overlap_index |= BOX_BLOCKED;
            } else {
                box->overlap_index &= ~BOX_BLOCKED;
            }
        }
    }
}

void MovableBlock_SetPushPull(ITEM *const item, const bool enable)
{
    MOVABLE_BLOCK_INFO *const data = item->data;
    data->is_push_pull = enable;
}

bool MovableBlock_IsPushPull(const ITEM *const item)
{
    const MOVABLE_BLOCK_INFO *const data = item->data;
    return data ? data->is_push_pull : false;
}

void MovableBlock_SetGravityFrames(ITEM *const item, const uint8_t frames)
{
    MOVABLE_BLOCK_INFO *const data = item->data;
    data->gravity_frames = frames;
}

uint16_t MovableBlock_GetGravityFrames(const ITEM *const item)
{
    const MOVABLE_BLOCK_INFO *const data = item->data;
    return data ? data->gravity_frames : 0;
}

void MovableBlock_SetLinked(ITEM *const item)
{
    MOVABLE_BLOCK_INFO *const data = item->data;
    data->linked.pos = item->pos;
    data->linked.room_num = item->room_num;
}

GAME_VECTOR MovableBlock_GetLinked(const ITEM *const item)
{
    const MOVABLE_BLOCK_INFO *const data = item->data;
    return data->linked;
}

void MovableBlock_DropStack(
    int32_t stack_height, const XYZ_32 sector_pos, int16_t room_num)
{
    SECTOR *sector =
        Room_GetSector(sector_pos.x, sector_pos.y, sector_pos.z, &room_num);
    sector = Room_GetPitSector(sector, sector_pos.x, sector_pos.z);

    // Check for a stack of movable blocks and trigger each one.
    VECTOR *stack = Vector_Create(sizeof(int16_t));
    for (WALKABLE *w = sector->walkable; w != nullptr; w = w->next) {
        const ITEM *const item = Item_Get(w->item_num);
        if (Object_IsType(item->object_id, g_MovableBlockObjects)) {
            const OBJECT *const obj = Object_Get(item->object_id);
            if (w->pos.x == sector_pos.x && w->pos.y == stack_height
                && w->pos.z == sector_pos.z) {
                Vector_Add(stack, (void *)&w->item_num);
                stack_height -= WALL_L;
            }
        }
    }

    for (int16_t i = stack->count - 1; i >= 0; i--) {
        const int16_t item_num = *(const int16_t *)Vector_Get(stack, i);
        ITEM *const item = Item_Get(item_num);
        MovableBlock_SetGravityFrames(item, i);
        item->status = IS_ACTIVE;
        Item_AddActive(item_num);
        Item_Animate(item);
    }

    Vector_Free(stack);
    stack = nullptr;
}

void MovableBlock_SlideStack(
    int32_t stack_height, const XYZ_32 old_pos, const ITEM *const dest_item,
    const bool reposition)
{
    int16_t room_num = dest_item->room_num;
    SECTOR *sector = Room_GetSector(old_pos.x, old_pos.y, old_pos.z, &room_num);
    sector = Room_GetPitSector(sector, old_pos.x, old_pos.z);

    // Check for a stack of movable blocks and trigger each one.
    VECTOR *stack = Vector_Create(sizeof(int16_t));
    for (WALKABLE *w = sector->walkable; w != nullptr; w = w->next) {
        const ITEM *const item = Item_Get(w->item_num);
        if (Object_IsType(item->object_id, g_MovableBlockObjects)) {
            const OBJECT *const obj = Object_Get(item->object_id);
            if (w->pos.x == old_pos.x && w->pos.y == stack_height
                && w->pos.z == old_pos.z) {
                Vector_Add(stack, (void *)&w->item_num);
                stack_height -= WALL_L;
            }
        }
    }

    for (int16_t i = 0; i < stack->count; i++) {
        const int16_t item_num = *(const int16_t *)Vector_Get(stack, i);
        ITEM *const item = Item_Get(item_num);
        item->status = IS_ACTIVE;
        item->pos.x = dest_item->pos.x;
        item->pos.z = dest_item->pos.z;
        if (reposition) {
            GAME_VECTOR target = {
                .pos = item->pos,
                .room_num = item->room_num,
            };
            Walkable_Reposition(item_num, MovableBlock_GetLinked(item), target);
            MovableBlock_SetLinked(item);
            item->status = IS_INACTIVE;
        }
    }

    Vector_Free(stack);
    stack = nullptr;
}
