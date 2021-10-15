#ifndef T1M_GLOBAL_VARS_PLATFORM_H
#define T1M_GLOBAL_VARS_PLATFORM_H

#include "util.h"

#include "specific/ati.h"
#include <ddraw.h>
#include <dsound.h>
#include <windows.h>

// clang-format off
#define TombHWND                VAR_U_(0x00463600, HWND)
#define AuxDeviceID             VAR_U_(0x0045B984, uint32_t)
#define MCIDeviceID             VAR_U_(0x0045B994, uint32_t)
#define DSound                  VAR_U_(0x0045F1CC, LPDIRECTSOUND)
#define HATI3DCIFModule         VAR_U_(0x00459CF0, HMODULE)
#define DDrawSurfaceMinX        VAR_I_(0x00459F24, float, 0.0)
#define DDrawSurfaceMinY        VAR_I_(0x00459F20, float, 0.0)
#define DDrawSurfaceMaxX        VAR_I_(0x00453068, float, 639.0)
#define DDrawSurfaceMaxY        VAR_I_(0x00453064, float, 479.0)
#define DDrawSurfaceWidth       VAR_U_(0x00456D90, int32_t)
#define DDrawSurfaceHeight      VAR_U_(0x00456D94, int32_t)
#define Surface1                VAR_U_(0x005DA6A4, LPDIRECTDRAWSURFACE)
#define Surface1DrawPtr         VAR_U_(0x00463564, void*)
#define Surface2DrawPtr         VAR_U_(0x005DB480, void*)
#define DDraw                   VAR_U_(0x0045A998, LPDIRECTDRAW)
#define ATIRenderContext        VAR_U_(0x0045A994, C3D_HRC)
#define ATIInfo                 VAR_U_(0x0045A960, C3D_3DCIFINFO)
#define HWR_OldIsRendering      VAR_U_(0x00463568, int32_t)
#define HWR_IsRendering         VAR_U_(0x00459F34, int32_t)
#define HWR_IsTextureMode       VAR_U_(0x00459F28, int32_t)
#define HWR_SelectedTexture     VAR_I_(0x00453060, int32_t, -1)
// clang-format on

extern HINSTANCE TombModule;

#endif
