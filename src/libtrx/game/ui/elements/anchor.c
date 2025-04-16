#include "game/ui/elements/anchor.h"

#include "game/text.h"
#include "game/ui/helpers.h"

typedef struct {
    float x;
    float y;
} M_DATA;

static void M_Measure(UI_NODE *node);
static void M_Layout(UI_NODE *node, float x, float y, float w, float h);

static const UI_WIDGET_OPS m_Ops = {
    .measure = UI_MeasureWrapper,
    .layout = M_Layout,
    .draw = UI_DrawWrapper,
};

static void M_Measure(UI_NODE *const node)
{
    node->measure_w = UI_GetCanvasWidth();
    node->measure_h = UI_GetCanvasHeight() - TEXT_HEIGHT_FIXED;
}

static void M_Layout(
    UI_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;
    UI_NODE *child = node->first_child;
    while (child != nullptr) {
        const float cw = child->measure_w;
        const float ch = child->measure_h;
        const float cx = x + (w - cw) * data->x;
        const float cy = y + (h - ch) * data->y;
        child->ops->layout(child, cx, cy, cw, ch);
        child = child->next_sibling;
    }
}

void UI_BeginAnchor(const float x, const float y)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->x = x;
    data->y = y;
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndAnchor(void)
{
    UI_PopCurrent();
}
