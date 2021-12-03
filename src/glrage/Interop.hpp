#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#else
    #include <stdbool.h>
#endif

void GLRage_Attach(HWND nhwnd);
void GLRage_Detach();

void GLRage_SetFullscreen(bool fullscreen);
void GLRage_SetWindowSize(int width, int height);

bool GLRage_MakeScreenshot(const char *path);

#ifdef __cplusplus
}
#endif
