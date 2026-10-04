#include <trx/game/phase/phase_save_load.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/game/game.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>
#include <trx/game/savegame.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/ui/dialogs/takeover.h>

typedef struct {
    INVENTORY_MODE mode;
    bool music_paused;
} M_PRIV;

static INVENTORY_MODE M_ResolveMode(const INVENTORY_MODE mode)
{
    if (mode == INV_LOAD_MODE && SG_Manager_GetTotalCount() == 0) {
        return INV_SAVE_MODE;
    }
    return mode;
}

static bool M_IsLoading(const M_PRIV *const p)
{
    return p->mode == INV_LOAD_MODE;
}

static void M_SetTitle(const M_PRIV *const p)
{
    Overlay_SetBottomText((OVERLAY_TEXT) {
        .kind = OVERLAY_TEXT_GS_KEY,
        .gs_key = M_IsLoading(p) ? GS_ID("general/passport/load_game")
                                 : GS_ID("general/passport/save_game"),
        .fmt_gs_key = GS_ID("general/inventory_ring/object_name_fmt"),
    });
}

static PHASE_CONTROL M_Start(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;

    // The player asked for the slots, not for the ring, so no key that was
    // already down reaches them.
    Input_HoldOffMenu();

    if (!g_Config.audio.enable_music_in_inventory) {
        Music_Pause();
        Sound_PauseAll();
        p->music_paused = true;
    }

    Output_Overlay_CaptureGameSnapshot();
    M_SetTitle(p);
    UI_Takeover_Offer(UI_TAKEOVER_SAVE_LOAD, p->mode);
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
    UI_Takeover_Release(UI_TAKEOVER_SAVE_LOAD);
}

static PHASE_CONTROL M_Leave(M_PRIV *const p, const GF_COMMAND gf_cmd)
{
    if (p->music_paused && gf_cmd.action == GF_NOOP) {
        Music_Unpause();
        Sound_UnpauseAll();
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_END, .gf_cmd = gf_cmd };
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;

    Input_Update();
    Shell_ProcessInput();
    if (Shell_IsExiting()) {
        return M_Leave(p, (GF_COMMAND) { .action = GF_EXIT_GAME });
    }

    if (!UI_Takeover_IsHeld(UI_TAKEOVER_SAVE_LOAD)
        || UI_Takeover_TakeChoice(UI_TAKEOVER_SAVE_LOAD)
            != UI_TAKEOVER_CHOICE_NONE) {
        return M_Leave(p, (GF_COMMAND) { .action = GF_NOOP });
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
    Output_Overlay_DrawBackground(
        g_Config.ui.inventory_background_style, 1.0f, nullptr);
    Output_Flush();
}

bool Phase_SaveLoad_IsAvailable(const INVENTORY_MODE mode)
{
    if (!g_Config.ui.instant_save_load_screen
        || g_Config.flow.load_save_disabled) {
        return false;
    }

    switch (M_ResolveMode(mode)) {
    case INV_SAVE_MODE:
        return !Game_IsInGym() && Savegame_IsManualSaveAllowed();

    case INV_LOAD_MODE:
        return SG_Manager_GetTotalCount() > 0;

    case INV_SAVE_CRYSTAL_MODE:
        return !Game_IsInGym();

    default:
        return false;
    }
}

PHASE *Phase_SaveLoad_Create(const INVENTORY_MODE mode)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    M_PRIV *const p = Memory_Alloc(sizeof(M_PRIV));
    p->mode = M_ResolveMode(mode);
    phase->priv = p;
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_SaveLoad_Destroy(PHASE *const phase)
{
    Memory_Free(phase->priv);
    Memory_Free(phase);
}
