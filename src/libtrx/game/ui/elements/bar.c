#include "game/ui/elements/bar.h"

#include "config.h"
#include "game/output.h"
#include "game/scaler.h"
#include "game/ui/draw.h"
#include "game/ui/helpers.h"
#include "utils.h"

#define M_COLOR_STEPS 5

typedef struct {
    float x, y, w, h;
} M_RECT_F;

typedef struct M_LOOK {
    float basic_scale;
    void (*draw_border)(const struct M_LOOK *look, M_RECT_F rect, float border);
    void (*draw_background)(const struct M_LOOK *look, M_RECT_F rect);
    void (*draw_fill)(
        const struct M_LOOK *look, const UI_BAR_SETTINGS *settings,
        M_RECT_F rect, float percent);
} M_LOOK;

typedef struct {
    M_LOOK base;
    RGBA_8888 border_light;
    RGBA_8888 border_dark;
    uint32_t color_map[BC_NUMBER_OF][M_COLOR_STEPS];
} M_LOOK_PC;

typedef struct {
    M_LOOK base;
    uint32_t color_map[UI_BAR_NUMBER_OF][2][M_COLOR_STEPS];
} M_LOOK_PS1;

typedef struct {
    UI_BAR_SETTINGS settings;
    const M_LOOK *look;
} M_DATA;

static void M_DrawBackground(const M_LOOK *look, M_RECT_F rect);
static void M_DrawBorderPC(const M_LOOK *look, M_RECT_F rect, float border);
static void M_DrawBorderPS1(const M_LOOK *look, M_RECT_F rect, float border);
static void M_DrawFillPC(
    const M_LOOK *look, const UI_BAR_SETTINGS *settings, M_RECT_F rect,
    float percent);
static void M_DrawFillPS1(
    const M_LOOK *look, const UI_BAR_SETTINGS *settings, M_RECT_F rect,
    float percent);

static const M_LOOK_PC m_LookTR1 = {
    .base = {
        .basic_scale = 1.0f,
        .draw_border = M_DrawBorderPC,
        .draw_background = M_DrawBackground,
        .draw_fill = M_DrawFillPC,
    },
    .border_light = { 0x35, 0x35, 0x35, 0xFF },
    .border_dark = { 0x35, 0x35, 0x35, 0xFF },
    .color_map = {
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
    },
};

static const M_LOOK_PC m_LookTR23PC = {
    .base = {
        .basic_scale = 0.75f,
        .draw_border = M_DrawBorderPC,
        .draw_background = M_DrawBackground,
        .draw_fill = M_DrawFillPC,
    },
    .border_light = { 0xFF, 0xFF, 0xFF, 0xFF },
    .border_dark = { 0x40, 0x40, 0x40, 0xFF },
    .color_map = {
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
    },
};

static const M_LOOK_PS1 m_LookTR23PS1 = {
    .base = {
        .basic_scale = 1.0f,
        .draw_border = M_DrawBorderPS1,
        .draw_background = M_DrawBackground,
        .draw_fill = M_DrawFillPS1,
    },
    .color_map = {
        // clang-format off
        [UI_BAR_LARA_HP] = {
            { 0xB60100, 0xEA0100, 0xB60100, 0x870000, 0x6D0000 },
            { 0x01B900, 0x03FA00, 0x01B900, 0x018800, 0x015A00 },
        },
        [UI_BAR_LARA_AIR] = {
            { 0x00717A, 0x009298, 0x00717A, 0x005D6A, 0x004A5A },
            { 0x007101, 0x009201, 0x007101, 0x005D01, 0x004A00 },
        },
        [UI_BAR_LARA_STAMINA] = {
            { 0xC00100, 0xF00100, 0xC00100, 0x900000, 0x600000 },
            { 0xC0B900, 0xF0E800, 0xC0B900, 0x908A00, 0x605A00 },
        },
        [UI_BAR_LARA_EXPOSURE] = {
            { 0x000170, 0x090091, 0x000170, 0x000053, 0x00003E },
            { 0xC00100, 0xF00100, 0xC00100, 0x900000, 0x600000 },
        },
        [UI_BAR_ENEMY_HP] = {
            { 0xB64C00, 0xED6400, 0xB64C00, 0x843700, 0x6D2D00 },
            { 0xC00100, 0xF00100, 0xC00100, 0x900000, 0x600000 },
        },
        [UI_BAR_ALLY_HP] = {
            { 0xB78900, 0xEEB300, 0xB78900, 0x846300, 0x6D5200 },
            { 0x01B900, 0x03FA00, 0x01B900, 0x018800, 0x015A00 },
        },
        [UI_BAR_PROGRESS] = {
            { 0x2F0030, 0x5F0060, 0x7E007F, 0x5F0060, 0x2F0030 },
            { 0x002E30, 0x015D60, 0x017C7F, 0x015D60, 0x002E30 },
        },
        // clang-format on
    },
};

