#pragma once

#include "global/types.h"

#include <libtrx/game/output.h>

#include <stdint.h>

typedef struct {
    struct {
        float x;
        float y;
        float z;
    } pos;
    float rhw;
    struct {
        float u;
        float v;
        float z;
        float w;
    } tex;
    float g;
} VERTEX_INFO;

void Output_ApplyLevelSettings(void);

void Output_DrawObjectMesh(const OBJECT_MESH *mesh, int32_t clip);
void Output_DrawObjectMesh_I(const OBJECT_MESH *mesh, int32_t clip);
void Output_DrawRoom(const ROOM_MESH *mesh, bool is_outside);
void Output_DrawSkybox(const OBJECT_MESH *mesh);

void Output_InsertClippedPoly_Textured(
    int32_t vtx_count, float z, int16_t poly_type, int16_t tex_page);

void Output_InsertPoly_Gouraud(
    int32_t vtx_count, float z, int32_t red, int32_t green, int32_t blue,
    int16_t poly_type);

void Output_DrawClippedPoly_Textured(int32_t vtx_count);

void Output_DrawPoly_Gouraud(
    int32_t vtx_count, int32_t red, int32_t green, int32_t blue);

void Output_DrawSprite(
    uint32_t flags, int32_t x, int32_t y, int32_t z, int16_t sprite_idx,
    int16_t shade, int16_t scale);

void Output_DrawPickup(
    int32_t sx, int32_t sy, int32_t scale, int16_t sprite_idx, int16_t shade);

void Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t sz, int32_t scale_h, int32_t scale_v,
    int16_t sprite_idx, int16_t shade, uint16_t flags);

void Output_ClearDepthBuffer(void);

bool Output_MakeScreenshot(const char *path);

void Output_InsertBackPolygon(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

void Output_DrawScreenLine(
    int32_t x, int32_t y, int32_t z, int32_t x_len, int32_t y_len,
    uint8_t color_idx, const void *gour, uint16_t flags);

void Output_DrawScreenFrame(
    int32_t sx, int32_t sy, int32_t z, int32_t width, int32_t height,
    uint8_t color_idx, const void *gour, uint16_t flags);

void Output_DrawScreenFBox(
    int32_t sx, int32_t sy, int32_t z, int32_t width, int32_t height,
    uint8_t color_idx, const void *gour, uint16_t flags);

void Output_DrawHealthBar(int32_t percent);
void Output_DrawAirBar(int32_t percent);

BACKGROUND_TYPE Output_GetBackgroundType(void);
void Output_LoadBackgroundFromObject(void);

void Output_InsertShadow(
    int16_t radius, const BOUNDS_16 *bounds, const ITEM *item);

void Output_CalculateWibbleTable(void);
void Output_SetupBelowWater(bool is_underwater);
void Output_SetupAboveWater(bool is_underwater);
void Output_AnimateTextures(int32_t ticks);

void Output_SetShadeEffect(bool shade_effect);
bool Output_IsShadeEffect(void);
void Output_SetSunsetEnabled(bool enabled);
void Output_SetSunsetTimer(int32_t timer);

int32_t Output_GetNearZ(void);
int32_t Output_GetFarZ(void);

void Output_SetWaterColor(RGB_888 color);
RGB_F Output_GetTint(void);

int32_t Output_GetDepthBias(void);
void Output_SetDepthBias(int32_t bias);
