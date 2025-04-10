#pragma once

#include <libtrx/game/math/types.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output/types.h>
#include <libtrx/game/rooms/types.h>

void Output_Sprites_Init(void);
void Output_Sprites_Shutdown(void);
void Output_Sprites_ObserveLevelLoad(void);
void Output_Sprites_ObserveTextureAnimation(void);
void Output_Sprites_UploadProjectionMatrix(void);

void Output_Sprites_RenderBegin(void);
void Output_Sprites_RenderRoomSprites(
    const MATRIX *matrix, RGB_F tint, const ROOM *room);
void Output_Sprites_RenderSingleSprite(
    const MATRIX *matrix, XYZ_32 pos, int32_t sprite_idx, uint16_t shade);
bool Output_Sprites_Flush(void);
