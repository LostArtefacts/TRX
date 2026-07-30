#include <trx/game/ui/elements/resize.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/ui/helpers.h>
#include <trx/game/ui/scaler.h>

typedef struct {
    float w;
    float h;
    float align_h;
    float align_v;
} M_DATA;

static void M_Measure(UI_NODE *const node)
{
    UI_MeasureWrapper(node);
    const M_DATA *const data = node->data;
    if (data->w >= 0.0f) {
        node->measure_w = data->w;
    }
    if (data->h >= 0.0f) {
        node->measure_h = data->h;
    }
}

static void M_Layout(
    UI_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;
    UI_NODE *child = node->first_child;
    while (child != nullptr) {
        float child_w = w;
        float child_h = h;
        if (data->w >= 0.0f && data->align_h != 0.0f) {
            child_w = MAX(child->measure_w, w);
        }
        if (data->h >= 0.0f && data->align_v != 0.0f) {
            child_h = MAX(child->measure_h, h);
        }

        const float child_x = x + (w - child_w) * data->align_h;
        const float child_y = y + (h - child_h) * data->align_v;
        if (child->ops.layout != nullptr) {
            child->ops.layout(child, child_x, child_y, child_w, child_h);
        }
        child = child->next_sibling;
    }
}

static void M_Draw(const UI_NODE *const node)
{
    if (node->measure_w <= 0.0f || node->measure_h <= 0.0f) {
        return;
    }
    UI_DrawWrapper(node);
}

void UI_BeginResizeEx(const UI_RESIZE_SETTINGS settings)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = M_Layout,
            .draw = M_Draw,
        },
        sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->w = settings.w * UI_Scaler_GetTextScale();
    data->h = settings.h * UI_Scaler_GetTextScale();
    data->align_h = settings.align_h;
    data->align_v = settings.align_v;
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_BeginResize(const float w, const float h)
{
    UI_BeginResizeEx((UI_RESIZE_SETTINGS) {
        .w = w,
        .h = h,
    });
}

void UI_EndResize(void)
{
    UI_PopCurrent();
}
