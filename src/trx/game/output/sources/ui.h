#pragma once

#include <trx/core/math/types.h>
#include <trx/game/objects/types.h>
#include <trx/game/output/textures.h>
#include <trx/game/output/utils.h>
#include <trx/game/viewport.h>

// The light a model is drawn in: an ambient light, up to three lights shining
// from a direction in a colour of their own, and a colour some of the meshes,
// one bit each, take from every light in place of the light's own. Each game
// reads it its own way: TR3 lights in colour, TR4 by the ambient light alone,
// and TR1 and TR2 by the ambient light's brightness and the first light's.
typedef struct {
    RGB_F ambient;
    int32_t light_count;
    struct {
        XYZ_32 dir;
        RGB_F color;
    } lights[3];
    uint32_t lit_mask;
    RGB_F colors[32];
} OUTPUT_UI_MESH_LIGHT;

// Where a model stands in a world of its own, and the camera that sees it: the
// model is placed by turns and moves applied in order from the world's origin,
// as the engine places a model, and the light's directions are in that world.
#define OUTPUT_UI_MESH_STEPS 16
typedef struct {
    XYZ_32 camera_pos;
    XYZ_32 camera_target;
    int16_t fov;
    int32_t step_count;
    struct {
        bool turn;
        XYZ_32 vec;
    } steps[OUTPUT_UI_MESH_STEPS];
} OUTPUT_UI_MESH_WORLD;

// An object drawn as a model in a box on the canvas. The box says where it
// goes and how large it is; the model is fitted into it and lit by the light
// given, or by a plain white one where there is none.
typedef struct {
    const OBJECT *object;
    VIEWPORT_RECT rect;
    uint32_t mesh_mask;
    XYZ_16 rot;
    // Tilted and turned first as the game holds up a pickup.
    bool held;
    // Tilted towards the viewer before anything else, about the screen's
    // horizontal axis.
    int16_t tilt;
    const OUTPUT_UI_MESH_LIGHT *light;
    // Where it stands in a world of its own instead of in the box, if at all.
    const OUTPUT_UI_MESH_WORLD *world;
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
