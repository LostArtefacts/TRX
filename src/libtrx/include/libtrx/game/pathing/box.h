#pragma once

#include "./types.h"

void Box_InitialiseBoxes(int32_t num_boxes);
int32_t Box_GetCount(void);
BOX_INFO *Box_GetBox(int32_t box_idx);
