#include <trx/game/ui/elements/spacer.h>

#include <trx/config.h>
#include <trx/game/ui/helpers.h>
#include <trx/game/ui/scaler.h>

static void M_Measure(UI_NODE *const node)
{
    // already done in the constructor
}

void UI_Spacer(const float w, const float h)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = UI_LayoutBasic,
            .draw = UI_DrawWrapper,
        },
        0);
    node->measure_w = w * UI_Scaler_GetTextScale();
    node->measure_h = h * UI_Scaler_GetTextScale();
    UI_AddChild(node);
}
