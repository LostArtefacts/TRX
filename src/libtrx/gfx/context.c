#include "gfx/context.h"

#include "game/shell.h"
#include "gfx/gl/gl_core_3_3.h"
#include "gfx/gl/utils.h"
#include "gfx/renderers/fbo_renderer.h"
#include "gfx/renderers/legacy_renderer.h"
#include "gfx/screenshot.h"
#include "log.h"
#include "memory.h"

#include <SDL2/SDL_video.h>
#include <string.h>

typedef struct {
    SDL_GLContext context;
    SDL_Window *window_handle;

    GFX_CONFIG config;
    GFX_RENDER_MODE render_mode;
    int32_t display_width;
    int32_t display_height;
    int32_t window_width;
    int32_t window_height;

    char *scheduled_screenshot_path;
    GFX_RENDERER *renderer;
    GFX_2D_RENDERER renderer_2d;
    GFX_3D_RENDERER renderer_3d;
} GFX_CONTEXT;

static GFX_CONTEXT m_Context = { 0 };

static bool M_IsExtensionSupported(const char *name);
static void M_CheckExtensionSupport(const char *name);

static bool M_IsExtensionSupported(const char *name)
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

static void M_CheckExtensionSupport(const char *name)
{
    LOG_INFO(
        "%s supported: %s", name, M_IsExtensionSupported(name) ? "yes" : "no");
}

void GFX_Context_SwitchToWindowViewport(void)
{
    glViewport(0, 0, m_Context.window_width, m_Context.window_height);
    GFX_GL_CheckError();
}

