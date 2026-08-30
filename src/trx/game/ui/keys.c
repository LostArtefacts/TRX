// Translation of SDL keyboard events into the UI's own key roles. This is the
// only part of the UI that talks to SDL, and it sits apart from the scene
// module so that measuring and laying out a scene needs no window system.

#include <trx/game/ui/keys.h>

#include <trx/game/ui/events.h>

#include <SDL2/SDL.h>

static UI_INPUT M_TranslateInput(const uint32_t system_keycode)
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
    case SDLK_KP_ENTER:  return UI_KEY_RETURN;
    case SDLK_ESCAPE:    return UI_KEY_ESCAPE;
    case SDLK_TAB:
        return (SDL_GetModState() & KMOD_SHIFT) != 0 ? UI_KEY_SHIFT_TAB
                                                     : UI_KEY_TAB;
    }
    // clang-format on
    return -1;
}

void UI_HandleKeyDown(const uint32_t key)
{
    UI_FireEvent((EVENT) {
        .name = "key_down",
        .sender = nullptr,
        .data = (void *)M_TranslateInput(key),
    });
}

void UI_HandleKeyUp(const uint32_t key)
{
    UI_FireEvent((EVENT) {
        .name = "key_up",
        .sender = nullptr,
        .data = (void *)M_TranslateInput(key),
    });
}

void UI_HandleTextEdit(const char *const text)
{
    UI_FireEvent((EVENT) {
        .name = "text_edit", .sender = nullptr, .data = (void *)text });
}

void UI_HandlePaste(void)
{
    if (!SDL_HasClipboardText()) {
        return;
    }

    char *const text = SDL_GetClipboardText();
    if (text == nullptr) {
        return;
    }

    UI_HandleTextEdit(text);
    SDL_free(text);
}

RESULT UI_SetClipboardText(const char *const text)
{
    if (SDL_SetClipboardText(text) != 0) {
        return FAIL("failed to set the clipboard: %s", SDL_GetError());
    }
    return OK;
}
