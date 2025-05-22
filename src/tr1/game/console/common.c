#include "game/console/common.h"

#include "game/output.h"
#include "game/screen.h"
#include "game/viewport.h"

#include <libtrx/game/ui.h>

void Console_DrawBackdrop(void)
{
    int32_t sx = 0;
    int32_t sw = Viewport_GetWidth();
    int32_t sh = Screen_GetRenderScale(
        // not entirely accurate, but good enough
        UI_TEXT_HEIGHT * 1.0 + 7 * UI_TEXT_HEIGHT * 0.8, RSR_TEXT);
    int32_t sy = Viewport_GetHeight() - sh;

    RGBA_8888 top = { 0, 0, 0, 0 };
    RGBA_8888 bottom = { 0, 0, 0, 196 };

    Output_DrawPolyList();
    Output_DrawScreenGradientQuad(sx, sy, sw, sh, top, top, bottom, bottom);
}
