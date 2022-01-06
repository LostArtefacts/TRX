#include "gfx/context.h"

#include "game/shell.h"
#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/wgl_ext.h"
#include "gfx/screenshot.h"
#include "log.h"
#include "memory.h"

#include <string.h>

typedef struct GFX_Context {
    HWND hwnd; // window handle
    HDC hdc; // GDI device context
    HGLRC hglrc; // OpenGL context handle
    bool is_fullscreen; // fullscreen flag
    bool is_rendered; // rendering flag
    int32_t display_width;
    int32_t display_height;
    int32_t screen_width;
    int32_t screen_height;
    int32_t window_width;
    int32_t window_height;
    char *scheduled_screenshot_path;
    GFX_2D_Renderer renderer_2d;
    GFX_3D_Renderer renderer_3d;
} GFX_Context;

static GFX_Context m_Context = { 0 };

static const char *GFX_Context_GetWindowsErrorStr()
{
    DWORD error = GetLastError();
    if (error) {
        LPSTR msg_buf = NULL;
        DWORD msg_buf_size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&msg_buf, 0, NULL);

        if (msg_buf_size) {
            return msg_buf;
        }
    }

    return "Unknown error";
}

void GFX_Context_Attach(HWND hwnd)
{
    if (m_Context.hglrc || m_Context.hwnd) {
        return;
    }

    LOG_INFO("Attaching to HWND %p", hwnd);

    m_Context.hwnd = hwnd;

    m_Context.hdc = GetDC(m_Context.hwnd);
    if (!m_Context.hdc) {
        Shell_ExitSystemFmt(
            "Can't get device context", GFX_Context_GetWindowsErrorStr());
    }

    // get screen dimensions
    m_Context.screen_width = GetSystemMetrics(SM_CXSCREEN);
    m_Context.screen_height = GetSystemMetrics(SM_CYSCREEN);

    // set pixel format
    PIXELFORMATDESCRIPTOR pfd;
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(m_Context.hdc, &pfd);
    if (!pf) {
        Shell_ExitSystemFmt(
            "Can't choose pixel format: %s", GFX_Context_GetWindowsErrorStr());
    }

    if (!SetPixelFormat(m_Context.hdc, pf, &pfd)) {
        Shell_ExitSystemFmt(
            "Can't set pixel format: %s", GFX_Context_GetWindowsErrorStr());
    }

    m_Context.hglrc = wglCreateContext(m_Context.hdc);
    if (!m_Context.hglrc || !wglMakeCurrent(m_Context.hdc, m_Context.hglrc)) {
        Shell_ExitSystemFmt(
            "Can't create OpenGL context: %s",
            GFX_Context_GetWindowsErrorStr());
    }

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);

    bool vsync = true;
    if (vsync) {
        wglSwapIntervalEXT(1);
    }

    GFX_2D_Renderer_Init(&m_Context.renderer_2d);
    GFX_3D_Renderer_Init(&m_Context.renderer_3d);
}

void GFX_Context_Detach()
{
    if (!m_Context.hwnd) {
        return;
    }

    GFX_2D_Renderer_Close(&m_Context.renderer_2d);
    GFX_3D_Renderer_Close(&m_Context.renderer_3d);

    wglDeleteContext(m_Context.hglrc);
    m_Context.hglrc = NULL;

    m_Context.hwnd = NULL;
}

bool GFX_Context_IsFullscreen()
{
    return m_Context.is_fullscreen;
}

void GFX_Context_SetFullscreen(bool fullscreen)
{
    m_Context.is_fullscreen = fullscreen;
}

void GFX_Context_SetWindowSize(int32_t width, int32_t height)
{
    LOG_INFO("Window size: %dx%d", width, height);
    m_Context.window_width = width;
    m_Context.window_height = height;
}

void GFX_Context_SetDisplaySize(int32_t width, int32_t height)
{
    LOG_INFO("Display size: %dx%d", width, height);
    m_Context.display_width = width;
    m_Context.display_height = height;
}

int32_t GFX_Context_GetDisplayWidth()
{
    return m_Context.display_width;
}

int32_t GFX_Context_GetDisplayHeight()
{
    return m_Context.display_height;
}

int32_t GFX_Context_GetWindowWidth()
{
    return m_Context.window_width ? m_Context.window_width
                                  : m_Context.display_width;
}

int32_t GFX_Context_GetWindowHeight()
{
    return m_Context.window_height ? m_Context.window_height
                                   : m_Context.display_height;
}

int32_t GFX_Context_GetScreenWidth()
{
    return m_Context.screen_width;
}

int32_t GFX_Context_GetScreenHeight()
{
    return m_Context.screen_height;
}

void GFX_Context_SetupViewport()
{
    int vp_width = m_Context.window_width;
    int vp_height = m_Context.window_height;

    // default to bottom left corner of the window
    int vp_x = 0;
    int vp_y = 0;

    int hw = m_Context.display_height * vp_width;
    int wh = m_Context.display_width * vp_height;

    // create viewport offset if the window has a different
    // aspect ratio than the current display mode
    if (hw > wh) {
        int max_w = wh / m_Context.display_height;
        vp_x = (vp_width - max_w) / 2;
        vp_width = max_w;
    } else if (hw < wh) {
        int max_h = hw / m_Context.display_width;
        vp_y = (vp_height - max_h) / 2;
        vp_height = max_h;
    }

    glViewport(vp_x, vp_y, vp_width, vp_height);
}

void GFX_Context_SwapBuffers()
{
    glFinish();

    if (m_Context.scheduled_screenshot_path) {
        GFX_Screenshot_CaptureToFile(m_Context.scheduled_screenshot_path);
        Memory_FreePointer(&m_Context.scheduled_screenshot_path);
    }

    SwapBuffers(m_Context.hdc);
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_Context.is_rendered = false;
}

void GFX_Context_SetRendered()
{
    m_Context.is_rendered = true;
}

bool GFX_Context_IsRendered()
{
    return m_Context.is_rendered;
}

HWND GFX_Context_GetHWnd()
{
    return m_Context.hwnd;
}

void GFX_Context_ScheduleScreenshot(const char *path)
{
    m_Context.scheduled_screenshot_path = strdup(path);
}

GFX_2D_Renderer *GFX_Context_GetRenderer2D()
{
    return &m_Context.renderer_2d;
}

GFX_3D_Renderer *GFX_Context_GetRenderer3D()
{
    return &m_Context.renderer_3d;
}
