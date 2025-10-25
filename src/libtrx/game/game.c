#include "game/game.h"

#include "config.h"
#include "game/camera.h"
#include "game/clock.h"
#include "game/demo.h"
#include "game/effects.h"
#include "game/fmv.h"
#include "game/gym.h"
#include "game/interpolation.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/option/passport.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "game/ui.h"
#include "version.h"

#define M_FRAME_BUFFER(key)                                                    \
    do {                                                                       \
        Shell_ProcessEvents();                                                 \
        Output_BeginScene();                                                   \
        Game_Draw(true);                                                       \
        Input_Update();                                                        \
        Output_EndScene();                                                     \
        Output_FlipScreen();                                                   \
        Clock_WaitTick();                                                      \
    } while (g_Input.key);

int32_t g_OverlayFlag = 0;

static bool m_IsPlaying = false;
static const GF_LEVEL *m_CurrentLevel = nullptr;
static GAME_BONUS_FLAG m_BonusFlag = GBF_NONE;
static bool m_IsLevelComplete = false;
static FADER m_Fader;

void Game_SetIsPlaying(const bool is_playing)
{
    m_IsPlaying = is_playing;
    Random_FreezeDraw(!is_playing);
}

bool Game_IsPlaying(void)
{
    return m_IsPlaying;
}

const GF_LEVEL *Game_GetCurrentLevel(void)
{
    return m_CurrentLevel;
}

void Game_SetCurrentLevel(const GF_LEVEL *const level)
{
    m_CurrentLevel = level;
}

bool Game_IsInGym(void)
{
    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    return current_level != nullptr && current_level->type == GFL_GYM;
}

bool Game_IsLoaded(void)
{
    if (FMV_IsPlaying()) {
        return false;
    }
    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    if (current_level == nullptr || current_level->type == GFL_TITLE) {
        return false;
    }
    return true;
}

bool Game_IsPlayable(void)
{
    if (FMV_IsPlaying()) {
        return false;
    }

    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    if (current_level == nullptr || current_level->type == GFL_TITLE
        || current_level->type == GFL_DEMO
        || current_level->type == GFL_CUTSCENE) {
        return false;
    }

    if (!Object_Get(O_LARA)->loaded || Lara_GetItem() == nullptr
        || !Lara_IsControllable()) {
        return false;
    }

    return true;
}

GAME_BONUS_FLAG Game_GetBonusFlag(void)
{
    return m_BonusFlag;
}

void Game_SetBonusFlag(const GAME_BONUS_FLAG flag)
{
    m_BonusFlag = flag;
}

bool Game_IsBonusFlagSet(const GAME_BONUS_FLAG flag)
{
    return (m_BonusFlag & flag) != 0;
}

void Game_SetIsLevelComplete(const bool is_complete)
{
    m_IsLevelComplete = is_complete;
}

bool Game_IsLevelComplete(void)
{
    return m_IsLevelComplete;
}

void Game_FadeToBlack(const int32_t duration)
{
    Fader_Init(
        &m_Fader, FADER_TRANSPARENT, FADER_BLACK, duration / (float)LOGIC_FPS);
}

void Game_DrawFade(void)
{
    UI_BeginFade(&m_Fader, true);
    UI_EndFade();
}

bool Game_Start(const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    Game_SetCurrentLevel(level);
#if TR_VERSION == 1
    Game_FadeToBlack(-1);
#endif

    g_OverlayFlag = 1;
    Camera_Initialise();
    Interpolation_Remember();

    Sound_StopAll();
    const bool is_cutscene = level->type == GFL_CUTSCENE;
    if (level->music_track != MX_INACTIVE
        && (is_cutscene || Music_GetCurrentLoopedTrack() == MX_INACTIVE)) {
        Music_Play_Direct(
            level->music_track, is_cutscene ? MPM_ALWAYS : MPM_LOOPED);
    }

    return true;
}

void Game_End(void)
{
    Savegame_PersistGameToCurrentInfo(Game_GetCurrentLevel());
    Music_Stop();
}

