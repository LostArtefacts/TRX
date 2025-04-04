#pragma once

#include "global/types.h"

#include <libtrx/game/output.h>
#include <libtrx/gfx/context.h>

#include <stddef.h>
#include <stdint.h>

bool Output_Init(void);
void Output_Shutdown(void);
void Output_ReserveVertexBuffer(size_t size);

void Output_SetWindowSize(int32_t width, int32_t height);
void Output_ApplyRenderSettings(void);
void Output_ObserveLevelLoad(void);
void Output_ObserveLevelUnload(void);
void Output_ObserveFOVChange(void);

int32_t Output_GetNearZ(void);
int32_t Output_GetFarZ(void);
int32_t Output_GetDrawDistMin(void);
int32_t Output_GetDrawDistFade(void);
int32_t Output_GetDrawDistMax(void);
void Output_SetDrawDistFade(int32_t dist);
void Output_SetDrawDistMax(int32_t dist);
void Output_SetWaterColor(const RGB_F *color);

void Output_BeginScene(void);
void Output_EndScene(void);

void Output_ClearDepthBuffer(void);
void Output_SetSkyboxEnabled(bool enabled);
bool Output_IsSkyboxEnabled(void);

void Output_DrawSkybox(const OBJECT_MESH *mesh);
void Output_DrawObjectMesh(const OBJECT_MESH *mesh, int32_t clip);
void Output_DrawObjectMesh_I(const OBJECT_MESH *mesh, int32_t clip);
void Output_DrawRoomMesh(ROOM *mesh);
void Output_DrawRoomPortals(const ROOM *room);
void Output_DrawRoomTriggers(const ROOM *room);
void Output_DrawShadow(int16_t size, const BOUNDS_16 *bounds, const ITEM *item);
void Output_DrawLightningSegment(XYZ_32 pos_0, XYZ_32 pos_1, int32_t thickness);
void Output_Draw3DLine(XYZ_32 pos_0, XYZ_32 pos_1, RGBA_8888 color);
void Output_FlushTranslucentObjects(void);

void Output_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 color);
void Output_DrawScreenTranslucentQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 color);
void Output_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br);
void Output_DrawScreenFrame(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 colDark,
    RGBA_8888 colLight, int32_t thickness);
void Output_DrawGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, int32_t thickness);
void Output_DrawCentreGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 center, int32_t thickness);
void Output_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h);

void Output_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int32_t page);
void Output_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void Output_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade);

void Output_SetupBelowWater(bool underwater);
void Output_SetupAboveWater(bool underwater);
void Output_AnimateTextures(int32_t num_frames);

void Output_ApplyFOV(void);
void Output_ApplyTint(float *r, float *g, float *b);
RGB_F Output_GetTint(void);

bool Output_MakeScreenshot(const char *path);

bool Output_GetWaterEffect(void);
bool Output_GetWibbleEffect(void);
int32_t Output_GetTime(void);
void Output_GetProjectionMatrix(GLfloat output[][4]);
void Output_EnableScissor(float x, float y, float w, float h);
void Output_DisableScissor(void);

// TODO: these functions are poor in their design and should be not needed
void Output_RememberState(void);
void Output_RestoreState(void);

float Output_AdjustUV(uint16_t uv);
int32_t Output_GetLightDivider(void);
XYZ_32 Output_GetLightVectorView(void);
