#include "game/objects/traps/movable_block.h"

#include "game/const.h"
#include "game/game_buf.h"
#include "game/items.h"
#include "game/objects/vars.h"
#include "vector.h"

#include <log.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Bit‑layout of ITEM->priv for movable blocks
 * -------------------------------------------------------------------------
 * bit 0      : LARA_PUSH_PULL flag (boolean)
 * bits 1–7   : free (reserved for future one‑bit flags)
 * bits 8–15  : CONSTANT_GRAVITY frame counter (0‑255)
 * higher bits: currently unused – still available
 * -------------------------------------------------------------------------*/
#define LARA_PUSH_PULL ((uintptr_t)1u << 0)
#define GRAVITY_SHIFT 8u
#define GRAVITY_MASK ((uintptr_t)0xFFu << GRAVITY_SHIFT)

typedef struct {
    int16_t counter_rot[3];
    int16_t original_rot;
} M_PRIV;

static int32_t m_BlockCount = 0;
static VECTOR *m_UnsortedBlocks = nullptr;
static int16_t *m_SortedBlocks = nullptr;

static int32_t M_CompareBlock(const void *item_idx1, const void *item_idx2);
static bool M_IsValidFloorShiftState(const ITEM *item);
static void M_ShiftGlobalFloorUp(void);
static void M_ShiftGlobalFloorDown(void);
static void M_SetPrivBits(ITEM *item, uintptr_t mask, uintptr_t value_shifted);
static bool M_TestPrivFlag(const ITEM *item, uintptr_t mask);

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

static void M_SetPrivBits(
    ITEM *const item, const uintptr_t mask, const uintptr_t value_shifted)
{
    uintptr_t bits = (uintptr_t)item->priv;
    bits = (bits & ~mask) | value_shifted;
    item->priv = (void *)bits;
}

static uintptr_t M_GetPrivBits(
    const ITEM *item, const uintptr_t mask, const uint32_t shift)
{
    return (((uintptr_t)item->priv & mask) >> shift);
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
}

// TODO: make private
void MovableBlock_UpdateRotation(ITEM *const item, const int16_t rot_y)
{
    item->rot.y = rot_y;
    M_PRIV *const data = (M_PRIV *)item->data;
    data->counter_rot[0] = data->original_rot - rot_y;
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
    uintptr_t value = enable ? LARA_PUSH_PULL : 0u;
    M_SetPrivBits(item, LARA_PUSH_PULL, value);
}

bool MovableBlock_IsPushPull(const ITEM *const item)
{
    return ((uintptr_t)item->priv & LARA_PUSH_PULL) != 0u;
}

void MovableBlock_SetGravityFrames(ITEM *const item, const uint8_t frames)
{
    M_SetPrivBits(item, GRAVITY_MASK, (uintptr_t)frames << GRAVITY_SHIFT);
}

uint8_t MovableBlock_GetGravityFrames(const ITEM *const item)
{
    return (uint8_t)M_GetPrivBits(item, GRAVITY_MASK, GRAVITY_SHIFT);
}

void MovableBlock_ActivateStack(
    const ITEM *const base_item, const int32_t x, const int32_t y,
    const int32_t z)
{
    const ROOM *const room = Room_Get(base_item->room_num);
    const SECTOR *sector =
        Room_GetWorldSector(room, base_item->pos.x, base_item->pos.z);
    const SECTOR *const pit_sector = Room_GetPitSector(sector, x, z);
    int32_t height = pit_sector->floor.height;
    LOG_DEBUG("pit floor: %d; y: %d", pit_sector->floor.height, y);

    int16_t *triggered_items =
        (int16_t *)malloc((size_t)Item_GetWalkableCount() * sizeof(int16_t));
    int32_t triggered_count = 0;

    // Climb the stack of walkables and trigger each one.
    int32_t test_y = y;
    for (int32_t i = 0; i < Item_GetWalkableCount(); i++) {
        const int16_t item_num = Item_GetWalkableNum(i);
        ITEM *const item = Item_Get(item_num);
        if (Object_IsType(item->object_id, g_MovableBlockObjects)) {
            const OBJECT *const obj = Object_Get(item->object_id);
            if (obj->floor_height_func != nullptr) {
                const int32_t walkable_height =
                    obj->floor_height_func(item, x, test_y, z, height);
                if (walkable_height != height) {
                    test_y = walkable_height;
                    height = walkable_height;
                    triggered_items[triggered_count++] = item_num;
                    LOG_DEBUG(
                        "Activate floor height: %d; y: %d; test_y: %d; "
                        "item_num: "
                        "%d; object_id: "
                        "%d",
                        height, y, test_y, item_num, item->object_id);
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
        LOG_DEBUG("Trigger item_num: %d; delay: %d", item_num, i);
    }

    free(triggered_items);
}
