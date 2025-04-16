#include "game/ui/elements/fade.h"

#include "game/output.h"
#include "game/ui/helpers.h"

typedef struct {
    FADER *fader;
    bool is_on_top;
} M_DATA;

static void M_Draw(const UI_NODE *node);

static const UI_WIDGET_OPS m_Ops = {
    .measure = UI_MeasureWrapper,
    .layout = UI_LayoutWrapper,
    .draw = M_Draw,
};

static void M_Draw(const UI_NODE *const node)
{
    const M_DATA *const data = node->data;
    if (data->is_on_top) {
        UI_DrawWrapper(node);
        Output_DrawPolyList(); // flush geometry
        Fader_Draw(data->fader);
    } else {
        Fader_Draw(data->fader);
        UI_DrawWrapper(node);
    }
}

void UI_BeginFade(FADER *const fader, bool is_on_top)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(M_DATA *));
    M_DATA *const data = node->data;
    data->fader = fader;
    data->is_on_top = is_on_top;
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndFade(void)
{
    UI_PopCurrent();
}
