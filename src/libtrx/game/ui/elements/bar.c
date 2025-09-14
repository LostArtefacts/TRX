#include "game/ui/elements/bar.h"

#include "config.h"
#include "game/output.h"
#include "game/scaler.h"
#include "game/ui/draw.h"
#include "game/ui/helpers.h"
#include "utils.h"

#define M_COLOR_STEPS 5

typedef struct {
    float basic_scale;
    RGBA_8888 rgb_bgnd;
    RGBA_8888 rgb_border_light;
    RGBA_8888 rgb_border_dark;
    int32_t (*color_map)[M_COLOR_STEPS];
} M_LOOK;

typedef struct {
    UI_BAR_SETTINGS settings;
    M_LOOK *look;
} M_DATA;

static int32_t m_ColorMapTR1[][M_COLOR_STEPS] = {
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
    [BC_GREEN2] = { 0x10984D, 0x18B85B, 0x10984D, 0x0B7733, 0x075819 },
    // clang-format on
};

static int32_t m_ColorMapTR2[][M_COLOR_STEPS] = {
    // clang-format off
    [BC_RED]    = { 0xFF0000, 0xFF8000, 0xFF0000, 0xFF0000, 0xFF0000 },
    [BC_BLUE]   = { 0x0000FF, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x0000FF },
    [BC_GREY]   = { 0x4C504C, 0xA0A0A0, 0x4C504C, 0x4C504C, 0x4C504C },
    [BC_GOLD]   = { 0x7C5E25, 0xA1833C, 0x7C5E25, 0x7C5E25, 0x7C5E25 },
    [BC_SILVER] = { 0x969696, 0xE6E6E6, 0x969696, 0x969696, 0x969696 },
    [BC_GREEN]  = { 0x00A000, 0x82E61E, 0x00A000, 0x00A000, 0x00A000 },
    [BC_GOLD2]  = { 0x966400, 0xFFC800, 0x966400, 0x966400, 0x966400 },
    [BC_BLUE2]  = { 0x00AADC, 0x00C8FF, 0x008CB9, 0x008CB9, 0x008CB9 },
    [BC_PINK]   = { 0xFF40DF, 0xFF96C8, 0xFF40DF, 0xFF40DF, 0xFF40DF },
    [BC_PURPLE] = { 0x461E6B, 0xFF40DF, 0x461E6B, 0x461E6B, 0x461E6B },
    [BC_GREEN2] = { 0x0BAA6B, 0x2EE708, 0x0BAA6B, 0x0BAA6B, 0x0BAA6B },
    // clang-format on
};

static M_LOOK m_Looks[] = {
    [BAR_LOOK_TR1] = {
        .basic_scale = 1.0f,
        .rgb_bgnd = { 0, 0, 0, 0xFF },
        .rgb_border_light = { 0x35, 0x35, 0x35, 0xFF },
        .rgb_border_dark = { 0x35, 0x35, 0x35, 0xFF },
        .color_map = m_ColorMapTR1,
    },
    [BAR_LOOK_TR2] = {
        .basic_scale = 0.75f,
        .rgb_bgnd = { 0, 0, 0, 0xFF },
        .rgb_border_light = { 0xFF, 0xFF, 0xFF, 0xFF },
        .rgb_border_dark = { 0x40, 0x40, 0x40, 0xFF },
        .color_map = m_ColorMapTR2,
    },
};

static void M_Measure(UI_NODE *node);
static void M_Draw(const UI_NODE *node);

static const UI_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = UI_LayoutBasic,
    .draw = M_Draw,
};

static RGBA_8888 M_GetColor(
    const M_LOOK *const look, BAR_COLOR color, int32_t idx);

static RGBA_8888 M_GetColor(
    const M_LOOK *const look, const BAR_COLOR color, const int32_t idx)
{
    const int32_t value = look->color_map[color][idx];
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
    const float scale =
        Scaler_GetScale(SCALER_TARGET_BAR) * data->look->basic_scale;
    node->measure_w = data->settings.w * scale;
    node->measure_h = data->settings.h * scale;
}

static void M_Draw(const UI_NODE *const node)
{
    M_DATA *const data = node->data;
    const UI_BAR_SETTINGS *const settings = &data->settings;

    float percent = settings->value / (float)MAX(1, settings->max_value);
    CLAMP(percent, 0.0f, 1.0f);

    // Convert everything to screen coordinates
    const float x = UI_ScaleX(node->x);
    const float y = UI_ScaleY(node->y);
    const float w = UI_ScaleX(node->w);
    const float h = UI_ScaleY(node->h);
    const float scale =
        Scaler_GetScale(SCALER_TARGET_BAR) * data->look->basic_scale;
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
        outer_rect.h, data->look->rgb_border_light);
    UI_ScheduleDrawScreenFlatQuad(
        outer_rect.x + border, outer_rect.y + border, M_COLOR_STEPS * 3,
        outer_rect.w - border, outer_rect.h - border,
        data->look->rgb_border_dark);

    // Draw background
    UI_ScheduleDrawScreenFlatQuad(
        inner_rect.x, inner_rect.y, M_COLOR_STEPS * 2, inner_rect.w,
        inner_rect.h, data->look->rgb_bgnd);

    if (percent == 0.0f) {
        return;
    }

    // Draw fill
    BAR_COLOR color;
    switch (settings->type) {
    case UI_BAR_LARA_HP:
        color = g_Config.ui.lara_health_bar.color;
        break;
    case UI_BAR_LARA_AIR:
        color = g_Config.ui.lara_air_bar.color;
        break;
    case UI_BAR_LARA_STAMINA:
        color = g_Config.ui.lara_sprint_bar.color;
        break;
    case UI_BAR_ENEMY_HP:
        color = g_Config.ui.enemy_health_bar.color;
        break;
    case UI_BAR_ALLY_HP:
        color = g_Config.ui.enemy_health_bar.color_allies;
        break;
    case UI_BAR_PROGRESS:
        color = TR_VERSION == 2 ? BC_GREEN : BC_GOLD;
        break;
    }
    if (g_Config.ui.enable_smooth_bars) {
        for (int32_t i = 0; i < M_COLOR_STEPS - 1; i++) {
            const RGBA_8888 c1 = M_GetColor(data->look, color, i);
            const RGBA_8888 c2 = M_GetColor(data->look, color, i + 1);
            const int32_t lsy =
                bar_rect.y + i * bar_rect.h / (M_COLOR_STEPS - 1);
            const int32_t lsh =
                bar_rect.y + (i + 1) * bar_rect.h / (M_COLOR_STEPS - 1) - lsy;
            UI_ScheduleDrawScreenGradientQuad(
                bar_rect.x, lsy, 0, bar_rect.w, lsh, c1, c1, c2, c2);
        }
    } else {
        for (int32_t i = 0; i < M_COLOR_STEPS; i++) {
            const RGBA_8888 c = M_GetColor(data->look, color, i);
            const int32_t lsy = bar_rect.y + i * bar_rect.h / M_COLOR_STEPS;
            const int32_t lsh =
                bar_rect.y + (i + 1) * bar_rect.h / M_COLOR_STEPS - lsy;
            UI_ScheduleDrawScreenFlatQuad(
                bar_rect.x, lsy, 0, bar_rect.w, lsh, c);
        }
    }
}

void UI_Bar(const UI_BAR_SETTINGS settings)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->settings = settings;
    data->look = &m_Looks[g_Config.ui.bar_look];
    UI_AddChild(node);
}
