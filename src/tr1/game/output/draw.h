#pragma once

#include "game/output/types.h"

#include <libtrx/game/objects/types.h>
#include <libtrx/game/output/draw.h>
#include <libtrx/game/rooms/types.h>

void Output_DrawSkybox(const OBJECT_MESH *mesh);
void Output_DrawObjectMesh(const OBJECT_MESH *mesh, int32_t clip);
void Output_DrawObjectMesh_I(const OBJECT_MESH *mesh, int32_t clip);
void Output_DrawRoomMesh(ROOM *mesh);
void Output_DrawShadow(int16_t size, const BOUNDS_16 *bounds, const ITEM *item);
void Output_DrawLightningSegment(const LIGHTNING_SEGMENT segment);

void Output_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 color);
void Output_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 tl,
    RGBA_8888 tr, RGBA_8888 bl, RGBA_8888 br);

void Output_DrawScreenFrame(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col_dark,
    RGBA_8888 col_light, int32_t thickness);
void Output_DrawScreenGradientBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, int32_t thickness);
void Output_DrawScreenCentreGradientBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 center, int32_t thickness);

void Output_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade, RGB_F tint);
