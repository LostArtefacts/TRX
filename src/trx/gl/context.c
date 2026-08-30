#include <trx/gl/context.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/shell.h>
#include <trx/game/viewport.h>
#include <trx/gl/renderer.h>
#include <trx/gl/screenshot.h>
#include <trx/gl/utils.h>

#include <GL/glew.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    SDL_GLContext context;
    SDL_Window *window_handle;
    VIEWPORT_SPACE space;

    TRX_GL_CONFIG config;

    // Size of the SDL window.
    int32_t window_width;
    int32_t window_height;

    char *scheduled_screenshot_path;
    TRX_GL_RENDERER *renderer;
} TRX_GL_CONTEXT;

extern RGBA_F Output_GetBackgroundColor(void);

static TRX_GL_CONTEXT m_Context = {};

// The driver reports the same message once a frame, which the log has no room
// for. Only the first of a run is logged. The count of the rest names the
// message it belongs to, because other modules log between the two lines.
static bool m_HasLastDebug = false;
static GLuint m_LastDebugID = 0;
static uint32_t m_LastDebugRepeats = 0;

static bool M_IsExtensionSupported(const char *name)
{
    int number_of_extensions;

    glGetIntegerv(GL_NUM_EXTENSIONS, &number_of_extensions);
    TRX_GL_CheckError();

    for (int i = 0; i < number_of_extensions; i++) {
        const char *gl_ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
        TRX_GL_CheckError();

        if (gl_ext && !strcmp(gl_ext, name)) {
            return true;
        }
    }
    return false;
}

static void M_ReportDebugRepeats(void)
{
    if (m_LastDebugRepeats == 0) {
        return;
    }
    LOG_INFO(
        "OpenGL message #%u repeated %u more times", m_LastDebugID,
        m_LastDebugRepeats);
    m_LastDebugRepeats = 0;
}

static GLvoid GLAPIENTRY M_GLDebug(
    const GLenum source, const GLenum type, const GLuint id,
    const GLenum severity, const GLsizei length, const GLchar *const message,
    const void *const user_param)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    if (m_HasLastDebug && id == m_LastDebugID) {
        m_LastDebugRepeats++;
        return;
    }
    M_ReportDebugRepeats();
    m_HasLastDebug = true;
    m_LastDebugID = id;

    size_t len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
        len--;
    }
    LOG_INFO("%d #%u %.*s", source, id, (int)len, message);
}

void TRX_GL_Context_SwitchToViewport(const VIEWPORT_SPACE space)
{
    const VIEWPORT_RECT rect = Viewport_GetRect(space);
    m_Context.space = space;
    glViewport(rect.x, rect.y, rect.width, rect.height);
    TRX_GL_CheckError();
}

VIEWPORT_SPACE TRX_GL_Context_GetViewport(void)
{
    return m_Context.space;
}

RESULT TRX_GL_Context_Attach(void *window_handle)
{
    const char *shading_ver;

    if (m_Context.window_handle) {
        return FAIL("the OpenGL context is already attached");
    }

    LOG_INFO("Attaching to window %p", window_handle);
    m_Context.context = SDL_GL_CreateContext(window_handle);
    if (m_Context.context == nullptr) {
        return FAIL(
            "the OpenGL context could not be created: %s", SDL_GetError());
    }

    m_Context.config.line_width = 1;
    m_Context.config.enable_wireframe = false;
    SDL_GetWindowSize(
        window_handle, &m_Context.window_width, &m_Context.window_height);

    m_Context.window_handle = window_handle;

    FAIL_IF(
        SDL_GL_MakeCurrent(m_Context.window_handle, m_Context.context),
        "the OpenGL context could not be activated: %s", SDL_GetError());

    const GLenum err = glewInit();
    if (err != GLEW_OK) {
        if (err != 4) {
            return FAIL(
                "GLEW could not be brought up to load the OpenGL "
                "extensions: %d",
                err);
        }
        // https://github.com/nigels-com/glew/issues/417
        LOG_WARNING("GLEW failed to init: %d", err);
    }

    LOG_INFO("OpenGL vendor string:   %s", glGetString(GL_VENDOR));
    LOG_INFO("OpenGL renderer string: %s", glGetString(GL_RENDERER));
    LOG_INFO("OpenGL version string:  %s", glGetString(GL_VERSION));

    shading_ver = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (shading_ver != nullptr) {
        LOG_INFO("Shading version string: %s", shading_ver);
    } else {
        TRX_GL_CheckError();
    }

    glClearColor(0, 0, 0, 0);
    glClearDepth(1);
    TRX_GL_CheckError();

    // VSync defaults to on unless user disabled it in runtime json
    SDL_GL_SetSwapInterval(1);

#if DEBUG
    if (glDebugMessageCallback != nullptr) {
        glDebugMessageCallback(M_GLDebug, nullptr);
    }
    if (glDebugMessageControl != nullptr) {
        glDebugMessageControl(
            GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0,
            nullptr, GL_FALSE);
    }
    glEnable(GL_DEBUG_OUTPUT);
#endif

    m_Context.renderer = &g_TRX_GL_Renderer;
    if (m_Context.renderer->init != nullptr) {
        MUST(m_Context.renderer->init(m_Context.renderer, &m_Context.config));
    }

    return OK;
}