GF_COMMAND Game_Control(const bool demo_mode)
{
    if (g_Passport.ask_for_save) {
        // ask for a save at the start of a level for the save crystals mode
        const GF_COMMAND gf_cmd = GF_ShowInventory(INV_SAVE_CRYSTAL_MODE);
        g_Passport.ask_for_save = false;
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    Interpolation_Remember();
    if (!Game_IsInGym() || Gym_IsAssaultTimerActive()
        || !Object_Get(O_ASSAULT_DIGITS)->loaded) {
        Stats_UpdateTimer();
    }
    if (g_Config.flow.cheat_keys) {
        Lara_Cheat_CheckKeys();
    }

    if (Game_IsLevelComplete()) {
        Sound_StopAll();
        Music_Stop();
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    }

    Input_Update();
    Shell_ProcessInput();
    Game_ProcessInput();

    if (g_InputDB.toggle_photo_mode) {
        return GF_EnterPhotoMode();
    } else if (g_InputDB.pause && lara->death_timer == 0) {
        return GF_PauseGame();
    }

#if TR_VERSION == 2
    if (demo_mode) {
        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
        if (!Demo_GetInput()) {
            g_Input = (INPUT_STATE) {};
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
    }
#endif

    if (lara->death_timer > DEATH_WAIT
        || (lara->death_timer > DEATH_WAIT_INPUT
            && (g_InputDB.menu_confirm || g_InputDB.menu_back)
            && !g_Input.fly_cheat)
        || g_OverlayFlag == 2) {
        if (demo_mode || (g_TRVersion == 2 && Game_IsInGym())) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
        if (g_OverlayFlag == 2) {
            g_OverlayFlag = 1;
            return GF_ShowInventory(INV_DEATH_MODE);
        } else {
            g_OverlayFlag = 2;
        }
    }

    if ((g_InputDB.option || g_InputDB.load || g_InputDB.save
         || g_OverlayFlag <= 0)
        && lara->death_timer == 0 && !lara->extra_anim) {
        if (g_TRVersion == 1 && g_Camera.type == CAM_CINEMATIC) {
            g_OverlayFlag = 0;
        } else if (g_OverlayFlag > 0) {
            if (g_Config.flow.lockout_option_ring
                && g_Config.flow.load_save_disabled) {
                g_OverlayFlag = 0;
            } else if (g_Input.save) {
                g_OverlayFlag = -2;
            } else if (g_Input.load) {
                g_OverlayFlag = -1;
            } else {
                g_OverlayFlag = 0;
            }
        } else {
            GF_COMMAND gf_cmd;
            if (g_OverlayFlag == -1) {
                gf_cmd = GF_ShowInventory(INV_LOAD_MODE);
            } else if (g_OverlayFlag == -2) {
                gf_cmd = GF_ShowInventory(INV_SAVE_MODE);
            } else {
                gf_cmd = GF_ShowInventory(INV_GAME_MODE);
            }
            g_OverlayFlag = 1;
            if (gf_cmd.action != GF_NOOP) {
                return gf_cmd;
            }
        }
    }

    Output_ResetDynamicLights();

    Sound_ResetAmbient();
    Item_Control();
    Effect_Control();

    Lara_Control();
    Lara_Hair_Control(false);

    Camera_Update();
    ItemAction_RunActive();
    Sound_UpdateEffects();
    Overlay_Animate(1);
    Output_AnimateTextures(1);
    return (GF_COMMAND) { .action = GF_NOOP };
}

void Game_ProcessInput(void)
{
    if (GF_GetCurrentLevel()->type == GFL_DEMO) {
        return;
    }

    if (g_InputDB.use_small_medi && Inv_RequestItem(O_SMALL_MEDIPACK_OPTION)) {
        Lara_UseItem(O_SMALL_MEDIPACK_OPTION);
    }
    if (g_InputDB.use_big_medi && Inv_RequestItem(O_LARGE_MEDIPACK_OPTION)) {
        Lara_UseItem(O_LARGE_MEDIPACK_OPTION);
    }

#if TR_VERSION == 1
    if (g_Config.input.enable_buffering && Game_IsPlaying()) {
        if (g_Input.toggle_bilinear_filter) {
            M_FRAME_BUFFER(toggle_bilinear_filter);
        } else if (g_Input.toggle_trapezoid_filter) {
            M_FRAME_BUFFER(toggle_trapezoid_filter);
        } else if (g_Input.toggle_fps_counter) {
            M_FRAME_BUFFER(toggle_fps_counter);
        }
    }
#endif

    if (g_InputDB.toggle_ui) {
        UI_ToggleState(&g_Config.ui.enable_game_ui);
    }
}
