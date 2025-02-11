#include "gfx/context.h"

#include "game/shell.h"
#include "gfx/gl/utils.h"
#include "gfx/renderers/fbo_renderer.h"
#include "gfx/renderers/legacy_renderer.h"
#include "gfx/screenshot.h"
#include "log.h"
#include "memory.h"

#include <GL/glew.h>
#include <SDL2/SDL_video.h>
#include <string.h>

typedef struct {
    SDL_GLContext context;
    SDL_Window *window_handle;

    GFX_CONFIG config;
    GFX_RENDER_MODE render_mode;

    // Size of the OpenGL framebuffer.
    int32_t display_width;
    int32_t display_height;

    // Size of the SDL window.
    int32_t window_width;
    int32_t window_height;
    // How much border (inside the SDL window) to add around the rendered game.
    int32_t window_border;

    char *scheduled_screenshot_path;
    GFX_RENDERER *renderer;
} GFX_CONTEXT;

static GFX_CONTEXT m_Context = {};

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
    // Switch to window viewport at the aspect ratio of the display viewport.
    const int32_t max_w = m_Context.window_width;
    const int32_t max_h = m_Context.window_height;
    int32_t vp_w = m_Context.window_width - m_Context.window_border;
    int32_t vp_h = m_Context.window_height - m_Context.window_border;

    const int32_t hw = m_Context.display_height * vp_w;
    const int32_t wh = m_Context.display_width * vp_h;

    // Create viewport offset if the window has a different
    // aspect ratio than the current display mode.
    if (hw > wh) {
        vp_w = wh / m_Context.display_height;
    } else if (hw < wh) {
        vp_h = hw / m_Context.display_width;
    }

    const int32_t vp_x = (max_w - vp_w) / 2;
    const int32_t vp_y = (max_h - vp_h) / 2;
    glViewport(vp_x, vp_y, vp_w, vp_h);
    GFX_GL_CheckError();
}

void GFX_Context_SwitchToDisplayViewport(void)
{
    glViewport(0, 0, m_Context.display_width, m_Context.display_height);
    GFX_GL_CheckError();
}

bool GFX_Context_Attach(void *window_handle, GFX_GL_BACKEND backend)
{
    const char *shading_ver;

    if (m_Context.window_handle) {
        LOG_ERROR("Context already attached");
        return false;
    }

    LOG_INFO("Attaching to window %p", window_handle);
    m_Context.context = SDL_GL_CreateContext(window_handle);
    if (m_Context.context == nullptr) {
        LOG_ERROR("Can't create OpenGL context: %s", SDL_GetError());
        return false;
    }

    m_Context.config.backend = backend;
    m_Context.config.line_width = 1;
    m_Context.config.enable_wireframe = false;
    m_Context.render_mode = -1;
    SDL_GetWindowSize(
        window_handle, &m_Context.window_width, &m_Context.window_height);
    m_Context.window_border = 0;
    m_Context.display_width = m_Context.window_width;
    m_Context.display_height = m_Context.window_height;

    m_Context.window_handle = window_handle;

    if (SDL_GL_MakeCurrent(m_Context.window_handle, m_Context.context)) {
        Shell_ExitSystemFmt(
            "Can't activate OpenGL context: %s", SDL_GetError());
    }

    // Instruct GLEW to load non-Core Profile extensions with OpenGL 2.1,
    // as we rely on `GL_ARB_explicit_attrib_location` and
    // `GL_EXT_gpu_shader4`.
    if (m_Context.config.backend == GFX_GL_21) {
        glewExperimental = GL_TRUE; // Global state
    }
    if (glewInit() != GLEW_OK) {
        Shell_ExitSystem("Can't initialize GLEW for OpenGL extension loading");
    }

    LOG_INFO("OpenGL vendor string:   %s", glGetString(GL_VENDOR));
    LOG_INFO("OpenGL renderer string: %s", glGetString(GL_RENDERER));
    LOG_INFO("OpenGL version string:  %s", glGetString(GL_VERSION));

    shading_ver = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (shading_ver != nullptr) {
        LOG_INFO("Shading version string: %s", shading_ver);
    } else {
        GFX_GL_CheckError();
    }

    // Check the availability of non-Core Profile extensions for OpenGL 2.1
    if (m_Context.config.backend == GFX_GL_21) {
        M_CheckExtensionSupport("GL_ARB_explicit_attrib_location");
        M_CheckExtensionSupport("GL_EXT_gpu_shader4");
    }

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);
    GFX_GL_CheckError();

    // VSync defaults to on unless user disabled it in runtime json
    SDL_GL_SetSwapInterval(1);
    return true;
}

void GFX_Context_Detach(void)
{
    if (!m_Context.window_handle) {
        return;
    }

    if (m_Context.renderer != nullptr
        && m_Context.renderer->shutdown != nullptr) {
        m_Context.renderer->shutdown(m_Context.renderer);
    }

    SDL_GL_MakeCurrent(nullptr, nullptr);

    if (m_Context.context != nullptr) {
        SDL_GL_DeleteContext(m_Context.context);
        m_Context.context = nullptr;
    }
    m_Context.window_handle = nullptr;
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

void GFX_Context_SetVSync(bool vsync)
{
    SDL_GL_SetSwapInterval(vsync);
}

void GFX_Context_SetWindowBorder(const int32_t border_size)
{
    m_Context.window_border = border_size;
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

    if (m_Context.renderer != nullptr && m_Context.renderer->reset != nullptr) {
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
    if (m_Context.renderer != nullptr
        && m_Context.renderer->shutdown != nullptr) {
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
    if (m_Context.renderer != nullptr && m_Context.renderer->init != nullptr) {
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

    if (m_Context.renderer != nullptr
        && m_Context.renderer->swap_buffers != nullptr) {
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

GFX_CONFIG *GFX_Context_GetConfig(void)
{
    return &m_Context.config;
}
