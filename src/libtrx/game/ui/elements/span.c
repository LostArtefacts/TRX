#include "game/ui/elements/span.h"

#include "game/ui/helpers.h"
#include "utils.h"

static void M_Measure(UI_NODE *node);
static void M_Layout(UI_NODE *node, float x, float y, float w, float h);

static const UI_WIDGET_OPS m_Ops = {
    .measure = UI_MeasureWrapper,
    .layout = M_Layout,
    .draw = UI_DrawWrapper,
};

static void M_Measure(UI_NODE *const node)
{
    node->measure_w = 0.0f;
    node->measure_h = 0.0f;
    const UI_NODE *child = node->first_child;
    while (child != nullptr) {
        node->measure_w = MAX(node->measure_w, child->measure_w);
        node->measure_h = MAX(node->measure_h, child->measure_h);
        child = child->next_sibling;
    }
}

static void M_Layout(
    UI_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI_LayoutBasic(node, x, y, w, h);
    UI_NODE *child = node->first_child;
    while (child != nullptr) {
        child->ops->layout(child, x, y, node->measure_w, node->measure_h);
        child = child->next_sibling;
    }
}

void UI_BeginSpan(void)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, 0);
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndSpan(void)
{
    UI_PopCurrent();
}
