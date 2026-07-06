#pragma once

#include <trx/core/colors.h>
#include <trx/game/output/sources/objects.h>

#include <stdint.h>

// TR4 sky: up to two flat scrolling color layers drawn under the skybox
// (horizon) mesh, plus an optional lightning flash that recolors layer 0.

#define OUTPUT_SKY_LAYER_COUNT 2
// The distance after which the layer quads tile seamlessly.
#define OUTPUT_SKY_WRAP 0x2C00

typedef struct {
    bool enabled;
    RGB_888 color;
    int16_t speed;
} OUTPUT_SKY_LAYER;

void Output_Sky_Reset(void);
void Output_Sky_SetLayer(int32_t layer_idx, RGB_888 color, int16_t speed);
void Output_Sky_SetColorAdd(bool enabled);
// TR4 desert levels: blend the skybox mesh's bottom edge into the fog color
// (OG's hardcoded specular gradient on the horizon mesh's first 16 quads).
void Output_Sky_SetFogGradient(bool enabled);
void Output_Sky_SetLightningEnabled(bool enabled);
// Atlas page holding the level's sky image; -1 if none is loaded.
void Output_Sky_SetTexturePage(int32_t page_idx);
int32_t Output_Sky_GetTexturePage(void);

const OUTPUT_SKY_LAYER *Output_Sky_GetLayer(int32_t layer_idx);
int32_t Output_Sky_GetLayerPos(int32_t layer_idx);
// The layer color (with any lightning flash applied) in the OG 128-neutral
// scale, ready for the shader's VERT_OVERBRIGHT path.
RGB_888 Output_Sky_GetLayerDrawColor(int32_t layer_idx);
bool Output_Sky_IsColorAdd(void);

void Output_Sky_Update(void);

// Draws the skybox (horizon) mesh centered on the camera, staging the TR4
// flat layers beneath it first. Returns false if no skybox object is loaded.
bool Output_Sky_Draw(void);

// Registers/removes the skybox mesh render policy with the object mesh
// pipeline; must run before OutputSource_Objects_ObserveLevelLoad.
void Output_Sky_ObserveLevelLoad(void);
void Output_Sky_ObserveLevelUnload(void);
