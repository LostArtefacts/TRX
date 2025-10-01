#include "game/ui/elements/fixed.h"

#include "game/ui/helpers.h"
#include "utils.h"

typedef struct {
    float x;
    float y;
} M_DATA;

static void M_Measure(UI_NODE *const node)
{
    node->measure_w = 0.0f;
    node->measure_h = 0.0f;
}

static void M_Layout(UI_NODE *const node, float x, float y, float w, float h)
{
    const M_DATA *const data = node->data;
    w = 0;
    h = 0;
    const UI_NODE *child = node->first_child;
    while (child != nullptr) {
        w = MAX(w, child->measure_w);
        h = MAX(h, child->measure_h);
        child = child->next_sibling;
    }
    x -= w * data->x;
    y -= h * data->y;
    UI_LayoutWrapper(node, x, y, w, h);
}

static void M_Draw(const UI_NODE *const node)
{
    const UI_NODE *child = node->first_child;
    while (child != nullptr) {
        if (child->ops.draw != nullptr) {
            child->ops.draw(child);
        }
        child = child->next_sibling;
    }
}

void UI_BeginFixed(const float x, const float y)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = M_Layout,
            .draw = M_Draw,
        },
        sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->x = x;
    data->y = y;
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndFixed(void)
{
    UI_PopCurrent();
}
