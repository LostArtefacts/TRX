#include "specific/s_shell.h"

#include "game/console/common.h"
#include "game/fmv.h"
#include "game/input.h"
#include "game/music.h"
#include "game/option/option_controls.h"
#include "game/output.h"
#include "game/shell.h"
#include "game/sound.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/gfx/common.h>
#include <libtrx/gfx/context.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_messagebox.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SDL_Window *m_Window = nullptr;

static void M_HandleQuit(void);
static void M_SetWindowPos(int32_t x, int32_t y, bool update);
static void M_SetWindowSize(int32_t width, int32_t height, bool update);
static void M_SetWindowMaximized(bool is_enabled, bool update);
static void M_SetFullscreen(bool is_enabled, bool update);

static void M_HandleQuit(void)
{
    Shell_ScheduleExit();
}

static void M_SetWindowPos(int32_t x, int32_t y, bool update)
{
    if (x <= 0 || y <= 0) {
        return;
    }

    // only save window position if it's in windowed state.
    if (!g_Config.window.is_fullscreen && !g_Config.window.is_maximized) {
        g_Config.window.x = x;
        g_Config.window.y = y;
    }

    if (update) {
        SDL_SetWindowPosition(m_Window, x, y);
    }
}

static void M_SetWindowSize(int32_t width, int32_t height, bool update)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    // only save window size if it's in windowed state.
    if (!g_Config.window.is_fullscreen && !g_Config.window.is_maximized) {
        g_Config.window.width = width;
        g_Config.window.height = height;
    }

    Output_SetWindowSize(width, height);

    if (update) {
        SDL_SetWindowSize(m_Window, width, height);
    }
}

static void M_SetWindowMaximized(bool is_enabled, bool update)
{
    g_Config.window.is_maximized = is_enabled;

    if (update && is_enabled) {
        SDL_MaximizeWindow(m_Window);
    }
}

static void M_SetFullscreen(bool is_enabled, bool update)
{
    g_Config.window.is_fullscreen = is_enabled;

    if (update) {
        SDL_SetWindowFullscreen(
            m_Window, is_enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_ShowCursor(is_enabled ? SDL_DISABLE : SDL_ENABLE);
    }
}

void S_Shell_ToggleFullscreen(void)
{
    M_SetFullscreen(!g_Config.window.is_fullscreen, true);

    // save the updated config, but ensure it was loaded first
    if (g_Config.loaded) {
        Config_Write();
    }
}

void S_Shell_HandleWindowResize(void)
{
    int x;
    int y;
    int width;
    int height;
    bool is_maximized;

    Uint32 window_flags = SDL_GetWindowFlags(m_Window);
    is_maximized = window_flags & SDL_WINDOW_MAXIMIZED;
    SDL_GetWindowSize(m_Window, &width, &height);
    SDL_GetWindowPosition(m_Window, &x, &y);
    LOG_INFO("%dx%d+%d,%d (maximized: %d)", width, height, x, y, is_maximized);

    M_SetWindowMaximized(is_maximized, false);
    M_SetWindowPos(x, y, false);
    M_SetWindowSize(width, height, false);

    UI_Events_Fire(&(EVENT) { .name = "canvas_resize" });

    // save the updated config, but ensure it was loaded first
    if (g_Config.loaded) {
        Config_Write();
    }
}

void S_Shell_Init(void)
{
    M_SetFullscreen(g_Config.window.is_fullscreen, true);
    M_SetWindowPos(g_Config.window.x, g_Config.window.y, true);
    M_SetWindowSize(g_Config.window.width, g_Config.window.height, true);
    M_SetWindowMaximized(g_Config.window.is_maximized, true);
    SDL_ShowWindow(m_Window);
}

void Shell_ProcessEvents(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            M_HandleQuit();
            break;

        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                FMV_Unmute();
                Music_Unmute();
                Sound_SetMasterVolume(g_Config.audio.sound_volume);
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                FMV_Mute();
                Music_Mute();
                Sound_SetMasterVolume(0);
                break;

            case SDL_WINDOWEVENT_MOVED:
            case SDL_WINDOWEVENT_RESIZED:
                S_Shell_HandleWindowResize();
                break;
            }
            break;

        case SDL_KEYDOWN: {
            // NOTE: This normally would get handled by Input_Update,
            // but by the time Input_Update gets ran, we may already have lost
            // some keypresses if the player types really fast, so we need to
            // react sooner.
            if (!FMV_IsPlaying() && g_Config.gameplay.enable_console
                && !Console_IsOpened() && !Option_Controls_IsKeyChangeMode()
                && Input_IsPressed(
                    INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
                    INPUT_ROLE_ENTER_CONSOLE)) {
                Console_Open();
            } else {
                UI_HandleKeyDown(event.key.keysym.sym);
            }
            break;
        }

        case SDL_KEYUP:
            // NOTE: needs special handling on Windows -
            // SDL_SCANCODE_PRINTSCREEN is not sufficient to react to this.
            if (event.key.keysym.sym == SDLK_PRINTSCREEN) {
                Screenshot_Make(g_Config.rendering.screenshot_format);
            }
            break;

        case SDL_TEXTEDITING:
            UI_HandleTextEdit(event.text.text);
            break;

        case SDL_TEXTINPUT:
            UI_HandleTextEdit(event.text.text);
            break;

        case SDL_CONTROLLERDEVICEADDED:
        case SDL_JOYDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_JOYDEVICEREMOVED:
            Input_Discover();
            break;
        }
    }
}

static void M_SetGLBackend(const GFX_GL_BACKEND backend)
{
    switch (backend) {
    case GFX_GL_33C:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        break;

    case GFX_GL_INVALID_BACKEND:
        ASSERT_FAIL();
        break;
    }
}

void S_Shell_CreateWindow(void)
{
    SDL_Window *const window = SDL_CreateWindow(
        "TR1X", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_OPENGL);

    if (window == nullptr) {
        Shell_ExitSystem("System Error: cannot create window");
        return;
    }

    const GFX_GL_BACKEND backends_to_try[] = {
        // clang-format off
        GFX_GL_33C,
        GFX_GL_INVALID_BACKEND, // guard
        // clang-format on
    };

    for (int32_t i = 0; backends_to_try[i] != GFX_GL_INVALID_BACKEND; i++) {
        const GFX_GL_BACKEND backend = backends_to_try[i];

        M_SetGLBackend(backend);

        int32_t major;
        int32_t minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        LOG_DEBUG("Trying GL backend %d.%d", major, minor);
        if (GFX_Context_Attach(window, backend)) {
            m_Window = window;
            S_Shell_HandleWindowResize();
            return;
        }
    }

    Shell_ExitSystem("System Error: cannot attach opengl context");
}

SDL_Window *Shell_GetWindow(void)
{
    return m_Window;
}
