#ifndef T1M_SPECIFIC_HWR_H
#define T1M_SPECIFIC_HWR_H

#include "global/types.h"
#include "specific/ati.h"

#include <windows.h>
#include <ddraw.h>
#include <stdint.h>

// TODO: function naming, Render vs. Draw vs. Insert
// TODO: port ATI3DCIF to actual D3D calls

// clang-format off
#define HWR_InitialiseHardware      ((void      (*)())0x00408005)
#define HWR_ShutdownHardware        ((void      (*)())0x00408323)
#define HWR_DownloadTextures        ((void      (*)(int16_t level_num))0x004084DE)
#define HWR_SetPalette              ((void      (*)())0x004087EA)
#define HWR_DrawSprite              ((void      (*)(int32_t x1, int32_t x2, int32_t y1, int32_t y2, int32_t z, int16_t sprnum, int16_t shade))0x0040C425)
#define HWR_ClearSurfaceDepth       ((void      (*)())0x00408AC7)
#define HWR_PrepareFMV              ((void      (*)())0x0040834C)
#define HWR_FMVDone                 ((void      (*)())0x00408368)
#define HWR_CopyPicture             ((void      (*)())0x00408B85)
#define HWR_DownloadPicture         ((void      (*)())0x00408C3A)
#define HWR_SetHardwareVideoMode    ((void      (*)())0x00407BD2)
#define HWR_OutputPolyList          ((void      (*)())0x0040D2E0)
#define HWR_SetupRenderContextAndRender ((void  (*)())0x0040795F)
#define HWR_FadeWait                ((void      (*)())0x00408E32)
// clang-format on

void HWR_Error(HRESULT result);
void HWR_RenderBegin();
void HWR_RenderEnd();
void HWR_RenderToggle();
void HWR_DisableTextures();
void HWR_ClearSurface(LPDIRECTDRAWSURFACE surface);
void HWR_DumpScreen();
void HWR_FlipPrimaryBuffer();
void HWR_FadeToPal(int32_t fade_value, RGB888 *palette);
void HWR_BlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source);
void HWR_RenderTriangleStrip(C3D_VTCF *vertices, int num);
void HWR_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 color1,
    RGB888 color2);
void HWR_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br);
void HWR_DrawTranslucentQuad(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void HWR_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2);
int32_t HWR_ClipVertices(int32_t num, C3D_VTCF *source);
int32_t HWR_ClipVertices2(int32_t num, C3D_VTCF *source);
void HWR_SwitchResolution();

void T1MInjectSpecificHWR();

#endif
