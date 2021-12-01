#ifndef T1M_SPECIFIC_S_OUTPUT_H
#define T1M_SPECIFIC_S_OUTPUT_H

#include "game/picture.h"
#include "global/types.h"
#include "specific/s_ati.h"

#include "ddraw/ddraw.h"

#include <windows.h>
#include <stdint.h>

// TODO: function naming, Render vs. Draw
// TODO: port ATI3DCIF to actual D3D calls

void S_Output_EnableTextureMode(void);
void S_Output_DisableTextureMode(void);
void S_Output_RenderBegin();
void S_Output_RenderEnd();
void S_Output_RenderToggle();
void S_Output_ClearSurface(LPDIRECTDRAWSURFACE surface);
void S_Output_ReleaseSurfaces();
void S_Output_DumpScreen();
void S_Output_SetViewport(int width, int height);
void S_Output_SetFullscreen(bool fullscreen);
void S_Output_ClearBackBuffer();
void S_Output_FlipPrimaryBuffer();
void S_Output_FadeToPal(int32_t fade_value, RGB888 *palette);
void S_Output_FadeWait();
void S_Output_BlitSurface(
    LPDIRECTDRAWSURFACE source, LPDIRECTDRAWSURFACE target);
void S_Output_CopyFromPicture();
void S_Output_CopyToPicture();
void S_Output_DownloadPicture(const PICTURE *pic);
void S_Output_RenderTriangleStrip(C3D_VTCF *vertices, int num);
void S_Output_SelectTexture(int tex_num);
void S_Output_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade);
void S_Output_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 color1,
    RGB888 color2);
void S_Output_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br);
void S_Output_DrawTranslucentQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void S_Output_PrintShadow(PHD_VBUF *vbufs, int clip, int vertex_count);
void S_Output_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2);
int32_t S_Output_ClipVertices(int32_t num, C3D_VTCF *source);
int32_t S_Output_ClipVertices2(int32_t num, C3D_VTCF *source);
void S_Output_SwitchResolution();
void S_Output_SetHardwareVideoMode();
bool S_Output_Init();
void S_Output_Shutdown();
void S_Output_SetPalette();
void S_Output_SetupRenderContextAndRender();
void S_Output_DownloadTextures(int32_t pages);
void S_Output_InitPolyList();

int32_t S_Output_ZedClipper(
    int32_t vertex_count, POINT_INFO *pts, C3D_VTCF *vertices);

void S_Output_DrawFlatTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int32_t color);
void S_Output_DrawTexturedTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage, PHD_UV *uv1,
    PHD_UV *uv2, PHD_UV *uv3, uint16_t textype);
void S_Output_DrawTexturedQuad(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, uint16_t tpage,
    PHD_UV *uv1, PHD_UV *uv2, PHD_UV *uv3, PHD_UV *uv4, uint16_t textype);

#endif
