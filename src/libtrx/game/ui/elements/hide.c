#include "game/ui/elements/hide.h"

#include "game/ui/helpers.h"

static void M_Draw(const UI_NODE *node);

static const UI_WIDGET_OPS m_Ops = {
    .measure = UI_MeasureWrapper,
    .layout = UI_LayoutWrapper,
    .draw = M_Draw,
};

static void M_Draw(const UI_NODE *const node)
{
    const bool draw_children = *(bool *)node->data;
    if (draw_children) {
        UI_DrawWrapper(node);
    }
}

void UI_BeginHide(const bool hide_children)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(bool));
    *(bool *)node->data = !hide_children;
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndHide(void)
{
    UI_PopCurrent();
}
