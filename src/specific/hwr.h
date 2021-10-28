#ifndef T1M_SPECIFIC_HWR_H
#define T1M_SPECIFIC_HWR_H

#include "global/types.h"
#include "specific/ati.h"

#include <windows.h>
#include <ddraw.h>
#include <stdint.h>

// TODO: function naming, Render vs. Draw vs. Insert
// TODO: port ATI3DCIF to actual D3D calls

void HWR_CheckError(HRESULT result);
void HWR_RenderBegin();
void HWR_RenderEnd();
void HWR_RenderToggle();
void HWR_GetSurfaceAndPitch(
    LPDIRECTDRAWSURFACE surface, LPVOID *out_surface, int32_t *out_pitch);
void HWR_DisableTextures();
void HWR_ClearSurface(LPDIRECTDRAWSURFACE surface);
void HWR_ReleaseSurfaces();
void HWR_DumpScreen();
void HWR_ClearSurfaceDepth();
void HWR_FlipPrimaryBuffer();
void HWR_FadeToPal(int32_t fade_value, RGB888 *palette);
void HWR_FadeWait();
void HWR_BlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source);
void HWR_CopyPicture();
void HWR_DownloadPicture();
void HWR_RenderTriangleStrip(C3D_VTCF *vertices, int num);
void HWR_SelectTexture(int tex_num);
void HWR_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade);
void HWR_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 color1,
    RGB888 color2);
void HWR_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br);
void HWR_DrawTranslucentQuad(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void HWR_PrintShadow(PHD_VBUF *vbufs, int clip, int vertex_count);
void HWR_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2);
int32_t HWR_ClipVertices(int32_t num, C3D_VTCF *source);
int32_t HWR_ClipVertices2(int32_t num, C3D_VTCF *source);
void HWR_SwitchResolution();
int32_t HWR_SetHardwareVideoMode();
void HWR_InitialiseHardware();
void HWR_ShutdownHardware();
void HWR_PrepareFMV();
void HWR_FMVDone();
void HWR_FMVInit();
void HWR_SetPalette();
void HWR_SetupRenderContextAndRender();
void HWR_DownloadTextures(int32_t pages);
void HWR_InitPolyList();
void HWR_OutputPolyList();

int32_t
HWR_ZedClipper(int32_t vertex_count, POINT_INFO *pts, C3D_VTCF *vertices);

void HWR_DrawFlatTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int32_t color);
void HWR_DrawTexturedTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage, PHD_UV *uv1,
    PHD_UV *uv2, PHD_UV *uv3, uint16_t textype);
void HWR_DrawTexturedQuad(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, uint16_t tpage,
    PHD_UV *uv1, PHD_UV *uv2, PHD_UV *uv3, PHD_UV *uv4, uint16_t textype);

void HWR_ChangeWaterColor(const RGBF *color);

const int16_t *HWR_InsertObjectG3(const int16_t *obj_ptr, int32_t number);
const int16_t *HWR_InsertObjectG4(const int16_t *obj_ptr, int32_t number);
const int16_t *HWR_InsertObjectGT3(const int16_t *obj_ptr, int32_t number);
const int16_t *HWR_InsertObjectGT4(const int16_t *obj_ptr, int32_t number);

void T1MInjectSpecificHWR();

#endif
