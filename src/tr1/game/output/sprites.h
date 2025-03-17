#pragma once

#include <libtrx/game/math/types.h>
#include <libtrx/game/output/types.h>
#include <libtrx/game/rooms/types.h>

void Output_Sprites_Init(void);
void Output_Sprites_Shutdown(void);
void Output_Sprites_UploadLevel(void);
void Output_Sprites_UploadProjectionMatrix(void);
void Output_Sprites_UploadModelMatrix(void);
void Output_Sprites_UploadUniforms(void);
void Output_Sprites_RenderRoomSprites(const ROOM *room);
