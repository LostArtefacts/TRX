#include <trx/av/audio.h>
#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/console/common.h>
#include <trx/game/input/common.h>
#include <trx/game/input/raw.h>
#include <trx/game/input/sdl.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/guard.h>
#include <trx/game/replay/test_recorder.h>
#include <trx/game/replay/test_replay.h>
#include <trx/game/screenshot.h>
#include <trx/game/shell.h>
#include <trx/game/ui.h>
#include <trx/game/ui/keys.h>

// If true, next SDL_TEXT* event should be zeroed out.
static bool m_ConsoleJustOpened = false;

// Whether the game had the devices when the pairs were last balanced.
static bool m_InputWasReserved = false;

static void M_HandleQuit(void)
{
    Shell_ScheduleExit();
}

static void M_FireRawInput(const LUA_EVENT_TYPE type, const char *const name)
{
    if (name == nullptr) {
        return;
    }
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_STRING, .value.str = name },
    };
    LUA_FireEventEx(type, args, 1);
}

// Sends a key or a button to scripts, unless the game holds the devices.
static void M_FireLuaInputEvent(
    const LUA_EVENT_TYPE type, const char *const name)
{
    if (InputRaw_IsReserved()) {
        return;
    }
    M_FireRawInput(type, name);
}

static void M_FireDownAsUp(const INPUT_RAW_INPUT input, void *const user_data)
{
    M_FireRawInput(
        input.is_button ? LUA_EVENT_BUTTON_UP : LUA_EVENT_KEY_UP, input.name);
}

static void M_FireDownAsDown(const INPUT_RAW_INPUT input, void *const user_data)
{
    M_FireRawInput(
        input.is_button ? LUA_EVENT_BUTTON_DOWN : LUA_EVENT_KEY_DOWN,
        input.name);
}

// Keeps what a script was told matching what it can read. A script pairs a
// press with a release, and the game taking the keyboard mid-press would leave
// it holding a key that never comes up: releases go out as the game takes the
// devices, and presses as it gives them back with the keys still down.
static void M_BalanceReservedInput(void)
{
    const bool reserved = InputRaw_IsReserved();
    if (reserved == m_InputWasReserved) {
        return;
    }
    m_InputWasReserved = reserved;
    InputRaw_ForEachDown(
        INPUT_RAW_DEVICE_ALL, reserved ? M_FireDownAsUp : M_FireDownAsDown,
        nullptr);
}

// Lets go of what the named devices held, once they stop reporting. A key held
// as the window loses focus comes up nowhere, so a script would read it as held
// until the player pressed it again. The releases go out before the state goes,
// so that a script still sees each press paired.
static void M_DropRawInput(const INPUT_RAW_DEVICES devices)
{
    if (!InputRaw_IsReserved()) {
        InputRaw_ForEachDown(devices, M_FireDownAsUp, nullptr);
    }
    InputRaw_ClearDevices(devices);
}

static void M_HandleKeyDown(const SDL_Event *const event)
{
    // Read before the script answers the key: a script that lets the devices
    // go on this very key still took it, and the engine interface must not act
    // on it as well.
    const bool script_held = InputRaw_IsHeldByScript();
    M_FireLuaInputEvent(
        event->key.repeat != 0 ? LUA_EVENT_KEY_REPEAT : LUA_EVENT_KEY_DOWN,
        InputRaw_EventKeyName(event));

    // NOTE: Opening the console normally would get handled by Input_Update, but
    // by the time Input_Update gets ran, we may already have lost some
    // keypresses if the player types fast, so we need to react sooner.
    if (g_Config.gameplay.enable_console && !Console_IsOpened()
        && !Input_IsInListenMode()
        && Input_IsHeldEx(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_ENTER_CONSOLE)) {
        Console_Open();
        // Zero out the next text event so the console-open glyph never
        // shows up.
        m_ConsoleJustOpened = true;
    } else if (script_held) {
        // A script holding the devices answers for them, so the engine
        // interface must not act on the same key as well.
    } else if (
        event->key.keysym.sym == SDLK_v
        && (event->key.keysym.mod & KMOD_CTRL) != 0) {
        // SDL does not emit a SDL_TEXTINPUT event for Ctrl+V, so paste
        // is handled explicitly.
        UI_HandlePaste();
    } else {
        UI_HandleKeyDown(event->key.keysym.sym);
    }
}

