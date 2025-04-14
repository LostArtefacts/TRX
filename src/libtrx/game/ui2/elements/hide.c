#include "game/ui2/elements/hide.h"

#include "game/ui2/helpers.h"

static void M_Draw(const UI2_NODE *node);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = UI2_MeasureWrapper,
    .layout = UI2_LayoutWrapper,
    .draw = M_Draw,
};

static void M_Draw(const UI2_NODE *const node)
{
    const bool draw_children = *(bool *)node->data;
    if (draw_children) {
        UI2_DrawWrapper(node);
    }
}

void UI2_BeginHide(const bool hide_children)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(bool));
    *(bool *)node->data = !hide_children;
    UI2_AddChild(node);
    UI2_PushCurrent(node);
}

void UI2_EndHide(void)
{
    UI2_PopCurrent();
}
