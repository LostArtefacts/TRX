#include <trx/game/ui/frame.h>

TEXT_STYLE UI_Frame_GetTextStyle(const UI_FRAME_STYLE style)
{
    switch (style) {
    case UI_FRAME_DIALOG_BACKGROUND:
        return TS_BACKGROUND;
    case UI_FRAME_DIALOG_BACKGROUND_HEAVY:
        return TS_BACKGROUND_HEAVY;
    case UI_FRAME_DIALOG_HEADING:
    case UI_FRAME_HEADING_OUTLINE:
        return TS_HEADING;
    case UI_FRAME_SELECTED_OPTION:
    case UI_FRAME_OUTLINE_ONLY:
        return TS_REQUESTED;
    }
    return TS_BACKGROUND;
}

bool UI_Frame_HasBackground(const UI_FRAME_STYLE style)
{
    return style != UI_FRAME_OUTLINE_ONLY && style != UI_FRAME_HEADING_OUTLINE;
}
