#ifndef T1M_GLOBAL_VARS_PLATFORM_H
#define T1M_GLOBAL_VARS_PLATFORM_H

#include "util.h"

#include "specific/ati.h"
#include <dsound.h>
#include <windows.h>

// clang-format off
#define TombHWND                VAR_U_(0x00463600, HWND)
#define AuxDeviceID             VAR_U_(0x0045B984, uint32_t)
#define MCIDeviceID             VAR_U_(0x0045B994, uint32_t)
#define DSound                  VAR_U_(0x0045F1CC, LPDIRECTSOUND)
#define HHK                     VAR_U_(0x0045A93C, HHOOK)
#define HATI3DCIFModule         VAR_U_(0x00459CF0, HMODULE)
#define DDrawSurfaceWidth       VAR_U_(0x00456D90, int32_t)
#define DDrawSurfaceHeight      VAR_U_(0x00456D94, int32_t)
#define DDraw                   VAR_U_(0x0045A998, LPDIRECTDRAW)
#define ATIRenderContext        VAR_U_(0x0045A994, C3D_HRC)
#define ATIInfo                 VAR_U_(0x0045A960, C3D_3DCIFINFO)
#define DDOldIsRendering        VAR_U_(0x00463568, int32_t)
#define DDIsRendering           VAR_U_(0x00459F34, int32_t)
// clang-format on

#endif
