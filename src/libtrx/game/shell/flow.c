#include "config.h"
#include "enum_map.h"
#include "game/clock.h"
#include "game/console.h"
#include "game/fmv.h"
#include "game/game_buf.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/game_string_manager.h"
#include "game/items.h"
#include "game/lara/pose.h"
#include "game/lua.h"
#include "game/music.h"
#include "game/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/shell/platform.h"
#include "game/sound.h"
#include "log.h"

#include <stdio.h>

static void M_SetupSDL(void);
static void M_SetupGL(void);
static void M_ShowFatalError(const char *message);
static void M_LoadConfig(void);

static void M_SetupSDL(void)
{
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
        Shell_ExitSystemFmt("Cannot initialize SDL: %s", SDL_GetError());
    }
}

void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    const CONFIG *const old = &g_Config;
    const CONFIG *const new = &g_SavedConfig;
    Shell_HandleConfigChange(old, new);
}

static void M_SetupGL(void)
{
    // Setup minimum properties of GL context
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

void Shell_LoadConfig(void)
{
    Config_Read();
    Config_SubscribeChanges(M_HandleConfigChange, nullptr);

    Sound_SetMasterVolume(g_Config.audio.sound_volume);
    Music_SetVolume(g_Config.audio.music_volume);
}

void Shell_CommonInit(void)
{
    Shell_SetupHiDPI();
    Shell_SetupLibAV();
    M_SetupSDL();
    M_SetupGL();

    GameString_Init();
    GameStringManager_Init();
    EnumMap_Init();
    Config_Init();
    UI_Init();
    Console_Init();
    Overlay_Init();

    Input_Init();
    Sound_Init();
    Music_Init();

    GameBuf_Init();
    Random_Seed();
    Lara_Pose_Init();

    Shell_LoadConfig();
    Clock_Init();
    LUA_Init();
}

void Shell_ShutdownCommonModules(void)
{
    Lara_Pose_Shutdown();
    Item_ShutdownWalkables();

    Console_Shutdown();
    Savegame_Shutdown();

    GF_Shutdown();
    LUA_Shutdown();
    Overlay_Shutdown();
    Option_Shutdown();
    Output_Shutdown();

    Input_Shutdown();
    Music_Shutdown();
    Sound_Shutdown();
    UI_Shutdown();

    GameStringManager_Shutdown();
    GameString_Shutdown();
    GameBuf_Shutdown();

    Config_Shutdown();
    EnumMap_Shutdown();
    Log_Shutdown();
}
