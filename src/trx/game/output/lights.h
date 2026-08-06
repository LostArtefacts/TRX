#pragma once

#include <trx/core/colors.h>
#include <trx/core/vector.h>
#include <trx/game/objects/types.h>
#include <trx/game/rooms.h>
#include <trx/game/types.h>
#include <trx/game/viewport.h>

void Output_Lights_Init(void);
void Output_Lights_Shutdown(void);
void Output_Lights_ObserveLevelLoad(void);

void Output_CalculateLight(XYZ_32 pos, int16_t room_num);
void Output_CalculateStaticLight(int16_t adder);
void Output_CalculateStaticLightRGB15(int16_t rgb15);
void Output_CalculateStaticLightRGB_F(RGB_F rgb);
void Output_CalculateStaticMeshLight(XYZ_32 pos, SHADE shade, const ROOM *room);
void Output_CalculateObjectLighting(const ITEM *item, const BOUNDS_16 *bounds);
void Output_CalculateObjectLightingAt(
    const ITEM *item, const GAME_VECTOR sample_pos);
int32_t Output_GetRoomLightShade(ROOM_LIGHT_MODE mode);

int32_t Output_GetSunsetDuration(void);
void Output_SetSunsetEnabled(bool enabled);
int16_t Output_GetSkyShade(void);

void Output_ResetDynamicLights(void);
VECTOR *Output_GetDynamicLights(void);

// What one frame can show at once, shared between the bulbs a level carries,
// the timed ones the game triggers and the ones asked for on the fly. The
// shader's MAX_FOG_BULBS matches this.
#define OUTPUT_MAX_FOG_BULBS 10
// How many lights can wait for the draw at once. Past this, the ones nearest
// the camera are the ones kept; fog is held OUTPUT_MAX_FOG_BULBS deep, since
// the buffer it fills shows no more than that.
#define OUTPUT_MAX_PENDING_LIGHTS 64

// A light for this frame, asked for again every frame for as long as it should
// be seen. It reaches the renderer as the scene is drawn rather than as it is
// asked for, so that a caller's light does not turn on whether it ran before or
// after the point the frame's lights are reset.
void Output_AddDynamicLight(XYZ_32 pos, int32_t intensity, int32_t falloff);
// The same, in color. TR1 and TR2 shade in brightness alone, so the light
// stands for its brightest channel there and comes out white.
void Output_AddDynamicLightRGB(XYZ_32 pos, int32_t falloff, RGB_888 color);
// A volumetric fog bulb for this frame, in any game, on the same terms.
void Output_AddFogBulb(
    XYZ_32 pos, int32_t radius, int32_t density, RGB_888 color);

// Lets go of what was asked for, so that a control frame the game had no time
// to draw leaves nothing behind for the one that is drawn.
void Output_DropPendingLights(void);
void Output_FlushPendingLights(void);

// TR4 volumetric FX fog bulb (e.g. underwater flares); no-op otherwise.
void Output_TriggerFXFogBulb(
    XYZ_32 pos, int32_t fx_rad, int32_t density, RGB_888 color);
// TR4 doubles the sun light and disables fog bulbs in the inventory.
void Output_SetInventoryLightingMode(bool enabled);

void Output_AnimateLights(int32_t num_frames);
