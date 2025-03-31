#pragma once

#include <libtrx/game/matrix.h>
#include <libtrx/game/output/types.h>
#include <libtrx/game/rooms/types.h>

void Output_Rooms_Init(void);
void Output_Rooms_Shutdown(void);
void Output_Rooms_ObserveLevelLoad(void);

void Output_Rooms_RenderRoom(
    const MATRIX *matrix, RGB_F tint, const ROOM *room);
