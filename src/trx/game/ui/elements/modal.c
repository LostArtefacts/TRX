#include <trx/game/ui/elements/modal.h>

#include <trx/core/utils.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/elements/pad.h>
#include <trx/game/ui/helpers.h>

typedef struct {
    bool whole_screen;
} M_DATA;

static void M_Measure(UI_NODE *const node)
{
    node->measure_w = UI_GetCanvasWidth();
    node->measure_h = UI_GetCanvasHeight();
}

static void M_Layout(
    UI_NODE *const node, const float x, const float y, const float w,
    const float h)
{
    UI_LayoutBasic(node, x, y, w, h);
    const M_DATA *const data = node->data;
    const float child_x = data->whole_screen ? x : x + UI_SCREEN_MARGIN;
    const float child_y = data->whole_screen ? y : y + UI_GetSafeCanvasTop();
    const float child_w = data->whole_screen ? w : UI_GetSafeCanvasWidth();
    const float child_h = data->whole_screen ? h : UI_GetSafeCanvasHeight();
    UI_NODE *child = node->first_child;
    while (child != nullptr) {
        if (child->ops.layout != nullptr) {
            child->ops.layout(child, child_x, child_y, child_w, child_h);
        }
        child = child->next_sibling;
    }
}

static void M_Draw(const UI_NODE *const node)
{
    UI_DrawWrapper(node);
}

static void M_Begin(const float x, const float y, const bool whole_screen)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = M_Layout,
            .draw = M_Draw,
        },
        sizeof(M_DATA));
    M_DATA *const data = node->data;
    data->whole_screen = whole_screen;
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_BeginModal(const float x, const float y)
{
    M_Begin(x, y, false);
    UI_BeginPad(0.0f, 0.0f);
    UI_BeginAnchor(x, y);
}

void UI_BeginScreenModal(const float x, const float y)
{
    M_Begin(x, y, true);
    UI_BeginPad(0.0f, 0.0f);
    UI_BeginAnchor(x, y);
}

void UI_EndModal(void)
{
    UI_EndAnchor();
    UI_EndPad();
    UI_PopCurrent();
}
