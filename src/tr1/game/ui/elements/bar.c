#include "game/ui/elements/bar.h"

#include "game/output.h"

#include <libtrx/config.h>
#include <libtrx/game/scaler.h>
#include <libtrx/game/ui/helpers.h>
#include <libtrx/utils.h>

#include <math.h>
#include <string.h>

#define M_BORDER 2.0f
#define M_PADDING 2.0f
#define M_COLOR_STEPS 5

typedef struct {
    UI_BAR_SETTINGS settings;
} M_DATA;

static int32_t m_ColorMap[][M_COLOR_STEPS] = {
    // clang-format off
    [BC_GOLD]   = { 0x7C5E25, 0xA1833C, 0x7C5E25, 0x644613, 0x4C2E02 },
    [BC_BLUE]   = { 0x3D717B, 0x65929A, 0x3D717B, 0x1F5D6B, 0x004A5B },
    [BC_GREY]   = { 0x586458, 0x748474, 0x586458, 0x4C504C, 0x303030 },
    [BC_RED]    = { 0xA0281C, 0xB82C20, 0xA0281C, 0x7C2020, 0x541420 },
    [BC_SILVER] = { 0x969696, 0xE6E6E6, 0xC8C8C8, 0x8C8C8C, 0x646464 },
    [BC_GREEN]  = { 0x64BE14, 0x82E61E, 0x64BE14, 0x5A960F, 0x506E0A },
    [BC_GOLD2]  = { 0xDCAA00, 0xFFC800, 0xDCAA00, 0xB98C00, 0x966400 },
    [BC_BLUE2]  = { 0x00AADC, 0x00C8FF, 0x00AADC, 0x008CB9, 0x006496 },
    [BC_PINK]   = { 0xDC8CAA, 0xFF96C8, 0xD282A0, 0xA56478, 0x783C46 },
    [BC_PURPLE] = { 0x341650, 0x461E6B, 0x341650, 0x27113C, 0x1A0B28 },
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
        .a = 255,
    };
}

static void M_Measure(UI_NODE *const node)
{
    M_DATA *const data = node->data;
    const float scale = Scaler_GetScale(SCALER_TARGET_BAR);
    node->measure_w = data->settings.w * scale;
    node->measure_h = data->settings.h * scale;
}

static void M_Draw(const UI_NODE *const node)
{
    M_DATA *const data = node->data;
    const UI_BAR_SETTINGS *const settings = &data->settings;

    const RGBA_8888 rgb_bgnd = { 0, 0, 0, 255 };
    const RGBA_8888 rgb_border = { 53, 53, 53, 255 };

    float percent = settings->value / (float)MAX(1, settings->max_value);
    CLAMP(percent, 0.0f, 1.0f);

    // Convert everything to screen coordinates
    const float x = UI_ScaleX(node->x);
    const float y = UI_ScaleY(node->y);
    const float w = UI_ScaleX(node->w);
    const float h = UI_ScaleY(node->h);
    const float scale = Scaler_GetScale(SCALER_TARGET_BAR);
    const float border = ceil(UI_ScaleX(M_BORDER * scale));
    const float padding = ceil(UI_ScaleX(M_PADDING * scale));
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
    Output_DrawScreenFlatQuad(
        outer_rect.x, outer_rect.y, outer_rect.w, outer_rect.h, rgb_border);

    // Draw background
    Output_DrawScreenFlatQuad(
        inner_rect.x, inner_rect.y, inner_rect.w, inner_rect.h, rgb_bgnd);

    if (percent == 0.0f) {
        return;
    }

    // Draw fill
    if (g_Config.ui.enable_smooth_bars) {
        for (int32_t i = 0; i < M_COLOR_STEPS - 1; i++) {
            const RGBA_8888 c1 = M_GetColor(settings->color, i);
            const RGBA_8888 c2 = M_GetColor(settings->color, i + 1);
            const int32_t lsy =
                bar_rect.y + i * bar_rect.h / (M_COLOR_STEPS - 1);
            const int32_t lsh =
                bar_rect.y + (i + 1) * bar_rect.h / (M_COLOR_STEPS - 1) - lsy;
            Output_DrawScreenGradientQuad(
                bar_rect.x, lsy, bar_rect.w, lsh, c1, c1, c2, c2);
        }
    } else {
        for (int32_t i = 0; i < M_COLOR_STEPS; i++) {
            const RGBA_8888 color = M_GetColor(settings->color, i);
            const int32_t lsy = bar_rect.y + i * bar_rect.h / M_COLOR_STEPS;
            const int32_t lsh =
                bar_rect.y + (i + 1) * bar_rect.h / M_COLOR_STEPS - lsy;
            Output_DrawScreenFlatQuad(bar_rect.x, lsy, bar_rect.w, lsh, color);
        }
    }
}

void UI_Bar(const UI_BAR_SETTINGS settings)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->settings = settings;
    UI_AddChild(node);
}
