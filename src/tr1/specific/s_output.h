#pragma once

#include "global/types.h"

#include <libtrx/engine/image.h>
#include <libtrx/gfx/context.h>

#include <stdint.h>

bool S_Output_Init(void);
void S_Output_Shutdown(void);

void S_Output_EnableTextureMode(void);
void S_Output_DisableTextureMode(void);
void S_Output_SetBlendingMode(GFX_BLEND_MODE blend_mode);
void S_Output_EnableDepthWrites(void);
void S_Output_DisableDepthWrites(void);
void S_Output_EnableDepthTest(void);
void S_Output_DisableDepthTest(void);

void S_Output_RenderBegin(void);
void S_Output_RenderEnd(void);
void S_Output_Flush(void);
void S_Output_FlipScreen(void);
void S_Output_ClearDepthBuffer(void);

void S_Output_SetWindowSize(int width, int height);
void S_Output_ApplyRenderSettings(void);

void S_Output_DownloadTextures(int32_t pages);
void S_Output_SelectTexture(int32_t texture_num);
void S_Output_DownloadBackdropSurface(const IMAGE *image);
void S_Output_DrawBackdropSurface(void);

void S_Output_DrawFlatTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, RGBA_8888 color);
void S_Output_DrawEnvMapTriangle(
    const PHD_VBUF *vn1, const PHD_VBUF *vn2, const PHD_VBUF *vn3);
void S_Output_DrawEnvMapQuad(
    const PHD_VBUF *vn1, const PHD_VBUF *vn2, const PHD_VBUF *vn3,
    const PHD_VBUF *vn4);
void S_Output_DrawTexturedTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage,
    uint16_t textype);
void S_Output_DrawTexturedQuad(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, int16_t tpage,
    uint16_t textype);
void S_Output_Draw3DLine(
    const PHD_VBUF *vn1, const PHD_VBUF *vn2, const RGBA_8888 color);
void S_Output_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade);
void S_Output_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br);
void S_Output_DrawShadow(PHD_VBUF *vbufs, int clip, int vertex_count);
void S_Output_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2);

void S_Output_ScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col_dark,
    RGBA_8888 col_light, float thickness);
void S_Output_4ColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, float thickness);
void S_Output_2ToneColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 centre, float thickness);
