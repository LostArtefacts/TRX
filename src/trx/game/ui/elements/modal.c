#include <trx/game/ui/elements/modal.h>

#include <trx/game/ui/draw.h>
#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/elements/pad.h>
#include <trx/game/ui/helpers.h>

static void M_Measure(UI_NODE *const node)
{
    node->measure_w = UI_GetCanvasWidth();
    node->measure_h = UI_GetCanvasHeight();
}

static void M_Draw(const UI_NODE *const node)
{
    UI_DrawWrapper(node);
}

void UI_BeginModal(const float x, const float y)
{
    UI_BeginModalEx(x, y, 0.0f);
}

void UI_BeginModalEx(const float x, const float y, const float inset_v)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = UI_LayoutWrapper,
            .draw = M_Draw,
        },
        0);
    UI_AddChild(node);
    UI_PushCurrent(node);
    UI_BeginPad(0.0f, inset_v);
    UI_BeginAnchor(x, y);
}

void UI_EndModal(void)
{
    UI_EndAnchor();
    UI_EndPad();
    UI_PopCurrent();
}
