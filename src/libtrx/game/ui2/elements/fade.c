#include "game/ui2/elements/fade.h"

#include "game/output.h"
#include "game/ui2/helpers.h"

typedef struct {
    FADER *fader;
    bool is_on_top;
} M_DATA;

static void M_Draw(const UI2_NODE *node);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = UI2_MeasureWrapper,
    .layout = UI2_LayoutWrapper,
    .draw = M_Draw,
};

static void M_Draw(const UI2_NODE *const node)
{
    const M_DATA *const data = node->data;
    if (data->is_on_top) {
        UI2_DrawWrapper(node);
        Output_DrawPolyList(); // flush geometry
        Fader_Draw(data->fader);
    } else {
        Fader_Draw(data->fader);
        UI2_DrawWrapper(node);
    }
}

void UI2_BeginFade(FADER *const fader, bool is_on_top)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(M_DATA *));
    M_DATA *const data = node->data;
    data->fader = fader;
    data->is_on_top = is_on_top;
    UI2_AddChild(node);
    UI2_PushCurrent(node);
}

void UI2_EndFade(void)
{
    UI2_PopCurrent();
}
