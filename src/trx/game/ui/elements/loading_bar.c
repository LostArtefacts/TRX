#include <trx/game/ui/elements/loading_bar.h>

#include <trx/core/utils.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/helpers.h>
#include <trx/game/ui/scaler.h>

typedef struct {
    float width;
    float progress;
} M_DATA;

static RGBA_8888 m_BorderColor = { 0xFF, 0xFF, 0xFF, 0xFF };
static RGBA_8888 m_BackgroundColor = { 0x00, 0x00, 0x00, 0xFF };
static RGBA_8888 m_FillEdgeColor = { 0x1A, 0x05, 0x15, 0xFF };
static RGBA_8888 m_FillCenterColor = { 0x9F, 0x1F, 0x80, 0xFF };

static void M_Measure(UI_NODE *const node)
{
    const M_DATA *const data = node->data;
    node->measure_w = data->width;
    node->measure_h = UI_LOADING_BAR_HEIGHT * UI_Scaler_GetTextScale();
}

static void M_Draw(const UI_NODE *const node)
{
    const M_DATA *const data = node->data;

    const int32_t x0 = UI_ScaleX(node->x);
    const int32_t y0 = UI_ScaleY(node->y);
    const int32_t x1 = UI_ScaleX(node->x + node->w);
    const int32_t y1 = UI_ScaleY(node->y + node->h);

    const int32_t border = MAX(1, UI_ScaleY(node->y + 1.0f) - y0);

    UI_ScheduleDrawScreenFlatQuad(x0, y0, 0, x1 - x0, y1 - y0, m_BorderColor);

    const int32_t in_x0 = x0 + border;
    const int32_t in_y0 = y0 + border;
    const int32_t in_x1 = x1 - border;
    const int32_t in_y1 = y1 - border;
    if (in_x1 <= in_x0 || in_y1 <= in_y0) {
        return;
    }
    UI_ScheduleDrawScreenFlatQuad(
        in_x0, in_y0, 0, in_x1 - in_x0, in_y1 - in_y0, m_BackgroundColor);

    const int32_t fill_w = (in_x1 - in_x0) * data->progress;
    if (fill_w <= 0) {
        return;
    }
    const int32_t mid_y = (in_y0 + in_y1) / 2;
    UI_ScheduleDrawScreenGradientQuad(
        in_x0, in_y0, 0, fill_w, mid_y - in_y0, m_FillEdgeColor,
        m_FillEdgeColor, m_FillCenterColor, m_FillCenterColor);
    UI_ScheduleDrawScreenGradientQuad(
        in_x0, mid_y, 0, fill_w, in_y1 - mid_y, m_FillCenterColor,
        m_FillCenterColor, m_FillEdgeColor, m_FillEdgeColor);
}

void UI_LoadingBar(const float width, float progress)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = UI_LayoutBasic,
            .draw = M_Draw,
        },
        sizeof(M_DATA));
    M_DATA *const data = node->data;
    CLAMP(progress, 0.0f, 1.0f);
    data->width = width;
    data->progress = progress;
    UI_AddChild(node);
}
