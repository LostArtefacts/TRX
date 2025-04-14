#include "game/ui2/elements/resize.h"

#include "config.h"
#include "game/ui2/helpers.h"

typedef struct {
    float x;
    float y;
} M_DATA;

static void M_Measure(UI2_NODE *node);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = UI2_LayoutWrapper,
    .draw = UI2_DrawWrapper,
};

static void M_Measure(UI2_NODE *const node)
{
    UI2_MeasureWrapper(node);
    const M_DATA *const data = node->data;
    if (data->x >= 0.0f) {
        node->measure_w = data->x;
    }
    if (data->y >= 0.0f) {
        node->measure_h = data->y;
    }
}

void UI2_BeginResize(const float x, const float y)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->x = x * g_Config.ui.text_scale;
    data->y = y * g_Config.ui.text_scale;
    UI2_AddChild(node);
    UI2_PushCurrent(node);
}

void UI2_EndResize(void)
{
    UI2_PopCurrent();
}
