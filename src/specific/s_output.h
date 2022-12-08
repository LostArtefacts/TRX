#pragma once

#include "game/picture.h"
#include "global/types.h"

bool S_Output_Init(void);
void S_Output_Shutdown(void);

void S_Output_EnableTextureMode(void);
void S_Output_DisableTextureMode(void);
void S_Output_EnableDepthTest(void);
void S_Output_DisableDepthTest(void);

void S_Output_RenderBegin(void);
void S_Output_RenderEnd(void);
void S_Output_RenderToggle(void);
void S_Output_DumpScreen(void);
void S_Output_ClearDepthBuffer(void);
void S_Output_ClearBackBuffer(void);
void S_Output_DrawEmpty(void);

void S_Output_SetViewport(int width, int height);
void S_Output_SetFullscreen(bool fullscreen);
void S_Output_ApplyResolution(void);

void S_Output_SetPalette(RGB888 palette[256]);
RGB888 S_Output_GetPaletteColor(uint8_t idx);

void S_Output_DownloadTextures(int32_t pages);
void S_Output_DownloadPicture(const PICTURE *pic);
void S_Output_SelectTexture(int tex_num);
void S_Output_CopyFromPicture(void);

void S_Output_DrawFlatTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, RGB888 color);
void S_Output_DrawTexturedTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage, PHD_UV *uv1,
    PHD_UV *uv2, PHD_UV *uv3, uint16_t textype);
void S_Output_DrawTexturedQuad(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, uint16_t tpage,
    PHD_UV *uv1, PHD_UV *uv2, PHD_UV *uv3, PHD_UV *uv4, uint16_t textype);
void S_Output_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade);
void S_Output_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGBA8888 color1,
    RGBA8888 color2);
void S_Output_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGBA8888 tl,
    RGBA8888 tr, RGBA8888 bl, RGBA8888 br);
void S_Output_DrawShadow(PHD_VBUF *vbufs, int clip, int vertex_count);
void S_Output_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2);

bool S_Output_MakeScreenshot(const char *path);

void S_Output_ScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 col_dark,
    RGBA8888 col_light, float thickness);
void S_Output_4ColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 tl, RGBA8888 tr,
    RGBA8888 bl, RGBA8888 br, float thickness);
void S_Output_2ToneColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA8888 edge,
    RGBA8888 centre, float thickness);
