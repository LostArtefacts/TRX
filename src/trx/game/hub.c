#include <trx/game/hub.h>

#include <trx/core/subsystem.h>
#include <trx/game/lara.h>

static int32_t m_NextLevel = 0;
static int32_t m_LaraStartPos = 0;
static const HUB_START_POINT *m_StartPoints = nullptr;
static int32_t m_StartPointCount = 0;

static void M_Shutdown(void)
{
    m_NextLevel = 0;
    m_LaraStartPos = 0;
    m_StartPoints = nullptr;
    m_StartPointCount = 0;
}

static const GF_LEVEL *M_GetNextLevel(const int32_t level_idx)
{
    return GF_GetLevel(GFLT_MAIN, level_idx - 1);
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

int32_t Hub_GetNextLevelIndex(void)
{
    return m_NextLevel;
}

void Hub_SetLaraStartIndex(const int32_t start_idx)
{
    m_LaraStartPos = start_idx;
}

int32_t Hub_GetLaraStartIndex(void)
{
    return m_LaraStartPos;
}

void Hub_SetStartPoints(
    const HUB_START_POINT *const points, const int32_t count)
{
    m_StartPoints = points;
    m_StartPointCount = count;
}

void Hub_InitialiseLaraStart(const GF_LEVEL *const level)
{
    ITEM *const lara_item = Lara_GetItem();
    if (m_LaraStartPos > 0 && m_LaraStartPos <= m_StartPointCount) {
        const HUB_START_POINT *const point = &m_StartPoints[m_LaraStartPos - 1];
        if (point->room_num != NO_ROOM) {
            lara_item->pos = point->pos;
            lara_item->rot.y = point->rot_y;
            Item_UpdateRoom(Item_GetIndex(lara_item), point->room_num);
        }
    }

    m_LaraStartPos = 0;
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
