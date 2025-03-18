#include "game/objects/traps/movable_block.h"

#include "game/const.h"
#include "game/game_buf.h"
#include "game/items.h"
#include "vector.h"

#include <stdlib.h>

static int32_t m_BlockCount = 0;
static VECTOR *m_UnsortedBlocks = nullptr;
static int16_t *m_SortedBlocks = nullptr;

static int32_t M_CompareBlock(const void *item_idx1, const void *item_idx2);
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

static void M_ShiftGlobalFloorUp(void)
{
    for (int32_t i = 0; i < m_BlockCount; i++) {
        ITEM *const item = Item_Get(m_SortedBlocks[i]);
        if (item->status == IS_INACTIVE
            && item->pos.y >= Item_GetHeight(item)) {
            Room_AlterFloorHeight(item, -WALL_L);
        }
    }
}

static void M_ShiftGlobalFloorDown(void)
{
    for (int32_t i = m_BlockCount - 1; i >= 0; i--) {
        ITEM *const item = Item_Get(m_SortedBlocks[i]);
        if (item->status == IS_INACTIVE
            && item->pos.y >= Item_GetHeight(item)) {
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
    M_ShiftGlobalFloorUp();

    Vector_Free(m_UnsortedBlocks);
    m_UnsortedBlocks = nullptr;
}

void MovableBlock_HandleFlipMap(const ROOM_FLIP_STATUS flip_status)
{
    if (m_UnsortedBlocks != nullptr) {
        return;
    }

    if (flip_status == RFS_FLIPPED) {
        M_ShiftGlobalFloorUp();
    } else {
        M_ShiftGlobalFloorDown();
    }
}
