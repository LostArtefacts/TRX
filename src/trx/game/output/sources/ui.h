#pragma once

#include <trx/core/math/types.h>
#include <trx/game/objects/types.h>
#include <trx/game/output/textures.h>
#include <trx/game/output/utils.h>
#include <trx/game/viewport.h>

// An object drawn as a model in a box on the canvas. The box says where it
// goes and how large it is; the model is fitted into it and lit as the game
// lights a model held up to the camera.
typedef struct {
    const OBJECT *object;
    VIEWPORT_RECT rect;
    uint32_t mesh_mask;
    XYZ_16 rot;
} OUTPUT_UI_MESH;

typedef struct {
    int32_t sprite_idx;
    int32_t x0, y0;
    int32_t x1, y1;
    int32_t z;
    int16_t shade;
    RGBA_F color[4];
} OUTPUT_UI_SPRITE;

typedef struct {
    int32_t x0, y0;
    int32_t x1, y1;
    int32_t z;
    RGBA_8888 tl, tr, bl, br;
} OUTPUT_UI_QUAD;

typedef struct {
    int32_t cx, cy;
    int32_t r_inner, r_outer;
    int32_t z;
    RGBA_8888 color;
} OUTPUT_UI_CIRCLE;

void OutputSource_UI_Init(void);
void OutputSource_UI_Shutdown(void);

void OutputSource_UI_StageMesh(OUTPUT_UI_MESH mesh);
void OutputSource_UI_StageBinocularMask(void);
void OutputSource_UI_StageSprite(OUTPUT_UI_SPRITE sprite);
void OutputSource_UI_StageQuad(OUTPUT_UI_QUAD quad);
void OutputSource_UI_StageCircle(OUTPUT_UI_CIRCLE circle);
void OutputSource_UI_StagePhotoModeFrame(
    VIEWPORT_RECT rect, RGBA_8888 color, int32_t thickness);
