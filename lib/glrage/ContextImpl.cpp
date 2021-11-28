#include "ContextImpl.hpp"

#include <glrage_util/ErrorUtils.hpp>
#include <glrage_util/Logger.hpp>
#include <glrage_util/StringUtils.hpp>

#include <glrage_gl/gl_core_3_3.h>
#include <glrage_gl/wgl_ext.h>

#include <shlwapi.h>

#include <stdexcept>

namespace glrage {

ContextImpl& ContextImpl::instance()
{
    static ContextImpl instance;
    return instance;
}

LRESULT CALLBACK ContextImpl::callbackWindowProc(HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    return instance().windowProc(hwnd, msg, wParam, lParam);
}

ContextImpl::ContextImpl()
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

void ContextImpl::attach(HWND hwnd)
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

void ContextImpl::detach()
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
ContextImpl::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        // Printscreen on Windows with OpenGL doesn't work in fullscreen, so
        // hook the key and implement screenshot saving to files.
        // For some reason, VK_SNAPSHOT doesn't generate WM_KEYDOWN events but
        // only WM_KEYUP. Works just as well, though.
        case WM_KEYUP:
            if (wParam == VK_SNAPSHOT) {
                m_screenshot.schedule(true);
                return TRUE;
            }
            break;

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

bool ContextImpl::isFullscreen()
{
    return m_fullscreen;
}

void ContextImpl::setFullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
}

void ContextImpl::setWindowSize(int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

void ContextImpl::setDisplaySize(int32_t width, int32_t height)
{
    LOG_INFO("Display size: %dx%d", width, height);

    m_displayWidth = width;
    m_displayHeight = height;
}

int32_t ContextImpl::getDisplayWidth()
{
    return m_displayWidth;
}

int32_t ContextImpl::getDisplayHeight()
{
    return m_displayHeight;
}

int32_t ContextImpl::getWindowWidth()
{
    return m_windowWidth ? m_windowWidth : m_displayWidth;
}

int32_t ContextImpl::getWindowHeight()
{
    return m_windowHeight ? m_windowHeight : m_displayHeight;
}

int32_t ContextImpl::getScreenWidth()
{
    return m_screenWidth;
}

int32_t ContextImpl::getScreenHeight()
{
    return m_screenHeight;
}

void ContextImpl::setupViewport()
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

void ContextImpl::swapBuffers()
{
    glFinish();

    try {
        m_screenshot.captureScheduled();
    } catch (const std::exception& ex) {
        ErrorUtils::warning("Can't capture screenshot", ex);
        m_screenshot.schedule(false);
    }

    SwapBuffers(m_hdc);
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_render = false;
}

void ContextImpl::setRendered()
{
    m_render = true;
}

bool ContextImpl::isRendered()
{
    return m_render;
}

HWND ContextImpl::getHWnd()
{
    return m_hwnd;
}

std::string ContextImpl::getBasePath()
{
    HMODULE hModule = nullptr;
    DWORD dwFlags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

    auto windowProc = reinterpret_cast<LPCSTR>(&callbackWindowProc);
    if (!GetModuleHandleEx(dwFlags, windowProc, &hModule)) {
        throw std::runtime_error("Can't get module handle");
    }

    TCHAR path[MAX_PATH];
    GetModuleFileName(hModule, path, sizeof(path));
    PathRemoveFileSpec(path);

    return path;
}

} // namespace glrage
