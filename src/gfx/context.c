#include "gfx/context.h"

#include "config.h"
#include "game/shell.h"
#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/utils.h"
#include "gfx/screenshot.h"
#include "log.h"
#include "memory.h"

#include <SDL2/SDL.h>
#include <string.h>

typedef struct GFX_Context {
    SDL_GLContext context;
    SDL_Window *window_handle;

    bool is_fullscreen; // fullscreen flag
    bool is_rendered; // rendering flag
    int32_t display_width;
    int32_t display_height;
    int32_t screen_width;
    int32_t screen_height;
    int32_t window_width;
    int32_t window_height;
    char *scheduled_screenshot_path;
    GFX_FBO_Renderer renderer_fbo;
    GFX_2D_Renderer renderer_2d;
    GFX_3D_Renderer renderer_3d;
} GFX_Context;

static GFX_Context m_Context = { 0 };

static bool GFX_Context_IsExtensionSupported(const char *name);
static void GFX_Context_CheckExtensionSupport(const char *name);
static void GFX_Context_SwitchToWindowViewport(void);
static void GFX_Context_SwitchToRenderViewport(void);

static bool GFX_Context_IsExtensionSupported(const char *name)
{
    int number_of_extensions;

    glGetIntegerv(GL_NUM_EXTENSIONS, &number_of_extensions);
    GFX_GL_CheckError();

    for (int i = 0; i < number_of_extensions; i++) {
        const char *gl_ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
        GFX_GL_CheckError();

        if (gl_ext && !strcmp(gl_ext, name)) {
            return true;
        }
    }
    return false;
}

static void GFX_Context_CheckExtensionSupport(const char *name)
{
    LOG_INFO(
        "%s supported: %s", name,
        GFX_Context_IsExtensionSupported(name) ? "yes" : "no");
}

static void GFX_Context_SwitchToWindowViewport(void)
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
    GFX_GL_CheckError();
}

static void GFX_Context_SwitchToRenderViewport(void)
{
    glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);
    GFX_GL_CheckError();
}

void GFX_Context_Attach(void *window_handle)
{
    const char *shading_ver;

    if (m_Context.window_handle) {
        return;
    }

    LOG_INFO("Attaching to window %p", window_handle);

    m_Context.window_handle = window_handle;
    m_Context.context = SDL_GL_CreateContext(m_Context.window_handle);

    if (!m_Context.context) {
        Shell_ExitSystem("Can't create OpenGL context");
    }

    if (SDL_GL_MakeCurrent(m_Context.window_handle, m_Context.context)) {
        Shell_ExitSystem("Can't activate OpenGL context");
    }

    LOG_INFO("OpenGL vendor string:   %s", glGetString(GL_VENDOR));
    LOG_INFO("OpenGL renderer string: %s", glGetString(GL_RENDERER));
    LOG_INFO("OpenGL version string:  %s", glGetString(GL_VERSION));

    shading_ver = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (shading_ver != NULL) {
        LOG_INFO("Shading version string: %s", shading_ver);
    } else {
        GFX_GL_CheckError();
    }

    GFX_Context_CheckExtensionSupport("GL_ARB_explicit_attrib_location");
    GFX_Context_CheckExtensionSupport("GL_EXT_gpu_shader4");

    // get screen dimensions
    SDL_DisplayMode DM;

    SDL_GetDesktopDisplayMode(0, &DM);
    m_Context.screen_width = DM.w;
    m_Context.screen_height = DM.h;
    LOG_INFO("GetDesktopDisplayMode=%dx%d", DM.w, DM.h);

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);
    GFX_GL_CheckError();

    // VSync defaults to on unless user disabled it in runtime json
    SDL_GL_SetSwapInterval(1);

    GFX_FBO_Renderer_Init(&m_Context.renderer_fbo);
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

    SDL_GL_MakeCurrent(NULL, NULL);

    if (m_Context.context != NULL) {
        SDL_GL_DeleteContext(m_Context.context);
        m_Context.context = NULL;
    }
    m_Context.window_handle = NULL;
}

void GFX_Context_SetVSync(bool vsync)
{
    SDL_GL_SetSwapInterval(vsync);
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

void GFX_Context_SwapBuffers(void)
{
    glFinish();
    GFX_GL_CheckError();

    GFX_Context_SwitchToWindowViewport();

    GFX_FBO_Renderer_Render(&m_Context.renderer_fbo);

    // TODO: check if this still works and doesn't crash
    if (m_Context.scheduled_screenshot_path) {
        GFX_Screenshot_CaptureToFile(m_Context.scheduled_screenshot_path);
        Memory_FreePointer(&m_Context.scheduled_screenshot_path);
    }

    SDL_GL_SwapWindow(m_Context.window_handle);

    GFX_Context_SwitchToRenderViewport();

    // TODO: this doesn't work when changing the resolution
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GFX_GL_CheckError();

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

GFX_FBO_Renderer *GFX_Context_GetRendererFBO(void)
{
    return &m_Context.renderer_fbo;
}
