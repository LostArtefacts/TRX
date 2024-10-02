#pragma once

#include <windows.h>

#define DirectInputCreate                                                      \
    ((HRESULT(__stdcall *)(                                                    \
        HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT * lpDirectInput,       \
        LPUNKNOWN punkOuter))0x00457CC0)
