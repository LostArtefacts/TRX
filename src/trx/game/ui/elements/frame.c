#include <trx/game/ui/elements/frame.h>

#include <trx/config.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/helpers.h>

typedef struct {
    UI_STYLE ui_style;
    TEXT_STYLE text_style;
    int32_t outline_z;
    int32_t background_z;
} M_DATA;

static void M_Draw(const UI_NODE *node)
{
    const M_DATA *const data = node->data;
    if (data->background_z >= 0) {
        UI_ScheduleDrawTextBackground(
            data->ui_style, UI_ScaleX(node->x), UI_ScaleY(node->y),
            data->background_z, UI_ScaleX(node->w), UI_ScaleY(node->h),
            data->text_style);
    }
    if (data->outline_z >= 0) {
        UI_ScheduleDrawTextOutline(
            data->ui_style, UI_ScaleX(node->x), UI_ScaleY(node->y),
            data->outline_z, UI_ScaleX(node->w), UI_ScaleY(node->h),
            data->text_style);
    }
    UI_DrawWrapper(node);
}

TEXT_STYLE UI_Frame_GetTextStyle(const UI_FRAME_STYLE style)
{
    switch (style) {
    case UI_FRAME_DIALOG_BACKGROUND:
        return TS_BACKGROUND;
    case UI_FRAME_DIALOG_BACKGROUND_HEAVY:
        return TS_BACKGROUND_HEAVY;
    case UI_FRAME_DIALOG_HEADING:
        return TS_HEADING;
    case UI_FRAME_SELECTED_OPTION:
    case UI_FRAME_OUTLINE_ONLY:
        return TS_REQUESTED;
    }
    return TS_BACKGROUND;
}

bool UI_Frame_HasBackground(const UI_FRAME_STYLE style)
{
    return style != UI_FRAME_OUTLINE_ONLY;
}

void UI_BeginFrame(UI_FRAME_STYLE style)
{
    UI_NODE *const node = UI_AllocNode(
        &(UI_WIDGET_OPS) {
            .measure = UI_MeasureWrapper,
            .layout = UI_LayoutWrapper,
            .draw = M_Draw,
        },
        sizeof(M_DATA));
    M_DATA *const data = node->data;

    data->ui_style = g_Config.ui.menu_style;
    data->text_style = UI_Frame_GetTextStyle(style);

    const int32_t z = style == UI_FRAME_DIALOG_BACKGROUND
            || style == UI_FRAME_DIALOG_BACKGROUND_HEAVY
        ? 160
        : 80;
    data->outline_z = z;
    data->background_z = UI_Frame_HasBackground(style) ? z : -1;

    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndFrame(void)
{
    UI_PopCurrent();
}
