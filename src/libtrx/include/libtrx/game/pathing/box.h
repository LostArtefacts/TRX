#pragma once

#include "./types.h"

extern BOX_INFO *g_Boxes; // TODO: make local

void Box_InitialiseBoxes(int32_t num_boxes);
int32_t Box_GetCount(void);
BOX_INFO *Box_GetBox(int32_t box_idx);
