#pragma once

#include <libtrx/game/matrix.h>
#include <libtrx/game/output/types.h>
#include <libtrx/game/rooms/types.h>

void Output_Meshes_InitRooms(void);
void Output_Meshes_ShutdownRooms(void);
void Output_Meshes_ObserveLevelLoadRooms(void);
void Output_Meshes_ObserveTextureAnimationRooms(void);
void Output_Meshes_ObserveRoomFlip(const ROOM *room);

void Output_Meshes_RenderRoomMesh(
    const MATRIX *matrix, RGB_F tint, const ROOM *room);
