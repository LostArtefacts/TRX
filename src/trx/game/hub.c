#include <trx/game/hub.h>

#include <trx/core/subsystem.h>
#include <trx/game/lara.h>

static int32_t m_NextLevel = 0;
static int32_t m_LaraStartPos = 0;

static void M_Shutdown(void)
{
    m_NextLevel = 0;
    m_LaraStartPos = 0;
}

static const GF_LEVEL *M_GetNextLevel(const int32_t level_idx)
{
    return GF_GetLevel(GFLT_MAIN, level_idx - 1);
}

static const ITEM *M_FindLaraStartItem(void)
{
    int32_t ai_index = 0;
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (item->object_id != O_LARA_START_POS) {
            continue;
        }

        ai_index++;
        if (ai_index == m_LaraStartPos) {
            return item;
        }
    }

    return nullptr;
}

void Hub_SetNextLevelIndex(const int32_t level_idx)
{
    if (level_idx <= 0) {
        return;
    }

    if (M_GetNextLevel(level_idx) != nullptr) {
        m_NextLevel = level_idx;
    }
}

const GF_LEVEL *Hub_GetNextLevel(void)
{
    if (m_NextLevel <= 0) {
        m_NextLevel = 0;
        return nullptr;
    }

    const GF_LEVEL *const level = M_GetNextLevel(m_NextLevel);
    m_NextLevel = 0;
    return level;
}

void Hub_SetLaraStartIndex(const int32_t start_idx)
{
    m_LaraStartPos = start_idx;
}

void Hub_InitialiseLaraStart(void)
{
    if (m_LaraStartPos <= 0) {
        goto finish;
    }

    const ITEM *const start_item = M_FindLaraStartItem();
    if (start_item == nullptr) {
        goto finish;
    }

    ITEM *const lara_item = Lara_GetItem();
    lara_item->pos = start_item->pos;
    lara_item->rot = start_item->rot;
    Item_UpdateRoom(Item_GetIndex(lara_item), start_item->room_num);

finish:
    m_LaraStartPos = 0;
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
