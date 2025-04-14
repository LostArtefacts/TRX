#include "game/scaler.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/ui/common.h>

#include <SDL2/SDL.h>

int32_t UI_GetCanvasWidth(void)
{
    return Scaler_CalcInverse(g_PhdWinWidth, SCALER_TARGET_TEXT);
}

int32_t UI_GetCanvasHeight(void)
{
    return Scaler_CalcInverse(g_PhdWinHeight, SCALER_TARGET_TEXT);
}

int32_t UI2_GetCanvasWidth(void)
{
    return Scaler_CalcInverse(g_PhdWinWidth, SCALER_TARGET_GENERIC);
}

int32_t UI2_GetCanvasHeight(void)
{
    return Scaler_CalcInverse(g_PhdWinHeight, SCALER_TARGET_GENERIC);
}

float UI2_ScaleX(const float x)
{
    return Scaler_Calc(x, SCALER_TARGET_GENERIC);
}

float UI2_ScaleY(const float y)
{
    return Scaler_Calc(y, SCALER_TARGET_GENERIC);
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
