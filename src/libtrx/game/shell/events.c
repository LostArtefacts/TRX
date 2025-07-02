#include "config.h"
#include "debug.h"
#include "game/console/common.h"
#include "game/fmv.h"
#include "game/shell.h"
#include "game/ui.h"

// If true, next SDL_TEXT* event should be zeroed out.
static bool m_ConsoleJustOpened = false;

static void M_HandleQuit(void);
static void M_HandleKeyDown(const SDL_Event *event);
static void M_HandleKeyUp(const SDL_Event *event);

static void M_HandleQuit(void)
{
    Shell_ScheduleExit();
}

static void M_HandleKeyDown(const SDL_Event *const event)
{
    // NOTE: Opening the console normally would get handled by Input_Update,
    // but by the time Input_Update gets ran, we may already have lost some
    // keypresses if the player types really fast, so we need to react sooner.
    if (!FMV_IsPlaying() && g_Config.gameplay.enable_console
        && !Console_IsOpened() && !Input_IsInListenMode()
        && Input_IsPressed(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_ENTER_CONSOLE)) {
        Console_Open();
        // Zero out the next text event so the console-open glyph never
        // shows up.
        m_ConsoleJustOpened = true;
    } else {
        UI_HandleKeyDown(event->key.keysym.sym);
    }
}

static void M_HandleKeyUp(const SDL_Event *const event)
{
    // NOTE: needs special handling on Windows -
    // SDL_SCANCODE_PRINTSCREEN is not sufficient to react to this.
    if (event->key.keysym.sym == SDLK_PRINTSCREEN) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }
}

bool Shell_ProcessCommonEvent(const SDL_Event *const event)
{
    switch (event->type) {
    case SDL_QUIT:
        M_HandleQuit();
        return true;

    case SDL_KEYDOWN:
        M_HandleKeyDown(event);
        return true;

    case SDL_KEYUP:
        M_HandleKeyUp(event);
        return true;

    case SDL_TEXTEDITING:
        if (m_ConsoleJustOpened) {
            m_ConsoleJustOpened = false;
        } else {
            UI_HandleTextEdit(event->text.text);
        }
        return true;

    case SDL_TEXTINPUT:
        if (m_ConsoleJustOpened) {
            m_ConsoleJustOpened = false;
        } else {
            UI_HandleTextEdit(event->text.text);
        }
        return true;

    case SDL_CONTROLLERDEVICEADDED:
    case SDL_JOYDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_JOYDEVICEREMOVED:
        Input_Discover();
        return true;
    }

    return false;
}
