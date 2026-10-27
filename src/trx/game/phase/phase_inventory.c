#include <trx/game/phase/phase_inventory.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/game/camera.h>
#include <trx/game/cutseq/playback.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/game.h>
#include <trx/game/game/control.h>
#include <trx/game/game_flow.h>
#include <trx/game/gym.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara/hair.h>
#include <trx/game/lua/events.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/random.h>
#include <trx/game/savegame.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/game/ui/menu.h>
#include <trx/game/ui/takeover.h>

typedef struct {
    INVENTORY_MODE mode;
    // Whether a script took the menu, which it does unless there is nothing
    // to show, as for the keys of a lock when Lara carries none.
    bool is_shown;
    bool music_paused;
    // The title level keeps running behind the menu, with its flyby camera.
    bool live_scene;
} M_PRIV;

// A minimal simulation tick keeping the title level alive behind the menu:
// the world, the camera it plays through and its actors - no player input,
// no HUD.
static void M_SimTick(void)
{
    Interpolation_Remember();
    Game_TickBeginFrame();
    Sound_ResetAmbient();
    Game_TickWorld();
    Game_TickPostControl();
    Game_TickEndFrame();
}

// The music plays quieter behind the menu, by how loud the player wants it
// there.
static void M_AdjustMusicVolume(const M_PRIV *const p)
{
    if (p->mode == INV_TITLE_MODE) {
        Music_SetVolume(g_Config.audio.music_volume);
        return;
    }
    const bool is_ambient =
        Music_GetCurrentPlayingTrack() == Music_GetCurrentLoopedTrack();
    const double base_volume = is_ambient ? g_Config.audio.ambient_volume
                                          : g_Config.audio.music_volume;
    const double multiplier = is_ambient
        ? g_Config.audio.inventory_ambient_volume
        : g_Config.audio.inventory_music_volume;
    Music_SetVolume(base_volume * multiplier);
    Sound_ResetAmbient();
    Sound_UpdateEffects();
}

// The entry the menu opens on: the gym's on the title of a game that asks for
// it, and otherwise the one asked for, which is asked for once.
static OBJECT_ID M_TakeRequestedObject(const M_PRIV *const p)
{
    const OBJECT_ID object_id = UI_TakeRequestedItem();
    if (p->mode == INV_TITLE_MODE && GF_GetGymLevel() != nullptr
        && Gym_IsInventoryOpenEnabled()) {
        return O_PHOTO_OPTION;
    }
    return object_id;
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;

    if (p->mode == INV_TITLE_MODE) {
        Music_Stop();
        const GF_LEVEL *const level = GF_GetTitleLevel();
        if (g_Config.audio.enable_music_in_menu && level->music_track >= 0) {
            Music_PlayBySlot(level->music_track, MPM_LOOP);
        }
        SG_Manager_ScanSavedGames();
        Camera_Initialise();
        LUA_FireEvent(LUA_EVENT_TITLE_START);
        p->live_scene = g_GameFlow.main_menu_use_live_scene;
        // The draw random number generator stands still outside live play,
        // which holds every effect the scene draws on one value. A live scene
        // runs, so it advances again for as long as the menu is up.
        if (p->live_scene) {
            Random_FreezeDraw(false);
            Game_SetIsLiveScene(true);
        }
    } else {
        // The snapshot re-renders the scene, so it must happen before the
        // inventory lighting mode kicks in.
        Lara_Hair_Follow();
        Output_Overlay_CaptureGameSnapshot();
        if (!g_Config.audio.enable_music_in_inventory) {
            Music_Pause();
            Sound_PauseAll();
            p->music_paused = true;
        }
    }
    Output_SetInventoryLightingMode(true);
    Interpolation_Remember();

    p->is_shown = UI_Takeover_Offer(
        UI_TAKEOVER_MENU,
        (UI_TAKEOVER_ARGS) {
            .mode = p->mode,
            .object_id = M_TakeRequestedObject(p),
            .background = p->mode == INV_TITLE_MODE && !p->live_scene
                ? g_GameFlow.main_menu_background_path
                : nullptr,
            .is_live = p->live_scene,
            .key_id = UI_TakeRequestedKey(),
        });
    if (!p->is_shown) {
        if (p->music_paused) {
            Music_Unpause();
            Sound_UnpauseAll();
        }
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

// Ends the phase with what the menu told the game to do next, and brings the
// music back where the game resumes.
static PHASE_CONTROL M_Leave(M_PRIV *const p, GF_COMMAND gf_cmd)
{
    if (gf_cmd.action == GF_NOOP) {
        gf_cmd = GF_GetOverrideCommand();
        GF_OverrideCommand((GF_COMMAND) { .action = GF_NOOP }, true);
    }
    if (p->music_paused && gf_cmd.action == GF_NOOP) {
        Music_Unpause();
        Sound_UnpauseAll();
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_END, .gf_cmd = gf_cmd };
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->live_scene) {
        M_SimTick();
    }
    M_AdjustMusicVolume(p);

    Input_Update();
    Shell_ProcessInput();
    Game_ProcessInput();
    if (Shell_IsExiting()) {
        return M_Leave(p, (GF_COMMAND) { .action = GF_EXIT_GAME });
    }

    if (g_Config.gameplay.enable_timer_in_inventory
        && !(Game_IsInGym() && Gym_TrackManager_HasStats(GYM_TRACK_ASSAULT))) {
        Stats_UpdateTimer();
    }

    if (!UI_Takeover_IsHeld(UI_TAKEOVER_MENU)
        || UI_Takeover_TakeChoice(UI_TAKEOVER_MENU)
            != UI_TAKEOVER_CHOICE_NONE) {
        return M_Leave(p, (GF_COMMAND) { .action = GF_NOOP });
    }
    // A live scene remembers its world as each tick starts. Remembering it
    // again here would leave nothing to interpolate.
    if (!p->live_scene) {
        Interpolation_Remember();
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    UI_Takeover_Release(UI_TAKEOVER_MENU);
    Output_SetInventoryLightingMode(false);
    if (p->mode == INV_TITLE_MODE) {
        Music_Stop();
        Sound_StopAll();
    }
    // Neither a cutscene nor a flyby may outlive the menu they played behind.
    // In this order: dropping the cutscene fires its end event, and a script
    // that answers one by starting a flyby - as the TR4 title does - would
    // otherwise leave that sequence holding the camera.
    if (p->live_scene) {
        CutSeq_Reset();
        if (FlybyMode_IsActive()) {
            FlybyMode_Deactivate();
        }
        Random_FreezeDraw(true);
        Game_SetIsLiveScene(false);
    }
    if (g_Config.input.enable_buffering_inventory) {
        g_OldInputDB = (INPUT_STATE) {};
    }
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    if (p->live_scene) {
        // The inventory lighting mode is meant for the menu's models; the
        // level behind them renders with its own in-game lighting.
        Output_SetInventoryLightingMode(false);
        Game_Draw(false);
        Output_SetInventoryLightingMode(true);
    } else if (p->mode == INV_TITLE_MODE) {
        Interpolation_Interpolate();
    }
}

bool Phase_Inventory_IsShown(const PHASE *const phase)
{
    const M_PRIV *const p = phase->priv;
    return p->is_shown;
}

PHASE *Phase_Inventory_Create(const INVENTORY_MODE mode)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->mode = mode;
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    phase->defers_override = true;
    return phase;
}

void Phase_Inventory_Destroy(PHASE *const phase)
{
    Memory_Free(phase->priv);
    Memory_Free(phase);
}
