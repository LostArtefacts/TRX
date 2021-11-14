#pragma once

#include "Context.hpp"
#include "Screenshot.hpp"

#include <glrage_util/Config.hpp>

namespace glrage {

class ContextImpl : public Context
{
public:
    static ContextImpl& instance();
    static LRESULT CALLBACK callbackWindowProc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK callbackEnumWindowsProc(HWND hwnd, LPARAM _this);

    void init();
    void attach(HWND hwnd);
    void attach();
    void detach();
    LRESULT windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL enumWindowsProc(HWND hwnd);
    bool isFullscreen();
    void setFullscreen(bool fullscreen);
    void toggleFullscreen();
    void setDisplaySize(int32_t width, int32_t height);
    int32_t getDisplayWidth();
    int32_t getDisplayHeight();
    void setWindowSize(int32_t width, int32_t height);
    int32_t getWindowWidth();
    int32_t getWindowHeight();
    int32_t getScreenWidth();
    int32_t getScreenHeight();
    void setupViewport();
    void swapBuffers();
    void setRendered();
    bool isRendered();
    HWND getHWnd();
    std::string getBasePath();

private:
    ContextImpl();
    ContextImpl(ContextImpl const&) = delete;
    void operator=(ContextImpl const&) = delete;

    // constants
    static const LONG STYLE_WINDOW =
        WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPED | WS_SYSMENU;
    static const LONG STYLE_WINDOW_EX = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
                                        WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;

    // config object
    Config& m_config{Config::instance()};

    // window handle
    HWND m_hwnd = nullptr;
    HWND m_hwndTmp = nullptr;

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

    // fullscreen override mode
    int32_t m_fullscreenMode = 0;

    // rendering flag
    bool m_render = false;

    // screenshot object
    Screenshot m_screenshot;

    // temporary rectangle
    RECT m_tmprect = {0, 0, 0, 0};

    // DirectDraw display mode
    int32_t m_width = 0;
    int32_t m_height = 0;

    // Screen display mode
    int32_t m_screenWidth = 0;
    int32_t m_screenHeight = 0;
};

} // namespace glrage
