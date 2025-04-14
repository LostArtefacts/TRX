#include "game/ui2/elements/spacer.h"

#include "config.h"
#include "game/ui2/helpers.h"

static void M_Measure(UI2_NODE *node);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = M_Measure,
    .layout = UI2_LayoutBasic,
    .draw = UI2_DrawWrapper,
};

static void M_Measure(UI2_NODE *const node)
{
    // already done in the constructor
}

void UI2_Spacer(const float w, const float h)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, 0);
    node->measure_w = w * g_Config.ui.text_scale;
    node->measure_h = h * g_Config.ui.text_scale;
    UI2_AddChild(node);
}
