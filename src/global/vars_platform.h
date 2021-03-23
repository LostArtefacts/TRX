#ifndef T1M_GLOBAL_VARS_PLATFORM_H
#define T1M_GLOBAL_VARS_PLATFORM_H

#include "util.h"

#include <dsound.h>
#include <windows.h>

// clang-format off
#define TombHWND                VAR_U_(0x00463600, HWND)
#define AuxDeviceID             VAR_U_(0x0045B984, uint32_t)
#define MCIDeviceID             VAR_U_(0x0045B994, uint32_t)
#define DSound                  VAR_U_(0x0045F1CC, LPDIRECTSOUND)
#define HHK                     VAR_U_(0x0045A93C, HHOOK)
// clang-format on

#endif
