#pragma once

#include "./types.h"

extern int16_t *g_FlyZone[2]; // TODO: make local
extern int16_t *g_GroundZone[][2];

void Box_InitialiseBoxes(int32_t num_boxes);
int16_t *Box_InitialiseOverlaps(int32_t num_overlaps);
int32_t Box_GetCount(void);
BOX_INFO *Box_GetBox(int32_t box_idx);
int16_t Box_GetOverlap(int32_t overlap_idx);
int16_t *Box_GetFlyZone(bool flip_status);
int16_t *Box_GetGroundZone(bool flip_status, int32_t zone_idx);
