#include <trx/game/option/save_crystal.h>

#include <trx/config.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/vars.h>
#include <trx/game/objects/ids.h>
#include <trx/game/savegame.h>
#include <trx/game/ui.h>

static struct {
    UI_SAVE_SLOT_DIALOG_STATE *save_slot;
    SAVEGAME_SLOT_REF chosen_slot;
    GAME_STRING_ID error_msg;
} m_Priv = {};

static bool M_IsUsable(void)
{
    return g_Config.gameplay.save_crystal_mode == SAVE_CRYSTAL_SAVE_PICKUP
        && g_InvRing_Mode == INV_GAME_MODE;
}

static SAVEGAME_SLOT_REF M_GetInitialSlot(void)
{
    SAVEGAME_SLOT_REF slot = Savegame_GetMostRecentlyUsedSlot();
    if (!Savegame_IsValidSlotRef(slot)) {
        slot = Savegame_GetMostRecentlyCreatedSlot();
    }
    if (!Savegame_IsValidSlotRef(slot)) {
        slot = Savegame_NormalSlot(0);
    }
    return slot;
}

void Option_SaveCrystal_Control(
    INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    if (is_busy || !M_IsUsable()) {
        return;
    }

    if (m_Priv.save_slot == nullptr) {
        m_Priv.chosen_slot = Savegame_InvalidSlot();
        m_Priv.save_slot = UI_SaveSlotDialog_Init(
            UI_SAVE_SLOT_DIALOG_SAVE_GAME, M_GetInitialSlot());
    }

    const UI_SAVE_SLOT_DIALOG_CHOICE choice =
        UI_SaveSlotDialog_Control(m_Priv.save_slot);
    switch (choice.action) {
    case UI_SAVE_SLOT_DIALOG_NO_CHOICE:
        // Make sure it's not possible to confirm empty slots
        g_Input.menu_confirm = false;
        g_InputDB.menu_confirm = false;
        break;

    case UI_SAVE_SLOT_DIALOG_CANCEL:
        // The ring deselects the crystal on the same input, and nothing is
        // spent.
        break;

    case UI_SAVE_SLOT_DIALOG_CONFIRM:
        // The ring closes on the same input; the save happens once it has.
        m_Priv.chosen_slot = choice.slot;
        break;

    case UI_SAVE_SLOT_DIALOG_DELETE_FAILED:
        m_Priv.error_msg = GS_ID("general/passport/delete_save_failed");
        break;
    }
}

void Option_SaveCrystal_Draw(void)
{
    if (m_Priv.save_slot != nullptr) {
        UI_SaveSlotDialog(m_Priv.save_slot);
    }

    if (m_Priv.error_msg != nullptr) {
        UI_BeginModal(0.5f, 0.67f);
        UI_BeginFrame(UI_FRAME_DIALOG_BACKGROUND);
        UI_BeginPad(8.0f, 8.0f);
        UI_Label(GameString_Get(m_Priv.error_msg));
        UI_EndPad();
        UI_EndFrame();
        UI_EndModal();
    }
}

void Option_SaveCrystal_Close(void)
{
    if (m_Priv.save_slot != nullptr) {
        UI_SaveSlotDialog_Free(m_Priv.save_slot);
        m_Priv.save_slot = nullptr;
    }
    m_Priv.error_msg = nullptr;
}

void Option_SaveCrystal_CommitSave(void)
{
    if (!M_IsUsable() || !Savegame_IsValidSlotRef(m_Priv.chosen_slot)) {
        return;
    }

    // The crystal is spent first so that the save reflects it.
    const SAVEGAME_SLOT_REF slot = m_Priv.chosen_slot;
    m_Priv.chosen_slot = Savegame_InvalidSlot();
    Inv_RemoveItem(O_SAVE_CRYSTAL_ITEM);
    if (!Savegame_Save(slot)) {
        Inv_AddItem(O_SAVE_CRYSTAL_ITEM);
    }
}
