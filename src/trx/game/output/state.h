#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>
#include <trx/game/output/uniforms.h>
#include <trx/game/rooms.h>
#include <trx/game/viewport.h>

#include <GL/glew.h>

void Output_SetSkyboxEnabled(bool enabled);
bool Output_IsSkyboxEnabled(void);

void Output_GetPerspProjectionMatrix(GLfloat output[][4]);
void Output_GetOrthoProjectionMatrix(GLfloat output[][4]);

void Output_SetTime(float time);
float Output_GetTime(void);
float Output_GetTimeInGame(void);
void Output_SetTimeInGame(float time);
bool Output_IsControlFrame(void);
void Output_SetControlFrame(bool is_control_frame);

void Output_SetupBelowWater(bool is_underwater);
void Output_SetupAboveWater(bool is_underwater);
RGB_F Output_GetWaterColor(void);
void Output_SetWaterColor(RGB_888 color);

// Returns the tint for objects and static meshes. TR1 to TR3 tint objects in
// water and dry rooms seen from below the surface. TR4 tints only dry rooms
// seen from below the surface.
RGBA_F Output_GetTint(void);
// Returns the tint for room geometry. TR4 tints wet and dry rooms while the
// camera is below the surface.
RGBA_F Output_GetRoomTint(void);
void Output_PushTintOverride(RGBA_F tint);
void Output_PopTintOverride(void);
bool Output_GetWaterEffect(void);
// Reports whether room geometry distorts. TR1 to TR3 distort water seen from
// above and dry rooms seen from below the surface. TR4 distorts every room
// while the camera is below the surface.
bool Output_GetWibbleEffect(void);
// Reports whether objects and static meshes distort.
bool Output_GetObjectWibbleEffect(void);

RGBA_F Output_GetFogColor(void);
int32_t Output_GetFogStart(void);
int32_t Output_GetFogEnd(void);

void Output_SetFogColor(RGBA_8888 color);
void Output_SetFogStart(int32_t dist);
void Output_SetFogEnd(int32_t dist);

int32_t Output_GetNearZ(void);
int32_t Output_GetFarZ(void);
int32_t Output_GetNearZ_UI(void);
int32_t Output_GetFarZ_UI(void);

void Output_SetCurrentRoom(const ROOM *room_num);
const ROOM *Output_GetCurrentRoom(void);

// Clips object meshes staged after this call to the given game viewport
// rectangle. Passing nullptr disables the object clip.
void Output_SetObjectScissor(const VIEWPORT_RECT *rect);
const VIEWPORT_RECT *Output_GetObjectScissor(void);

int32_t Output_GetLightAdder(void);
int32_t Output_GetLightDivider(void);
XYZ_32 Output_GetLightVectorView(void);
OUTPUT_LIGHT_INFO Output_GetLightInfo(void);
void Output_SetLightAdder(int32_t adder);
void Output_SetLightDivider(int32_t divider);
void Output_RotateLight(int16_t pitch, int16_t yaw);
void Output_SetTR3Light(
    RGB_F ambient, const RGB_F colors[3], const XYZ_32 dirs_view[3]);

void Output_EnableScissor(float x, float y, float w, float h);
void Output_DisableScissor(void);

void Output_AdjustDepth(float factor, float units);

void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);

void Output_AnimateTextures(int32_t num_frames);

// TR4 UV rotate: scroll speed for the UV-rotating animated texture ranges.
// Sign controls direction; 0 disables (the ranges frame-swap instead).
// Must be set before the level data loads to take effect.
void Output_SetUVRotateSpeed(int32_t speed);
int32_t Output_GetUVRotateSpeed(void);
// The scroll is measured in game frames and wraps at a period the level's
// scrolling textures share, so that the tick stays exact however long the
// level runs.
void Output_SetUVScrollTickPeriod(int32_t period);
float Output_GetUVScrollTick(void);
