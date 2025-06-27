#include "game/objects/traps/movable_block.h"

#include "game/const.h"
#include "game/game_buf.h"
#include "game/items.h"
#include "game/objects/vars.h"
#include "game/pathing.h"
#include "log.h"
#include "vector.h"

#include <stdlib.h>

static int32_t m_BlockCount = 0;
static VECTOR *m_UnsortedBlocks = nullptr;
static int16_t *m_SortedBlocks = nullptr;

static int32_t M_CompareBlock(const void *item_idx1, const void *item_idx2);
static bool M_IsValidFloorShiftState(const ITEM *item);
static void M_ShiftGlobalFloorUp(void);
static void M_ShiftGlobalFloorDown(void);
static int32_t M_GetSectorIndex(int32_t x, int32_t divisor);

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

static void M_ShiftGlobalFloorUp(void)
{
    for (int32_t i = 0; i < m_BlockCount; i++) {
        ITEM *const item = Item_Get(m_SortedBlocks[i]);
        if (M_IsValidFloorShiftState(item)) {
            Room_AlterFloorHeight(item, -WALL_L);
        }
    }
}

static void M_ShiftGlobalFloorDown(void)
{
    for (int32_t i = m_BlockCount - 1; i >= 0; i--) {
        ITEM *const item = Item_Get(m_SortedBlocks[i]);
        if (M_IsValidFloorShiftState(item)) {
            Room_AlterFloorHeight(item, WALL_L);
        }
    }
}

static int32_t M_GetSectorIndex(int32_t x, int32_t divisor)
{
    return (x >= 0) ? x / divisor : -((-x + divisor - 1) / divisor);
}

// TODO: make private once M_Setup can be migrated
void MovableBlock_Initialise(const int16_t item_num)
{
    if (m_UnsortedBlocks == nullptr) {
        m_UnsortedBlocks = Vector_Create(sizeof(int16_t));
    }
    Vector_Add(m_UnsortedBlocks, (void *)&item_num);

    // Ensure the block is snapped to the grid, otherwise the snapping occurs
    // during collision tests and can appear jarring. Additional angles are
    // stored to preserve item appearance in spite of control angle changes.
    ITEM *const item = Item_Get(item_num);
    MovableBlock_Info *const data =
        GameBuf_Alloc(sizeof(MovableBlock_Info), GBUF_ITEM_DATA);
    item->data = data;
    data->original_rot =
        (((item->rot.y + DEG_180) / DEG_90) * DEG_90) - DEG_180;
    MovableBlock_UpdateRotation(item, data->original_rot);
    data->gravity_frames = 0;
    data->is_push_pull = false;
    MovableBlock_UpdateBox(item, true);
}

// TODO: make private
void MovableBlock_UpdateRotation(ITEM *const item, const int16_t rot_y)
{
    item->rot.y = rot_y;
    MovableBlock_Info *const data = (MovableBlock_Info *)item->data;
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
        if (blocked) {
            box->overlap_index |= BOX_BLOCKED;
        } else {
            box->overlap_index &= ~BOX_BLOCKED;
        }
    }
}

void MovableBlock_SetupFloor(void)
{
    if (m_UnsortedBlocks == nullptr || m_UnsortedBlocks->count == 0) {
        m_BlockCount = 0;
        m_SortedBlocks = nullptr;
        m_UnsortedBlocks = nullptr;
        return;
    }

    m_BlockCount = m_UnsortedBlocks->count;
    m_SortedBlocks =
        GameBuf_Alloc(m_BlockCount * sizeof(int16_t), GBUF_ITEM_DATA);
    for (int32_t i = 0; i < m_BlockCount; i++) {
        m_SortedBlocks[i] = *(const int16_t *)Vector_Get(m_UnsortedBlocks, i);
    }

    qsort(m_SortedBlocks, m_BlockCount, sizeof(int16_t), M_CompareBlock);
    // M_ShiftGlobalFloorUp();

    Vector_Free(m_UnsortedBlocks);
    m_UnsortedBlocks = nullptr;
}

