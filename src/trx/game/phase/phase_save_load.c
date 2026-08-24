#include <trx/game/phase/phase_save_load.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/music.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>
#include <trx/game/savegame.h>
#include <trx/game/shell.h>
#include <trx/game/sound.h>
#include <trx/game/ui.h>

typedef struct {
    INVENTORY_MODE mode;
    UI_SAVE_SLOT_DIALOG_STATE *dialog;
    GAME_STRING_ID error_msg;
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

static SAVEGAME_SLOT_REF M_GetInitialSlot(void)
{
    SAVEGAME_SLOT_REF slot = SG_Manager_GetMostRecentlyUsedSlot();
    if (!SG_Manager_IsValidSlotRef(slot)) {
        slot = SG_Manager_GetMostRecentlyCreatedSlot();
    }
    if (!SG_Manager_IsValidSlotRef(slot)) {
        slot = SG_Manager_NormalSlot(0);
    }
    return slot;
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
    p->dialog = UI_SaveSlotDialog_Init(
        M_IsLoading(p) ? UI_SAVE_SLOT_DIALOG_LOAD_GAME
                       : UI_SAVE_SLOT_DIALOG_SAVE_GAME,
        M_GetInitialSlot());
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
    if (p->dialog != nullptr) {
        UI_SaveSlotDialog_Free(p->dialog);
        p->dialog = nullptr;
    }
}

static PHASE_CONTROL M_Leave(M_PRIV *const p, const GF_COMMAND gf_cmd)
{
    if (p->music_paused && gf_cmd.action == GF_NOOP) {
        Music_Unpause();
        Sound_UnpauseAll();
    }
    return (PHASE_CONTROL) { .action = PHASE_ACTION_END, .gf_cmd = gf_cmd };
}

static PHASE_CONTROL M_Confirm(M_PRIV *const p, const SAVEGAME_SLOT_REF slot)
{
    if (!M_IsLoading(p)) {
        Savegame_Save(slot);
        return M_Leave(p, (GF_COMMAND) { .action = GF_NOOP });
    }

    Inv_RemoveAllItems();
    return M_Leave(
        p,
        (GF_COMMAND) {
            .action = GF_START_SAVED_GAME,
            .param = SG_Manager_SlotToParam(slot),
        });
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    ASSERT(p->dialog != nullptr);

    Input_Update();
    Shell_ProcessInput();
    if (Shell_IsExiting()) {
        return M_Leave(p, (GF_COMMAND) { .action = GF_EXIT_GAME });
    }

    const UI_SAVE_SLOT_DIALOG_CHOICE choice =
        UI_SaveSlotDialog_Control(p->dialog);
    switch (choice.action) {
    case UI_SAVE_SLOT_DIALOG_NO_CHOICE:
        break;

    case UI_SAVE_SLOT_DIALOG_CANCEL:
        return M_Leave(p, (GF_COMMAND) { .action = GF_NOOP });

    case UI_SAVE_SLOT_DIALOG_CONFIRM:
        return M_Confirm(p, choice.slot);

    case UI_SAVE_SLOT_DIALOG_DELETE_FAILED:
        p->error_msg = GS_ID("general/passport/delete_save_failed");
        break;
    }

    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Output_Overlay_DrawBackground(
        g_Config.ui.inventory_background_style, 1.0f, nullptr);
    Output_Flush();

    UI_SaveSlotDialog(p->dialog);

    if (p->error_msg != nullptr) {
        UI_BeginModal(0.5f, 0.67f);
        UI_BeginFrame(UI_FRAME_DIALOG_BACKGROUND);
        UI_BeginPad(8.0f, 8.0f);
        UI_Label(GameString_Get(p->error_msg));
        UI_EndPad();
        UI_EndFrame();
        UI_EndModal();
    }
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
