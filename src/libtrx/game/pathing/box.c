#include "game/game_buf.h"
#include "game/pathing.h"
#include "game/rooms.h"

static int32_t m_BoxCount = 0;
static BOX_INFO *m_Boxes = nullptr;
static int16_t *m_Overlaps = nullptr;
int16_t *g_FlyZone[2] = {};
int16_t *g_GroundZone[MAX_ZONES][2] = {};

void Box_InitialiseBoxes(const int32_t num_boxes)
{
    m_BoxCount = num_boxes;
    m_Boxes = num_boxes == 0
        ? nullptr
        : GameBuf_Alloc(sizeof(BOX_INFO) * num_boxes, GBUF_BOXES);

    if (num_boxes == 0) {
        return;
    }

    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < MAX_ZONES; j++) {
            g_GroundZone[j][i] =
                GameBuf_Alloc(sizeof(int16_t) * num_boxes, GBUF_GROUND_ZONE);
        }
        g_FlyZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * num_boxes, GBUF_FLY_ZONE);
    }
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

int16_t *Box_GetFlyZone(const bool flip_status)
{
    return g_FlyZone[flip_status];
}

int16_t *Box_GetGroundZone(const bool flip_status, const int32_t zone_idx)
{
    return g_GroundZone[zone_idx][flip_status];
}
