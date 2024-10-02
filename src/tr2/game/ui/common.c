#include <libtrx/game/ui/common.h>

#include <windows.h>

int32_t UI_GetCanvasWidth(void)
{
    return 640;
}

int32_t UI_GetCanvasHeight(void)
{
    return 480;
}

UI_INPUT UI_TranslateInput(uint32_t system_keycode)
{
    // clang-format off
    switch (system_keycode) {
    case VK_LEFT:   return UI_KEY_LEFT;
    case VK_RIGHT:  return UI_KEY_RIGHT;
    case VK_HOME:   return UI_KEY_HOME;
    case VK_END:    return UI_KEY_END;
    case VK_BACK:   return UI_KEY_BACK;
    case VK_RETURN: return UI_KEY_RETURN;
    case VK_ESCAPE: return UI_KEY_ESCAPE;
    }
    // clang-format on
    return -1;
}
