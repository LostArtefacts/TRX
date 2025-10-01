#include "game/ui/elements/fade.h"

#include "game/output.h"
#include "game/ui/draw.h"
#include "game/ui/helpers.h"

typedef struct {
    FADER *fader;
    bool is_on_top;
} M_DATA;

static void M_Draw(const UI_NODE *const node)
{
    const M_DATA *const data = node->data;
    if (data->is_on_top) {
        UI_DrawWrapper(node);
        UI_ScheduleFaderDraw(data->fader);
    } else {
        UI_ScheduleFaderDraw(data->fader);
        UI_DrawWrapper(node);
    }
}

void UI_BeginFade(FADER *const fader, bool is_on_top)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = UI_MeasureWrapper,
            .layout = UI_LayoutWrapper,
            .draw = M_Draw,
        },
        sizeof(M_DATA));
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
