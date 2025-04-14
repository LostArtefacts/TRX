#include "game/ui2/elements/anchor.h"

#include "game/text.h"
#include "game/ui2/helpers.h"

typedef struct {
    float x;
    float y;
} M_DATA;

static void M_Measure(UI2_NODE *node);
static void M_Layout(UI2_NODE *node, float x, float y, float w, float h);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = UI2_MeasureWrapper,
    .layout = M_Layout,
    .draw = UI2_DrawWrapper,
};

static void M_Measure(UI2_NODE *const node)
{
    node->measure_w = UI2_GetCanvasWidth();
    node->measure_h = UI2_GetCanvasHeight() - TEXT_HEIGHT_FIXED;
}

static void M_Layout(
    UI2_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI2_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;
    UI2_NODE *child = node->first_child;
    while (child != nullptr) {
        const float cw = child->measure_w;
        const float ch = child->measure_h;
        const float cx = x + (w - cw) * data->x;
        const float cy = y + (h - ch) * data->y;
        child->ops->layout(child, cx, cy, cw, ch);
        child = child->next_sibling;
    }
}

void UI2_BeginAnchor(const float x, const float y)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->x = x;
    data->y = y;
    UI2_AddChild(node);
    UI2_PushCurrent(node);
}

void UI2_EndAnchor(void)
{
    UI2_PopCurrent();
}
