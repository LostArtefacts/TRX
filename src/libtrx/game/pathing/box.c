#include "game/game_buf.h"
#include "game/pathing.h"

static int32_t m_BoxCount = 0;
static BOX_INFO *m_Boxes = nullptr;
static int16_t *m_Overlaps = nullptr;

void Box_InitialiseBoxes(const int32_t num_boxes)
{
    m_BoxCount = num_boxes;
    m_Boxes = num_boxes == 0
        ? nullptr
        : GameBuf_Alloc(sizeof(BOX_INFO) * num_boxes, GBUF_BOXES);
}

int16_t *Box_InitialiseOverlaps(const int32_t num_overlaps)
{
    m_Overlaps = num_overlaps == 0
        ? nullptr
        : GameBuf_Alloc(sizeof(int16_t) * num_overlaps, GBUF_OVERLAPS);
    return m_Overlaps;
}

int32_t Box_GetCount(void)
{
    return m_BoxCount;
}

BOX_INFO *Box_GetBox(const int32_t box_idx)
{
    return m_Boxes == nullptr ? nullptr : &m_Boxes[box_idx];
}

int16_t Box_GetOverlap(const int32_t overlap_idx)
{
    return m_Overlaps == nullptr ? -1 : m_Overlaps[overlap_idx];
}
