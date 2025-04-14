#include "game/ui2/elements/modal.h"

#include "game/text.h"
#include "game/ui2/elements/anchor.h"
#include "game/ui2/helpers.h"

static void M_Measure(UI2_NODE *node);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = UI2_LayoutWrapper,
    .draw = UI2_DrawWrapper,
};

static void M_Measure(UI2_NODE *const node)
{
    node->measure_w = UI2_GetCanvasWidth();
    node->measure_h = UI2_GetCanvasHeight() - TEXT_HEIGHT_FIXED;
}

void UI2_BeginModal(const float x, const float y)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, 0);
    UI2_AddChild(node);
    UI2_PushCurrent(node);
    UI2_BeginAnchor(x, y);
}

void UI2_EndModal(void)
{
    UI2_EndAnchor();
    UI2_PopCurrent();
}
