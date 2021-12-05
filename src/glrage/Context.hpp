#pragma once

#include <cstdint>
#include <string>
#include <windows.h>

namespace glrage {

class Context {
public:
    static Context &instance();
    static LRESULT CALLBACK
    callbackWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void attach(HWND hwnd);
    void detach();
    LRESULT windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool isFullscreen();
    void setFullscreen(bool fullscreen);
    void setWindowSize(int32_t width, int32_t height);
    void setDisplaySize(int32_t width, int32_t height);
    int32_t getDisplayWidth();
    int32_t getDisplayHeight();
    int32_t getWindowWidth();
    int32_t getWindowHeight();
    int32_t getScreenWidth();
    int32_t getScreenHeight();
    void setupViewport();
    void swapBuffers();
    void setRendered();
    bool isRendered();
    HWND getHWnd();
    void scheduleScreenshot(const std::string &path);

private:
    Context();
    Context(Context const &) = delete;
    void operator=(Context const &) = delete;

    // constants
    static const LONG STYLE_WINDOW =
        WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPED | WS_SYSMENU;
    static const LONG STYLE_WINDOW_EX = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE
        | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;

    // window handle
    HWND m_hwnd = nullptr;

    // GDI device context
    HDC m_hdc = nullptr;

    // OpenGL context handle
    HGLRC m_hglrc = nullptr;

    // Attached process ID
    DWORD m_pid = 0;

    // Pixel format
    PIXELFORMATDESCRIPTOR m_pfd;

    // Original window properties
    WNDPROC m_windowProc = nullptr;

    // fullscreen flag
    bool m_fullscreen = false;

    // rendering flag
    bool m_render = false;

    // DirectDraw display mode
    int32_t m_displayWidth = 0;
    int32_t m_displayHeight = 0;

    // Screen display mode
    int32_t m_screenWidth = 0;
    int32_t m_screenHeight = 0;

    // Window size, controlled by SDL
    int32_t m_windowWidth = 0;
    int32_t m_windowHeight = 0;

    // whether to capture screenshot on next redraw
    std::string m_screenshotScheduledPath;
};

}