char *TRX_GL_Context_DescribeDriver(void *const window_handle)
{
    SDL_GL_ResetAttributes();
    SDL_GLContext context = SDL_GL_CreateContext(window_handle);
    if (context == nullptr) {
        LOG_ERROR("Can't create fallback OpenGL context: %s", SDL_GetError());
        return nullptr;
    }

    char *result = nullptr;
    if (SDL_GL_MakeCurrent(window_handle, context) == 0) {
        const char *const renderer = (const char *)glGetString(GL_RENDERER);
        const char *const version = (const char *)glGetString(GL_VERSION);
        if (renderer != nullptr && version != nullptr) {
            result = String_Format("%s (OpenGL %s)", renderer, version);
        } else if (version != nullptr) {
            result = String_Format("OpenGL %s", version);
        }
        SDL_GL_MakeCurrent(window_handle, nullptr);
    }

    SDL_GL_DeleteContext(context);
    return result;
}

void TRX_GL_Context_Detach(void)
{
    M_ReportDebugRepeats();

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

void TRX_GL_Context_SetDisplayFilter(const TEXTURE_FILTER filter)
{
    m_Context.config.display_filter = filter;
}

void TRX_GL_Context_SetMultisamplingFactor(const int32_t factor)
{
    m_Context.config.multisampling_factor = factor;
}

void TRX_GL_Context_SetDitherMode(const DITHER_MODE mode)
{
    m_Context.config.dither_mode = mode;
}

bool TRX_GL_Context_GetWireframeMode(void)
{
    return m_Context.config.enable_wireframe;
}

void TRX_GL_Context_SetWireframeMode(const bool enable)
{
    m_Context.config.enable_wireframe = enable;
}

void TRX_GL_Context_SetLineWidth(const int32_t line_width)
{
    m_Context.config.line_width = line_width;
}

void TRX_GL_Context_SetVSync(bool vsync)
{
    SDL_GL_SetSwapInterval(vsync);
}

void *TRX_GL_Context_GetWindowHandle(void)
{
    return m_Context.window_handle;
}

void TRX_GL_Context_Clear(void)
{
    const RGBA_F white = { 1.0f, 1.0f, 1.0f, 0.0f };
    const RGBA_F background = Output_GetBackgroundColor();
    const RGBA_F black = { 0.0f, 0.0f, 0.0f, 0.0f };
    const RGBA_F color =
        m_Context.space == VIEWPORT_GAME && m_Context.config.enable_wireframe
        ? white
        : m_Context.space == VIEWPORT_GAME ? background
                                           : black;
    glClearBufferfv(GL_COLOR, 0, &color.r);
}

void TRX_GL_Context_SwapBuffers(void)
{
    glFlush();
    TRX_GL_CheckError();

    if (m_Context.renderer != nullptr
        && m_Context.renderer->swap_buffers != nullptr) {
        m_Context.renderer->swap_buffers(m_Context.renderer);
    }
}

void TRX_GL_Context_ScheduleScreenshot(const char *path)
{
    Memory_FreePointer(&m_Context.scheduled_screenshot_path);
    m_Context.scheduled_screenshot_path = Memory_DupStr(path);
}

const char *TRX_GL_Context_GetScheduledScreenshotPath(void)
{
    return m_Context.scheduled_screenshot_path;
}

void TRX_GL_Context_ClearScheduledScreenshotPath(void)
{
    Memory_FreePointer(&m_Context.scheduled_screenshot_path);
}

TRX_GL_CONFIG *TRX_GL_Context_GetConfig(void)
{
    return &m_Context.config;
}