static const M_LOOK *m_Looks[] = {
    [BAR_LOOK_TR1] = &m_LookTR1.base,
    [BAR_LOOK_TR23_PC] = &m_LookTR23PC.base,
    [BAR_LOOK_TR23_PS1] = &m_LookTR23PS1.base,
};

static RGBA_8888 M_GetColor(const uint32_t value)
{
    return (RGBA_8888) {
        .r = (value >> 16) & 0xFF,
        .g = (value >> 8) & 0xFF,
        .b = (value) & 0xFF,
        .a = 0xFF,
    };
}

static RGBA_8888 M_MixColors(
    const RGBA_8888 c0, const RGBA_8888 c1, const float percent)
{
    return (RGBA_8888) {
        .r = (uint8_t)(c0.r * (1.0f - percent) + c1.r * percent),
        .g = (uint8_t)(c0.g * (1.0f - percent) + c1.g * percent),
        .b = (uint8_t)(c0.b * (1.0f - percent) + c1.b * percent),
        .a = (uint8_t)(c0.a * (1.0f - percent) + c1.a * percent),
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

static void M_DrawBackground(const M_LOOK *const look, const M_RECT_F rect)
{
    UI_ScheduleDrawScreenFlatQuad(
        rect.x, rect.y, 0, rect.w, rect.h, (RGBA_8888) { 0, 0, 0, 255 });
}

static void M_DrawBorderPC(
    const M_LOOK *const look, const M_RECT_F rect, const float border)
{
    const M_LOOK_PC *const pc_look = (const M_LOOK_PC *)look;
    UI_ScheduleDrawScreenFlatQuad(
        rect.x, rect.y, 0, rect.w, rect.h, pc_look->border_light);
    UI_ScheduleDrawScreenFlatQuad(
        rect.x + border, rect.y + border, 0, rect.w - border, rect.h - border,
        pc_look->border_dark);
}

static void M_DrawBorderPS1(
    const M_LOOK *const look, const M_RECT_F rect, const float border)
{
    const RGBA_8888 c1 = { 0x50, 0x82, 0x82, 0xFF };
    const RGBA_8888 c2 = { 0x9F, 0x9F, 0x9F, 0xFF };
    const RGBA_8888 c3 = { 0x29, 0x41, 0x41, 0xFF };
    const RGBA_8888 c4 = { 0x4F, 0x4F, 0x4F, 0xFF };
#if 0
    Output_DrawScreenGradientQuad(
        rect.x - border, rect.y + border, 0, rect.w + border * 2, rect.h - border * 2, c3, c4, c4, c3);
#endif
    Output_DrawScreenGradientQuad(
        rect.x, rect.y, 0, rect.w, rect.h, c1, c2, c2, c1);
}

static void M_DrawFillPC(
    const M_LOOK *const look, const UI_BAR_SETTINGS *const settings,
    const M_RECT_F rect, const float percent)
{
    const M_LOOK_PC *const pc_look = (const M_LOOK_PC *)look;
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
    case UI_BAR_LARA_EXPOSURE:
        color = g_Config.ui.lara_exposure_bar.color;
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
    case UI_BAR_NUMBER_OF:
        color = BC_GOLD;
        break;
    }

    if (g_Config.ui.enable_smooth_bars) {
        for (int32_t i = 0; i < M_COLOR_STEPS - 1; i++) {
            const RGBA_8888 c1 = M_GetColor(pc_look->color_map[color][i]);
            const RGBA_8888 c2 = M_GetColor(pc_look->color_map[color][i + 1]);
            const int32_t lsy = rect.y + i * rect.h / (M_COLOR_STEPS - 1);
            const int32_t lsh =
                rect.y + (i + 1) * rect.h / (M_COLOR_STEPS - 1) - lsy;
            UI_ScheduleDrawScreenGradientQuad(
                rect.x, lsy, 0, rect.w, lsh, c1, c1, c2, c2);
        }
    } else {
        for (int32_t i = 0; i < M_COLOR_STEPS; i++) {
            const RGBA_8888 c = M_GetColor(pc_look->color_map[color][i]);
            const int32_t lsy = rect.y + i * rect.h / M_COLOR_STEPS;
            const int32_t lsh = rect.y + (i + 1) * rect.h / M_COLOR_STEPS - lsy;
            UI_ScheduleDrawScreenFlatQuad(rect.x, lsy, 0, rect.w, lsh, c);
        }
    }
}

static void M_DrawFillPS1(
    const M_LOOK *const look, const UI_BAR_SETTINGS *const settings,
    const M_RECT_F rect, const float percent)
{
    const M_LOOK_PS1 *const ps1_look = (const M_LOOK_PS1 *)look;
    const UI_BAR_TYPE type = settings->type;
    if (g_Config.ui.enable_smooth_bars) {
        for (int32_t i = 0; i < M_COLOR_STEPS - 1; i++) {
            const RGBA_8888 ctl = M_GetColor(ps1_look->color_map[type][0][i]);
            const RGBA_8888 ctr = M_GetColor(ps1_look->color_map[type][1][i]);
            const RGBA_8888 cbl =
                M_GetColor(ps1_look->color_map[type][0][i + 1]);
            const RGBA_8888 cbr =
                M_GetColor(ps1_look->color_map[type][1][i + 1]);
            const RGBA_8888 ctrm = M_MixColors(ctl, ctr, percent);
            const RGBA_8888 cbrm = M_MixColors(cbl, cbr, percent);
            const int32_t lsy = rect.y + i * rect.h / (M_COLOR_STEPS - 1);
            const int32_t lsh =
                rect.y + (i + 1) * rect.h / (M_COLOR_STEPS - 1) - lsy;
            UI_ScheduleDrawScreenGradientQuad(
                rect.x, lsy, 0, rect.w, lsh, ctl, ctrm, cbl, cbrm);
        }
    } else {
        for (int32_t i = 0; i < M_COLOR_STEPS; i++) {
            const RGBA_8888 cl = M_GetColor(ps1_look->color_map[type][0][i]);
            const RGBA_8888 cr = M_GetColor(ps1_look->color_map[type][1][i]);
            const RGBA_8888 crm = M_MixColors(cl, cr, percent);
            const int32_t lsy = rect.y + i * rect.h / M_COLOR_STEPS;
            const int32_t lsh = rect.y + (i + 1) * rect.h / M_COLOR_STEPS - lsy;
            UI_ScheduleDrawScreenGradientQuad(
                rect.x, lsy, 0, rect.w, lsh, cl, crm, cl, crm);
        }
    }
}

static void M_Draw(const UI_NODE *const node)
{
    M_DATA *const data = node->data;
    const UI_BAR_SETTINGS *const settings = &data->settings;

    float percent = settings->value / (float)MAX(1, settings->max_value);
    CLAMP(percent, 0.0f, 1.0f);
    percent = (int32_t)(percent * 100) / 100.0f;

    // Convert everything to screen coordinates
    const float x = UI_ScaleX(node->x);
    const float y = UI_ScaleY(node->y);
    const float w = UI_ScaleX(node->w);
    const float h = UI_ScaleY(node->h);
    const float scale =
        Scaler_GetScale(SCALER_TARGET_BAR) * data->look->basic_scale;
    const float border = ceil(UI_ScaleX(UI_BAR_BORDER * scale));
    const float padding = ceil(UI_ScaleX(UI_BAR_PADDING * scale));
    const M_RECT_F outer_rect = {
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
        .w = (inner_rect.w - padding * 2) * percent,
        .h = inner_rect.h - padding * 2,
    };

    data->look->draw_border(data->look, outer_rect, border);
    data->look->draw_background(data->look, inner_rect);
    if (percent > 0.0f) {
        data->look->draw_fill(data->look, settings, bar_rect, percent);
    }
}

void UI_Bar(const UI_BAR_SETTINGS settings)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = UI_LayoutBasic,
            .draw = M_Draw,
        },
        sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->settings = settings;
    data->look = m_Looks[g_Config.ui.bar_look];
    UI_AddChild(node);
}