static void M_HandleKeyUp(const SDL_Event *const event)
{
    M_FireLuaInputEvent(LUA_EVENT_KEY_UP, InputRaw_EventKeyName(event));

    // NOTE: needs special handling on Windows -
    // SDL_SCANCODE_PRINTSCREEN is not sufficient to react to this.
    if (event->key.keysym.sym == SDLK_PRINTSCREEN) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }
}

static void M_HandleFocusGained(void)
{
    Shell_SetIsFocused(true);
    if (g_Config.audio.mute_out_of_focus) {
        Audio_Unmute();
    }
}

static void M_HandleFocusLost(void)
{
    M_DropRawInput(INPUT_RAW_DEVICE_ALL);
    Shell_SetIsFocused(false);
    if (g_Config.audio.mute_out_of_focus) {
        Audio_Mute();
    }
}

static void M_HandleWindowShown(void)
{
    LOG_DEBUG("");
}

static void M_HandleWindowRestored(void)
{
    Shell_SyncFromWindow(true);
}

static void M_HandleWindowMinimized(void)
{
    LOG_DEBUG("");
}

static void M_HandleWindowMaximized(void)
{
    Shell_SyncFromWindow(true);
}

static void M_HandleWindowMoved(const int32_t x, const int32_t y)
{
    Shell_SyncFromWindow(false);
}

static void M_HandleWindowResized(int32_t width, int32_t height)
{
    Shell_SyncFromWindow(true);
}

static bool M_ProcessReplayEvent(const SDL_Event *const event)
{
    switch (event->type) {
    case SDL_QUIT:
        M_HandleQuit();
        return true;
    }
    return false;
}

bool Shell_ProcessEvent(const SDL_Event *const event)
{
    Input_ProcessEvent(event);
    InputRaw_ProcessEvent(event);

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

    case SDL_TEXTINPUT:
        if (m_ConsoleJustOpened) {
            m_ConsoleJustOpened = false;
        } else {
            M_FireLuaInputEvent(LUA_EVENT_TEXT_INPUT, event->text.text);
            if (!InputRaw_IsHeldByScript()) {
                UI_HandleTextEdit(event->text.text);
            }
        }
        return true;

    case SDL_CONTROLLERBUTTONDOWN:
        M_FireLuaInputEvent(
            LUA_EVENT_BUTTON_DOWN, InputRaw_EventButtonName(event));
        return true;

    case SDL_CONTROLLERBUTTONUP:
        M_FireLuaInputEvent(
            LUA_EVENT_BUTTON_UP, InputRaw_EventButtonName(event));
        return true;

    case SDL_CONTROLLERDEVICEADDED:
    case SDL_JOYDEVICEADDED:
        Input_Discover();
        return true;

    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_JOYDEVICEREMOVED:
        M_DropRawInput(INPUT_RAW_DEVICE_CONTROLLER);
        Input_Discover();
        return true;

    case SDL_WINDOWEVENT:
        switch (event->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
            M_HandleWindowShown();
            break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
            M_HandleFocusGained();
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            M_HandleFocusLost();
            break;

        case SDL_WINDOWEVENT_RESTORED:
            M_HandleWindowRestored();
            break;

        case SDL_WINDOWEVENT_MINIMIZED:
            M_HandleWindowMinimized();
            break;

        case SDL_WINDOWEVENT_MAXIMIZED:
            M_HandleWindowMaximized();
            break;

        case SDL_WINDOWEVENT_MOVED:
            M_HandleWindowMoved(event->window.data1, event->window.data2);
            break;

        case SDL_WINDOWEVENT_RESIZED:
            M_HandleWindowResized(event->window.data1, event->window.data2);
            break;
        }
        break;
    }

    return false;
}

void Shell_ProcessEvents(void)
{
    LUA_Guard_Heartbeat();
    InputRaw_BeginFrame();
    M_BalanceReservedInput();

    SDL_Event event;
    if (TestReplay_IsOpened()) {
        TestReplay_RunFrame();
        while (SDL_PollEvent(&event) != 0) {
            M_ProcessReplayEvent(&event);
        }
        return;
    }

    if (TestRecorder_IsOpened()) {
        TestRecorder_BeginFrame();
    }
    while (SDL_PollEvent(&event) != 0) {
        TestRecorder_RecordEvent(&event);
        Shell_ProcessEvent(&event);
    }
    if (TestRecorder_IsOpened()) {
        TestRecorder_EndFrame();
    }
}
