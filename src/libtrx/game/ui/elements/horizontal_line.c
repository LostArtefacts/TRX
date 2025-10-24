#include "game/ui/elements/horizontal_line.h"

#include "config.h"
#include "game/output.h"
#include "game/ui/draw.h"
#include "game/ui/helpers.h"

static void M_Draw(const UI_NODE *node)
{
    // UI_DrawWrapper(node);
    UI_ScheduleDrawTextOutline(
        g_Config.ui.menu_style, UI_ScaleX(node->x), UI_ScaleY(node->y), 0,
        UI_ScaleX(node->w), UI_ScaleX(node->h), TS_LINE);
}

static void M_Measure(UI_NODE *const node)
{
    UI_MeasureWrapper(node);
    node->measure_h = 2 * g_Config.ui.text_scale;
}

void UI_HorizontalLine(void)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = M_Measure,
            .layout = UI_LayoutBasic,
            .draw = M_Draw,
        },
        0);
    UI_AddChild(node);
}
