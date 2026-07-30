#include <trx/game/ui/elements/pad.h>

#include <trx/config.h>
#include <trx/game/ui/helpers.h>
#include <trx/game/ui/scaler.h>

typedef struct {
    float t;
    float r;
    float d;
    float l;
} M_DATA;

static void M_Measure(UI_NODE *const node)
{
    UI_MeasureWrapper(node);
    const M_DATA *const data = node->data;
    node->measure_w += data->l + data->r;
    node->measure_h += data->t + data->d;
}

static void M_Layout(
    UI_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;
    UI_NODE *child = node->first_child;
    while (child != nullptr) {
        child->ops.layout(
            child, x + data->l, y + data->t, w - data->l - data->r,
            h - data->t - data->d);
        child = child->next_sibling;
    }
}

void UI_BeginPad(const float x, const float y)
{
    UI_BeginPadEx(x, x, y, y);
}

void UI_BeginPadEx(const float l, const float r, const float t, const float d)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = M_Layout,
            .draw = UI_DrawWrapper,
        },
        sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->t = UI_Pad_GetSize(t);
    data->r = UI_Pad_GetSize(r);
    data->d = UI_Pad_GetSize(d);
    data->l = UI_Pad_GetSize(l);
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndPad(void)
{
    UI_PopCurrent();
}

float UI_Pad_GetSize(const float size)
{
    return size * UI_Scaler_GetTextScale();
}
