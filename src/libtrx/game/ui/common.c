#include "game/ui/common.h"

#include "vector.h"

void UI_Init(void)
{
    UI_Events_Init();
}

void UI_Shutdown(void)
{
    UI_Events_Shutdown();
}

void UI_HandleKeyDown(const uint32_t key)
{
    const EVENT event = {
        .name = "key_down",
        .sender = NULL,
        .data = (void *)UI_TranslateInput(key),
    };
    UI_Events_Fire(&event);
}

void UI_HandleKeyUp(const uint32_t key)
{
    const EVENT event = {
        .name = "key_up",
        .sender = NULL,
        .data = (void *)UI_TranslateInput(key),
    };
    UI_Events_Fire(&event);
}

void UI_HandleTextEdit(const char *const text)
{
    const EVENT event = {
        .name = "text_edit",
        .sender = NULL,
        .data = (void *)text,
    };
    UI_Events_Fire(&event);
}
