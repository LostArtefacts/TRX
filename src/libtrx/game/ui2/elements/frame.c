#include "game/ui2/elements/frame.h"

#include "config.h"
#include "game/output.h"
#include "game/text.h"
#include "game/ui2/helpers.h"

typedef struct {
    UI_STYLE ui_style;
    TEXT_STYLE text_style;
    int32_t outline_z;
    int32_t background_z;
} M_DATA;

static void M_Draw(const UI2_NODE *node);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = UI2_MeasureWrapper,
    .layout = UI2_LayoutWrapper,
    .draw = M_Draw,
};

static void M_Draw(const UI2_NODE *node)
{
    const M_DATA *const data = node->data;
    if (data->background_z >= 0) {
        Output_DrawTextBackground(
            data->ui_style, UI2_ScaleX(node->x), UI2_ScaleY(node->y),
            UI2_ScaleX(node->w), UI2_ScaleY(node->h), data->background_z,
            data->text_style);
    }
    if (data->outline_z >= 0) {
        Output_DrawTextOutline(
            data->ui_style, UI2_ScaleX(node->x), UI2_ScaleY(node->y),
            UI2_ScaleX(node->w), UI2_ScaleY(node->h), data->outline_z,
            data->text_style);
    }
    UI2_DrawWrapper(node);
}

void UI2_BeginFrame(UI2_FRAME_STYLE style)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(M_DATA));
    M_DATA *const data = node->data;

    data->ui_style = UI_STYLE_PC;
#if TR_VERSION == 1
    data->ui_style = g_Config.ui.menu_style;
#endif

    switch (style) {
    case UI2_FRAME_DIALOG_BACKGROUND:
        data->outline_z = 160;
        data->background_z = 160;
        data->text_style = TS_BACKGROUND;
        break;
    case UI2_FRAME_DIALOG_HEADING:
        data->outline_z = 80;
        data->background_z = 80;
        data->text_style = TS_HEADING;
        break;
    case UI2_FRAME_SELECTED_OPTION:
        data->outline_z = 80;
        data->background_z = 80;
        data->text_style = TS_REQUESTED;
        break;
    case UI2_FRAME_OUTLINE_ONLY:
        data->outline_z = 80;
        data->background_z = -1;
        data->text_style = TS_REQUESTED;
        break;
    }

    UI2_AddChild(node);
    UI2_PushCurrent(node);
}

void UI2_EndFrame(void)
{
    UI2_PopCurrent();
}
