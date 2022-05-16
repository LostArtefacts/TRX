#include "gfx/context.h"

#include "config.h"
#include "game/shell.h"
#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/wgl_ext.h"
#include "gfx/screenshot.h"
#include "log.h"
#include "memory.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <string.h>

typedef struct GFX_Context {
    void *window_handle;
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

void GFX_Context_Attach(void *window_handle)
{
    if (m_Context.window_handle) {
        return;
    }

    LOG_INFO("Attaching to window %p", window_handle);

    m_Context.window_handle = window_handle;

    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(window_handle, &wm_info);

    HWND hwnd = wm_info.info.win.window;
    if (!hwnd) {
        Shell_ExitSystem("System Error: cannot create window");
        return;
    }

    m_Context.hdc = GetDC(hwnd);
    if (!m_Context.hdc) {
        Shell_ExitSystem("Can't get device context");
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
        Shell_ExitSystem("Can't choose pixel format");
    }

    if (!SetPixelFormat(m_Context.hdc, pf, &pfd)) {
        Shell_ExitSystem("Can't set pixel format");
    }

    m_Context.hglrc = wglCreateContext(m_Context.hdc);
    if (!m_Context.hglrc || !wglMakeCurrent(m_Context.hdc, m_Context.hglrc)) {
        Shell_ExitSystem("Can't create OpenGL context");
    }

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);

    // VSync defaults to on unless user disabled it in runtime json
    wglSwapIntervalEXT(1);

    GFX_2D_Renderer_Init(&m_Context.renderer_2d);
    GFX_3D_Renderer_Init(&m_Context.renderer_3d);
}

void GFX_Context_Detach(void)
{
    if (!m_Context.window_handle) {
        return;
    }

    GFX_2D_Renderer_Close(&m_Context.renderer_2d);
    GFX_3D_Renderer_Close(&m_Context.renderer_3d);

    wglDeleteContext(m_Context.hglrc);
    m_Context.hglrc = NULL;

    m_Context.window_handle = NULL;
}

void GFX_Context_SetVSync(bool vsync)
{
    wglSwapIntervalEXT(vsync);
}

bool GFX_Context_IsFullscreen(void)
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

int32_t GFX_Context_GetDisplayWidth(void)
{
    return m_Context.display_width;
}

int32_t GFX_Context_GetDisplayHeight(void)
{
    return m_Context.display_height;
}

int32_t GFX_Context_GetWindowWidth(void)
{
    return m_Context.window_width ? m_Context.window_width
                                  : m_Context.display_width;
}

int32_t GFX_Context_GetWindowHeight(void)
{
    return m_Context.window_height ? m_Context.window_height
                                   : m_Context.display_height;
}

int32_t GFX_Context_GetScreenWidth(void)
{
    return m_Context.screen_width;
}

int32_t GFX_Context_GetScreenHeight(void)
{
    return m_Context.screen_height;
}

void GFX_Context_SetupViewport(void)
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

void GFX_Context_SwapBuffers(void)
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

void GFX_Context_SetRendered(void)
{
    m_Context.is_rendered = true;
}

bool GFX_Context_IsRendered(void)
{
    return m_Context.is_rendered;
}

void GFX_Context_ScheduleScreenshot(const char *path)
{
    m_Context.scheduled_screenshot_path = Memory_DupStr(path);
}

GFX_2D_Renderer *GFX_Context_GetRenderer2D(void)
{
    return &m_Context.renderer_2d;
}

GFX_3D_Renderer *GFX_Context_GetRenderer3D(void)
{
    return &m_Context.renderer_3d;
}
