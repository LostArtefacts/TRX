#include "game/ui/elements/modal.h"

#include "game/text.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/helpers.h"

static void M_Measure(UI_NODE *node);

static const UI_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = UI_LayoutWrapper,
    .draw = UI_DrawWrapper,
};

static void M_Measure(UI_NODE *const node)
{
    node->measure_w = UI_GetCanvasWidth();
    node->measure_h = UI_GetCanvasHeight();
}

void UI_BeginModal(const float x, const float y)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, 0);
    UI_AddChild(node);
    UI_PushCurrent(node);
    UI_BeginAnchor(x, y);
}

void UI_EndModal(void)
{
    UI_EndAnchor();
    UI_PopCurrent();
}