void MovableBlock_HandleFlipMap(const ROOM_FLIP_STATUS flip_status)
{
    if (m_UnsortedBlocks != nullptr) {
        return;
    }

    if (flip_status == RFS_FLIPPED) {
        // M_ShiftGlobalFloorUp();
    } else {
        // M_ShiftGlobalFloorDown();
    }
}

void MovableBlock_SetPushPull(ITEM *const item, const bool enable)
{
    MovableBlock_Info *const data = (MovableBlock_Info *)item->data;
    data->is_push_pull = enable;
}

bool MovableBlock_IsPushPull(const ITEM *const item)
{
    const MovableBlock_Info *const data = (MovableBlock_Info *)item->data;
    return data ? data->is_push_pull : false;
}

void MovableBlock_SetGravityFrames(ITEM *const item, const uint8_t frames)
{
    MovableBlock_Info *const data = (MovableBlock_Info *)item->data;
    data->gravity_frames = frames;
}

uint16_t MovableBlock_GetGravityFrames(const ITEM *const item)
{
    const MovableBlock_Info *const data = (MovableBlock_Info *)item->data;
    return data ? data->gravity_frames : 0;
}

void MovableBlock_ActivateSectors(const ITEM *const item)
{
    const BOUNDS_16 rot_bounds = Item_RotateBounds(item, item->rot.y);

    // World space coordinates
    const int32_t x0 = item->pos.x + rot_bounds.min.x;
    const int32_t x1 = item->pos.x + rot_bounds.max.x;
    const int32_t z0 = item->pos.z + rot_bounds.min.z;
    const int32_t z1 = item->pos.z + rot_bounds.max.z;

    // Convert to sector indices
    int32_t sx0 = M_GetSectorIndex(x0, WALL_L);
    int32_t sx1 = M_GetSectorIndex(x1, WALL_L);
    int32_t sz0 = M_GetSectorIndex(z0, WALL_L);
    int32_t sz1 = M_GetSectorIndex(z1, WALL_L);

    // Swap direction if needed
    if (sx0 > sx1) {
        int32_t t = sx0;
        sx0 = sx1;
        sx1 = t;
    }
    if (sz0 > sz1) {
        int32_t t = sz0;
        sz0 = sz1;
        sz1 = t;
    }

    // Walk every covered sector
    for (int32_t sx = sx0; sx <= sx1; ++sx) {
        for (int32_t sz = sz0; sz <= sz1; ++sz) {
            const XYZ_32 pos = {
                .x = sx * WALL_L + WALL_L / 2,
                .y = item->pos.y,
                .z = sz * WALL_L + WALL_L / 2,
            };
            MovableBlock_ActivateStack(item->pos.y, pos);
        }
    }
}

void MovableBlock_ActivateStack(int32_t stack_height, const XYZ_32 sector_pos)
{
    int16_t *triggered_items =
        (int16_t *)malloc((size_t)Item_GetWalkableCount() * sizeof(int16_t));
    int32_t triggered_count = 0;

    // Check for a stack of movable blocks and trigger each one.
    for (int32_t i = 0; i < Item_GetWalkableCount(); i++) {
        const int16_t item_num = Item_GetWalkableNum(i);
        ITEM *const item = Item_Get(item_num);
        if (Object_IsType(item->object_id, g_MovableBlockObjects)) {
            const OBJECT *const obj = Object_Get(item->object_id);
            if (obj->floor_height_func != nullptr) {
                if (item->pos.x == sector_pos.x && item->pos.y == stack_height
                    && item->pos.z == sector_pos.z) {
                    stack_height -= WALL_L;
                    triggered_items[triggered_count++] = item_num;
                    LOG_DEBUG(
                        "Add item_num: %d to stack. stack_height now: %d",
                        item_num, stack_height);
                }
            }
        }
    }

    // Trigger blocks top to bottom because of item control order.
    for (int16_t i = triggered_count - 1; i >= 0; i--) {
        int16_t item_num = triggered_items[i];
        ITEM *const item = Item_Get(item_num);
        MovableBlock_SetGravityFrames(item, i);
        LOG_DEBUG("Trigger item_num: %d with gravity frames: %d", item_num, i);
        item->status = IS_ACTIVE;
        Item_AddActive(item_num);
        Item_Animate(item);
    }

    free(triggered_items);
}
