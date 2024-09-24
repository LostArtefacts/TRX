#include <libtrx/game/ui/common.h>

#include <SDL2/SDL.h>

UI_INPUT UI_TranslateInput(uint32_t system_keycode)
{
    // clang-format off
    switch (system_keycode) {
    case SDLK_LEFT:   return UI_KEY_LEFT;
    case SDLK_RIGHT:  return UI_KEY_RIGHT;
    case SDLK_HOME:   return UI_KEY_HOME;
    case SDLK_END:    return UI_KEY_END;
    case SDLK_BACKSPACE:   return UI_KEY_BACK;
    case SDLK_RETURN: return UI_KEY_RETURN;
    case SDLK_ESCAPE: return UI_KEY_ESCAPE;
    }
    // clang-format on
    return -1;
}
