#include "glrage/Context.hpp"

#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/wgl_ext.h"
#include "gfx/screenshot.h"
#include "glrage_util/ErrorUtils.hpp"
#include "glrage_util/StringUtils.hpp"
#include "log.h"

#include <stdexcept>

namespace glrage {

Context &Context::instance()
{
    static Context instance;
    return instance;
}

LRESULT CALLBACK
Context::callbackWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return instance().windowProc(hwnd, msg, wParam, lParam);
}

Context::Context()
{
    // set pixel format
    m_pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    m_pfd.nVersion = 1;
    m_pfd.iPixelType = PFD_TYPE_RGBA;
    m_pfd.cColorBits = 32;
    m_pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    m_pfd.cDepthBits = 32;
    m_pfd.iLayerType = PFD_MAIN_PLANE;

    // get screen dimensions
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
}

void Context::attach(HWND hwnd)
{
    if (m_hglrc || m_hwnd) {
        return;
    }

    LOG_INFO("Attaching to HWND %p", hwnd);

    m_hwnd = hwnd;

    m_hdc = GetDC(m_hwnd);
    if (!m_hdc) {
        ErrorUtils::error(
            "Can't get device context", ErrorUtils::getWindowsErrorString());
    }

    auto pf = ChoosePixelFormat(m_hdc, &m_pfd);
    if (!pf) {
        ErrorUtils::error(
            "Can't choose pixel format", ErrorUtils::getWindowsErrorString());
    }

    if (!SetPixelFormat(m_hdc, pf, &m_pfd)) {
        ErrorUtils::error(
            "Can't set pixel format", ErrorUtils::getWindowsErrorString());
    }

    m_hglrc = wglCreateContext(m_hdc);
    if (!m_hglrc || !wglMakeCurrent(m_hdc, m_hglrc)) {
        ErrorUtils::error(
            "Can't create OpenGL context", ErrorUtils::getWindowsErrorString());
    }

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);

    bool vsync = true;
    if (vsync) {
        wglSwapIntervalEXT(1);
    }

    ErrorUtils::setHWnd(m_hwnd);

    // get window procedure pointer and replace it with custom procedure
    auto windowProc = GetWindowLongPtr(m_hwnd, GWLP_WNDPROC);
    m_windowProc = reinterpret_cast<WNDPROC>(windowProc);
    windowProc = reinterpret_cast<LONG_PTR>(&callbackWindowProc);
    SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, windowProc);
}

void Context::detach()
{
    if (!m_hwnd) {
        return;
    }

    wglDeleteContext(m_hglrc);
    m_hglrc = nullptr;

    auto windowProc = reinterpret_cast<LONG_PTR>(m_windowProc);
    SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, windowProc);
    m_windowProc = nullptr;

    m_hwnd = nullptr;
}

LRESULT
Context::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    // force default handling for some window messages when in windowed
    // mode, especially important for Tomb Raider
    case WM_MOVE:
    case WM_MOVING:
    case WM_SIZE:
    case WM_NCPAINT:
    case WM_SETCURSOR:
    case WM_GETMINMAXINFO:
    case WM_ERASEBKGND:
        if (!m_fullscreen) {
            return CallWindowProc(DefWindowProc, hwnd, msg, wParam, lParam);
        }
        break;
    }

    return CallWindowProc(m_windowProc, hwnd, msg, wParam, lParam);
}

bool Context::isFullscreen()
{
    return m_fullscreen;
}

void Context::setFullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
}

void Context::setWindowSize(int width, int height)
{
    LOG_INFO("Window size: %dx%d", width, height);
    m_windowWidth = width;
    m_windowHeight = height;
}

void Context::setDisplaySize(int32_t width, int32_t height)
{
    LOG_INFO("Display size: %dx%d", width, height);
    m_displayWidth = width;
    m_displayHeight = height;
}

int32_t Context::getDisplayWidth()
{
    return m_displayWidth;
}

int32_t Context::getDisplayHeight()
{
    return m_displayHeight;
}

int32_t Context::getWindowWidth()
{
    return m_windowWidth ? m_windowWidth : m_displayWidth;
}

int32_t Context::getWindowHeight()
{
    return m_windowHeight ? m_windowHeight : m_displayHeight;
}

int32_t Context::getScreenWidth()
{
    return m_screenWidth;
}

int32_t Context::getScreenHeight()
{
    return m_screenHeight;
}

void Context::setupViewport()
{
    auto vpWidth = getWindowWidth();
    auto vpHeight = getWindowHeight();

    // default to bottom left corner of the window
    auto vpX = 0;
    auto vpY = 0;

    auto hw = m_displayHeight * vpWidth;
    auto wh = m_displayWidth * vpHeight;

    // create viewport offset if the window has a different
    // aspect ratio than the current display mode
    if (hw > wh) {
        auto wMax = wh / m_displayHeight;
        vpX = (vpWidth - wMax) / 2;
        vpWidth = wMax;
    } else if (hw < wh) {
        auto hMax = hw / m_displayWidth;
        vpY = (vpHeight - hMax) / 2;
        vpHeight = hMax;
    }

    glViewport(vpX, vpY, vpWidth, vpHeight);
}

void Context::swapBuffers()
{
    glFinish();

    if (!m_screenshotScheduledPath.empty()) {
        GFX_Screenshot_CaptureToFile(m_screenshotScheduledPath.c_str());
        m_screenshotScheduledPath.clear();
    }

    SwapBuffers(m_hdc);
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_render = false;
}

void Context::setRendered()
{
    m_render = true;
}

bool Context::isRendered()
{
    return m_render;
}

HWND Context::getHWnd()
{
    return m_hwnd;
}

void Context::scheduleScreenshot(const std::string &path)
{
    m_screenshotScheduledPath = path;
}

}
