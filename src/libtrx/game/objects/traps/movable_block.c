#include "game/objects/traps/movable_block.h"

#include "game/const.h"
#include "game/game_buf.h"
#include "game/items.h"
#include "game/objects/vars.h"
#include "game/pathing.h"
#include "vector.h"

#include <stdlib.h>

typedef struct {
    int16_t counter_rot[3];
    int16_t original_rot;
    uint16_t gravity_frames;
    bool is_push_pull;
} M_PRIV;

static int32_t m_BlockCount = 0;
static VECTOR *m_UnsortedBlocks = nullptr;
static int16_t *m_SortedBlocks = nullptr;

static int32_t M_CompareBlock(const void *item_idx1, const void *item_idx2);
static bool M_IsValidFloorShiftState(const ITEM *item);
static void M_ShiftGlobalFloorUp(void);
static void M_ShiftGlobalFloorDown(void);

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
    M_PRIV *const data = GameBuf_Alloc(sizeof(M_PRIV), GBUF_ITEM_DATA);
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
    M_PRIV *const data = (M_PRIV *)item->data;
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
    M_PRIV *const data = (M_PRIV *)item->data;
    data->is_push_pull = enable;
}

bool MovableBlock_IsPushPull(const ITEM *const item)
{
    const M_PRIV *const data = (M_PRIV *)item->data;
    return data ? data->is_push_pull : false;
}

void MovableBlock_SetGravityFrames(ITEM *const item, const uint8_t frames)
{
    M_PRIV *const data = (M_PRIV *)item->data;
    data->gravity_frames = frames;
}

uint16_t MovableBlock_GetGravityFrames(const ITEM *const item)
{
    const M_PRIV *const data = (M_PRIV *)item->data;
    return data ? data->gravity_frames : 0;
}

void MovableBlock_ActivateStack(
    const ITEM *const base_item, const XYZ_32 sector_pos)
{
    int16_t *triggered_items =
        (int16_t *)malloc((size_t)Item_GetWalkableCount() * sizeof(int16_t));
    int32_t triggered_count = 0;

    // Check for a stack of movable blocks and trigger each one.
    // Only trigger stacked blocks resting on the trapdoor pos.y.
    int32_t stack_height = base_item->pos.y;
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
                }
            }
        }
    }

    // Trigger blocks top to bottom because of item control order.
    for (int16_t i = triggered_count - 1; i >= 0; i--) {
        int16_t item_num = triggered_items[i];
        ITEM *const item = Item_Get(item_num);
        MovableBlock_SetGravityFrames(item, i);
        item->status = IS_ACTIVE;
        Item_AddActive(item_num);
        Item_Animate(item);
    }

    free(triggered_items);
}