void GFX_Context_SwitchToWindowViewportAR(void)
{
    // switch to window viewport at the aspect ratio of the display viewport
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

void GFX_Context_SwitchToDisplayViewport(void)
{
    glViewport(0, 0, m_Context.display_width, m_Context.display_height);
    GFX_GL_CheckError();
}

void GFX_Context_SetupEnvironment(void)
{
    switch (GFX_GL_DEFAULT_BACKEND) {
    case GFX_GL_33C:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        break;
    }
}

void GFX_Context_Attach(void *window_handle)
{
    const char *shading_ver;

    if (m_Context.window_handle) {
        return;
    }

    LOG_INFO("Attaching to window %p", window_handle);

    m_Context.config.line_width = 1;
    m_Context.config.enable_wireframe = false;
    m_Context.render_mode = -1;
    SDL_GetWindowSize(
        window_handle, &m_Context.window_width, &m_Context.window_height);
    m_Context.display_width = m_Context.window_width;
    m_Context.display_height = m_Context.window_height;

    m_Context.window_handle = window_handle;

    m_Context.context = SDL_GL_CreateContext(m_Context.window_handle);

    if (m_Context.context == NULL) {
        Shell_ExitSystemFmt("Can't create OpenGL context: %s", SDL_GetError());
    }

    if (SDL_GL_MakeCurrent(m_Context.window_handle, m_Context.context)) {
        Shell_ExitSystemFmt(
            "Can't activate OpenGL context: %s", SDL_GetError());
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

    // Check the availability of non-Core Profile extensions for OpenGL 2.1
    if (GFX_GL_DEFAULT_BACKEND == GFX_GL_21) {
        M_CheckExtensionSupport("GL_ARB_explicit_attrib_location");
        M_CheckExtensionSupport("GL_EXT_gpu_shader4");
    }

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);
    GFX_GL_CheckError();

    // VSync defaults to on unless user disabled it in runtime json
    SDL_GL_SetSwapInterval(1);

    GFX_2D_Renderer_Init(&m_Context.renderer_2d);
    GFX_3D_Renderer_Init(&m_Context.renderer_3d, &m_Context.config);
}

void GFX_Context_Detach(void)
{
    if (!m_Context.window_handle) {
        return;
    }

    if (m_Context.renderer != NULL && m_Context.renderer->shutdown != NULL) {
        m_Context.renderer->shutdown(m_Context.renderer);
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

void GFX_Context_SetDisplayFilter(const GFX_TEXTURE_FILTER filter)
{
    m_Context.config.display_filter = filter;
}

void GFX_Context_SetWireframeMode(const bool enable)
{
    m_Context.config.enable_wireframe = enable;
}

void GFX_Context_SetLineWidth(const int32_t line_width)
{
    m_Context.config.line_width = line_width;
}

void GFX_Context_SetAnisotropyFilter(float value)
{
    GFX_GL_Sampler_Bind(&m_Context.renderer_3d.sampler, 0);
    GFX_GL_Sampler_Parameterf(
        &m_Context.renderer_3d.sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, value);
}

void GFX_Context_SetVSync(bool vsync)
{
    SDL_GL_SetSwapInterval(vsync);
}

void GFX_Context_SetWindowSize(int32_t width, int32_t height)
{
    LOG_INFO("Window size: %dx%d", width, height);
    m_Context.window_width = width;
    m_Context.window_height = height;
}

void GFX_Context_SetDisplaySize(int32_t width, int32_t height)
{
    if (width == m_Context.display_width
        && height == m_Context.display_height) {
        return;
    }

    LOG_INFO("Display size: %dx%d", width, height);
    if (width <= 0 || height <= 0) {
        LOG_INFO("invalid size, ignoring");
        return;
    }

    m_Context.display_width = width;
    m_Context.display_height = height;

    if (m_Context.renderer != NULL && m_Context.renderer->reset != NULL) {
        m_Context.renderer->reset(m_Context.renderer);
    }
}

void GFX_Context_SetRenderingMode(GFX_RENDER_MODE target_mode)
{
    GFX_RENDER_MODE current_mode = m_Context.render_mode;
    if (current_mode == target_mode) {
        return;
    }

    LOG_INFO("Render mode: %d", target_mode);
    if (m_Context.renderer != NULL && m_Context.renderer->shutdown != NULL) {
        m_Context.renderer->shutdown(m_Context.renderer);
    }
    switch (target_mode) {
    case GFX_RM_FRAMEBUFFER:
        m_Context.renderer = &g_GFX_Renderer_FBO;
        break;
    case GFX_RM_LEGACY:
        m_Context.renderer = &g_GFX_Renderer_Legacy;
        break;
    }
    if (m_Context.renderer != NULL && m_Context.renderer->init != NULL) {
        m_Context.renderer->init(m_Context.renderer, &m_Context.config);
    }
    m_Context.render_mode = target_mode;
}

void *GFX_Context_GetWindowHandle(void)
{
    return m_Context.window_handle;
}

int32_t GFX_Context_GetDisplayWidth(void)
{
    return m_Context.display_width;
}

int32_t GFX_Context_GetDisplayHeight(void)
{
    return m_Context.display_height;
}

void GFX_Context_Clear(void)
{
    if (m_Context.config.enable_wireframe) {
        glClearColor(1.0, 1.0, 1.0, 0.0);
    } else {
        glClearColor(0.0, 0.0, 0.0, 0.0);
    }
    glClear(GL_COLOR_BUFFER_BIT);
}

void GFX_Context_SwapBuffers(void)
{
    glFinish();
    GFX_GL_CheckError();

    if (m_Context.renderer != NULL
        && m_Context.renderer->swap_buffers != NULL) {
        m_Context.renderer->swap_buffers(m_Context.renderer);
    }
}

void GFX_Context_ScheduleScreenshot(const char *path)
{
    Memory_FreePointer(&m_Context.scheduled_screenshot_path);
    m_Context.scheduled_screenshot_path = Memory_DupStr(path);
}

const char *GFX_Context_GetScheduledScreenshotPath(void)
{
    return m_Context.scheduled_screenshot_path;
}

void GFX_Context_ClearScheduledScreenshotPath(void)
{
    Memory_FreePointer(&m_Context.scheduled_screenshot_path);
}

GFX_2D_RENDERER *GFX_Context_GetRenderer2D(void)
{
    return &m_Context.renderer_2d;
}

GFX_3D_RENDERER *GFX_Context_GetRenderer3D(void)
{
    return &m_Context.renderer_3d;
}
