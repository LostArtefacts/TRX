#pragma once

#include "types.h"

extern int32_t Room_GetTotalCount(void);
extern ROOM *Room_Get(int32_t room_num);

extern void Room_FlipMap(void);
extern bool Room_GetFlipStatus(void);
