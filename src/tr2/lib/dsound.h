#pragma once

#include <windows.h>

#define DirectSoundCreate                                                      \
    ((HRESULT(__stdcall *)(                                                    \
        LPCGUID pcGuidDevice, LPDIRECTSOUND * ppDS,                            \
        LPUNKNOWN pUnkOuter))0x00458CEE)
#define DirectSoundEnumerateA                                                  \
    ((HRESULT(__stdcall *)(                                                    \
        LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext))0x00458CE8)
