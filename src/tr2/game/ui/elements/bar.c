#include "game/ui/elements/bar.h"

#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/output.h>
#include <libtrx/game/scaler.h>
#include <libtrx/game/ui/draw.h>
#include <libtrx/game/ui/helpers.h>
#include <libtrx/utils.h>

#include <math.h>
#include <string.h>

#define M_BASIC_SCALE 0.75f
#define M_COLOR_STEPS 6

typedef struct {
    UI_BAR_SETTINGS settings;
} M_DATA;

static int32_t m_ColorMap[][M_COLOR_STEPS] = {
    // clang-format off
    [BC_RED]    = { 0xFF0000, 0xFF8000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000 },
    [BC_BLUE]   = { 0x0000FF, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x0000FF, 0x0000FF },
    [BC_GREY]   = { 0x4C504C, 0xA0A0A0, 0x4C504C, 0x4C504C, 0x4C504C, 0x4C504C },
    [BC_GOLD]   = { 0x7C5E25, 0xA1833C, 0x7C5E25, 0x7C5E25, 0x7C5E25, 0x7C5E25 },
    [BC_SILVER] = { 0x969696, 0xE6E6E6, 0x969696, 0x969696, 0x969696, 0x969696 },
    [BC_GREEN]  = { 0x00A000, 0x82E61E, 0x00A000, 0x00A000, 0x00A000, 0x00A000 },
    [BC_GOLD2]  = { 0x966400, 0xFFC800, 0x966400, 0x966400, 0x966400, 0x966400 },
    [BC_BLUE2]  = { 0x00AADC, 0x00C8FF, 0x008CB9, 0x008CB9, 0x008CB9, 0x008CB9 },
    [BC_PINK]   = { 0xFF40DF, 0xFF96C8, 0xFF40DF, 0xFF40DF, 0xFF40DF, 0xFF40DF },
    [BC_PURPLE] = { 0x461E6B, 0xFF40DF, 0x461E6B, 0x461E6B, 0x461E6B, 0x461E6B },
    [BC_GREEN2] = { 0x0BAA6B, 0x2EE708, 0x0BAA6B, 0x0BAA6B, 0x0BAA6B, 0x0BAA6B },
    // clang-format on
};
#undef C

static void M_Measure(UI_NODE *node);
static void M_Draw(const UI_NODE *node);

static const UI_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = UI_LayoutBasic,
    .draw = M_Draw,
};

static RGBA_8888 M_GetColor(BAR_COLOR color, int32_t idx);

static RGBA_8888 M_GetColor(const BAR_COLOR color, const int32_t idx)
{
    const int32_t value = m_ColorMap[color][idx];
    return (RGBA_8888) {
        .r = (value >> 16) & 0xFF,
        .g = (value >> 8) & 0xFF,
        .b = (value) & 0xFF,
        .a = 0xFF,
    };
}

static void M_Measure(UI_NODE *const node)
{
    M_DATA *const data = node->data;
    const float scale = Scaler_GetScale(SCALER_TARGET_BAR) * M_BASIC_SCALE;
    node->measure_w = data->settings.w * scale;
    node->measure_h = data->settings.h * scale;
}

static void M_Draw(const UI_NODE *const node)
{
    M_DATA *const data = node->data;
    const UI_BAR_SETTINGS *const settings = &data->settings;

    const RGBA_8888 rgb_bgnd = { 0, 0, 0, 0xFF };
    const RGBA_8888 rgb_border_highlight = { 0xFF, 0xFF, 0xFF, 0xFF };
    const RGBA_8888 rgb_border_dark = { 0x40, 0x40, 0x40, 0xFF };

    float percent = settings->value / (float)MAX(1, settings->max_value);
    CLAMP(percent, 0.0f, 1.0f);

    // Convert everything to screen coordinates
    const float x = UI_ScaleX(node->x);
    const float y = UI_ScaleY(node->y);
    const float w = UI_ScaleX(node->w);
    const float h = UI_ScaleY(node->h);
    const float scale = Scaler_GetScale(SCALER_TARGET_BAR) * M_BASIC_SCALE;
    const float border = ceil(UI_ScaleX(UI_BAR_BORDER * scale));
    const float padding = ceil(UI_ScaleX(UI_BAR_PADDING * scale));
    struct {
        float x, y, w, h;
    } outer_rect = {
        .x = x,
        .y = y,
        .w = w,
        .h = h,
    }, inner_rect = {
        .x = outer_rect.x + border,
        .y = outer_rect.y + border,
        .w = outer_rect.w - border * 2,
        .h = outer_rect.h - border * 2,
    }, bar_rect = {
        .x = inner_rect.x + padding,
        .y = inner_rect.y + padding,
        .w = (inner_rect.w - padding * 2) * (int32_t)(percent * 100) / 100,
        .h = inner_rect.h - padding * 2,
    };

    // Draw border
    UI_ScheduleDrawScreenFlatQuad(
        outer_rect.x, outer_rect.y, M_COLOR_STEPS * 4, outer_rect.w,
        outer_rect.h, rgb_border_highlight);
    UI_ScheduleDrawScreenFlatQuad(
        outer_rect.x + border, outer_rect.y + border, M_COLOR_STEPS * 3,
        outer_rect.w - border, outer_rect.h - border, rgb_border_dark);

    // Draw background
    UI_ScheduleDrawScreenFlatQuad(
        inner_rect.x, inner_rect.y, M_COLOR_STEPS * 2, inner_rect.w,
        inner_rect.h, rgb_bgnd);

    if (percent == 0.0f) {
        return;
    }

    // Draw fill
    for (int32_t i = 0; i < M_COLOR_STEPS; i++) {
        const RGBA_8888 color = M_GetColor(settings->color, i);
        const int32_t lsy = bar_rect.y + i * bar_rect.h / M_COLOR_STEPS;
        const int32_t lsh =
            bar_rect.y + (i + 1) * bar_rect.h / M_COLOR_STEPS - lsy;
        UI_ScheduleDrawScreenFlatQuad(
            bar_rect.x, lsy, M_COLOR_STEPS - i, bar_rect.w, lsh, color);
    }
}

void UI_Bar(const UI_BAR_SETTINGS settings)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->settings = settings;
    UI_AddChild(node);
}
