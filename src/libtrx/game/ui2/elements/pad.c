#include "game/ui2/elements/pad.h"

#include "config.h"
#include "game/ui2/helpers.h"

typedef struct {
    float t;
    float r;
    float d;
    float l;
} M_DATA;

static void M_Measure(UI2_NODE *node);
static void M_Layout(UI2_NODE *node, float x, float y, float w, float h);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = M_Layout,
    .draw = UI2_DrawWrapper,
};

static void M_Measure(UI2_NODE *const node)
{
    UI2_MeasureWrapper(node);
    const M_DATA *const data = node->data;
    node->measure_w += data->l + data->r;
    node->measure_h += data->t + data->d;
}

static void M_Layout(
    UI2_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI2_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;
    const float px = data->l;
    const float py = data->t;
    UI2_NODE *child = node->first_child;
    while (child != nullptr) {
        child->ops->layout(child, x + px, y + py, w - px * 2.0f, h - py * 2.0f);
        child = child->next_sibling;
    }
}

void UI2_BeginPad(const float x, const float y)
{
    UI2_BeginPadEx(x, x, y, y);
}

void UI2_BeginPadEx(const float l, const float r, const float t, const float d)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->t = t * g_Config.ui.text_scale;
    data->r = r * g_Config.ui.text_scale;
    data->d = d * g_Config.ui.text_scale;
    data->l = l * g_Config.ui.text_scale;
    UI2_AddChild(node);
    UI2_PushCurrent(node);
}

void UI2_EndPad(void)
{
    UI2_PopCurrent();
}
