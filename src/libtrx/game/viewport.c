#include "game/viewport.h"

#include "config.h"
#include "game/shell.h"
#include "log.h"

static VIEWPORT_RECT m_Rects[VIEWPORT_NUMBER_OF] = {
    [VIEWPORT_GAME] = {
        .width = 800,
        .height = 600,
    },
};

VIEWPORT_RECT *g_Viewport_Rects = m_Rects;

int16_t Viewport_GetEffectiveFOV(void)
{
    return Viewport_GetSystemFOV() != -1 ? Viewport_GetSystemFOV()
                                         : Viewport_GetUserFOV();
}

int32_t Viewport_GetMinX(const VIEWPORT_SPACE space)
{
    return m_Rects[space].x;
}

int32_t Viewport_GetMinY(const VIEWPORT_SPACE space)
{
    return m_Rects[space].y;
}

int32_t Viewport_GetMaxX(const VIEWPORT_SPACE space)
{
    return m_Rects[space].x + m_Rects[space].width;
}

int32_t Viewport_GetMaxY(const VIEWPORT_SPACE space)
{
    return m_Rects[space].y + m_Rects[space].height;
}

int32_t Viewport_GetCenterX(const VIEWPORT_SPACE space)
{
    return (m_Rects[space].x + m_Rects[space].width) / 2;
}

int32_t Viewport_GetCenterY(const VIEWPORT_SPACE space)
{
    return (m_Rects[space].y + m_Rects[space].height) / 2;
}

int32_t Viewport_GetWidth(const VIEWPORT_SPACE space)
{
    return m_Rects[space].width;
}

int32_t Viewport_GetHeight(const VIEWPORT_SPACE space)
{
    return m_Rects[space].height;
}

VIEWPORT_RECT Viewport_GetRect(const VIEWPORT_SPACE space)
{
    return m_Rects[space];
}

void Viewport_ResetCommon(void)
{
    const SHELL_SIZE size = Shell_GetCurrentSize();
    m_Rects[VIEWPORT_WINDOW].x = 0;
    m_Rects[VIEWPORT_WINDOW].y = 0;
    m_Rects[VIEWPORT_WINDOW].width = size.w;
    m_Rects[VIEWPORT_WINDOW].height = size.h;

    const int32_t max_w =
        m_Rects[VIEWPORT_WINDOW].width * (1.0 - g_Config.rendering.borders);
    const int32_t max_h =
        m_Rects[VIEWPORT_WINDOW].height * (1.0 - g_Config.rendering.borders);

    double aspect_ratio = 0.0;
    switch (g_Config.rendering.aspect_mode) {
    case AM_4_3:
        aspect_ratio = 4.0 / 3.0;
        break;
    case AM_16_9:
        aspect_ratio = 16.0 / 9.0;
        break;
    case AM_ANY:
    default:
        aspect_ratio = (double)max_w / (double)max_h; // just match window
        break;
    }

    // Fit the aspect ratio rectangle within max_w x max_h
    int32_t target_w = max_w;
    int32_t target_h = max_w / aspect_ratio;

    if (target_h > max_h) {
        // too tall, clamp
        target_h = max_h;
        target_w = max_h * aspect_ratio;
    }

    m_Rects[VIEWPORT_TARGET].width = target_w;
    m_Rects[VIEWPORT_TARGET].height = target_h;
    m_Rects[VIEWPORT_TARGET].x =
        (m_Rects[VIEWPORT_WINDOW].width - target_w) / 2;
    m_Rects[VIEWPORT_TARGET].y =
        (m_Rects[VIEWPORT_WINDOW].height - target_h) / 2;
}

void Viewport_Debug(void)
{
    const VIEWPORT_RECT *r;
    r = &m_Rects[VIEWPORT_WINDOW];
    LOG_TRACE("Window viewport: %dx%d+%d,%d", r->width, r->height, r->x, r->y);
    r = &m_Rects[VIEWPORT_TARGET];
    LOG_TRACE("Target viewport: %dx%d+%d,%d", r->width, r->height, r->x, r->y);
    r = &m_Rects[VIEWPORT_GAME];
    LOG_TRACE("Game viewport: %dx%d+%d,%d", r->width, r->height, r->x, r->y);
}
