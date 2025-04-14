#include "game/screen.h"

#include <libtrx/config.h>
#include <libtrx/game/ui/common.h>

#include <SDL2/SDL.h>

int32_t UI_GetCanvasWidth(void)
{
    return Screen_GetResWidthDownscaled(RSR_TEXT);
}

int32_t UI_GetCanvasHeight(void)
{
    return Screen_GetResHeightDownscaled(RSR_TEXT);
}

int32_t UI2_GetCanvasWidth(void)
{
    return Screen_GetResHeightDownscaled(RSR_GENERIC) * 16 / 9;
}

int32_t UI2_GetCanvasHeight(void)
{
    return Screen_GetResHeightDownscaled(RSR_GENERIC);
}

float UI2_ScaleX(const float x)
{
    return Screen_GetRenderScale(x * 0x10000, RSR_GENERIC) / (float)0x10000;
}

float UI2_ScaleY(const float y)
{
    return Screen_GetRenderScale(y * 0x10000, RSR_GENERIC) / (float)0x10000;
}

UI_INPUT UI_TranslateInput(uint32_t system_keycode)
{
    // clang-format off
    switch (system_keycode) {
    case SDLK_UP:        return UI_KEY_UP;
    case SDLK_DOWN:      return UI_KEY_DOWN;
    case SDLK_LEFT:      return UI_KEY_LEFT;
    case SDLK_RIGHT:     return UI_KEY_RIGHT;
    case SDLK_HOME:      return UI_KEY_HOME;
    case SDLK_END:       return UI_KEY_END;
    case SDLK_BACKSPACE: return UI_KEY_BACK;
    case SDLK_RETURN:    return UI_KEY_RETURN;
    case SDLK_ESCAPE:    return UI_KEY_ESCAPE;
    }
    // clang-format on
    return -1;
}
