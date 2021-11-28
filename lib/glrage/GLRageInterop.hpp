#pragma once

#if defined(GLR_EXPORTS)
#define GLRAPI __declspec(dllexport)
#else
#define GLRAPI
#endif

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#else
#include <stdbool.h>
#endif

    void GLRAPI GLRage_Attach(HWND nhwnd);
    void GLRAPI GLRage_Detach();

    void GLRAPI GLRage_SetFullscreen(bool fullscreen);
    void GLRAPI GLRage_SetWindowSize(int width, int height);

#ifdef __cplusplus
}
#endif
