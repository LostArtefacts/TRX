#include <trx/game/ui/dialogs/save_slot.h>

#include <trx/game/game_string.h>
#include <trx/game/input.h>
#include <trx/game/savegame.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/dialogs/base_passport.h>
#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/elements/hide.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/elements/offset.h>
#include <trx/game/ui/elements/requester.h>
#include <trx/game/ui/elements/spacer.h>
#include <trx/game/ui/elements/stack.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/viewport.h>
#include <trx/memory.h>
#include <trx/utils.h>
#include <trx/version.h>

#define M_IMMEDIATE (g_TRVersion >= 2)

typedef struct UI_SAVE_SLOT_DIALOG_STATE {
    UI_SAVE_SLOT_DIALOG_TYPE type;
    UI_REQUESTER_STATE req;
} UI_SAVE_SLOT_DIALOG_STATE;

static void M_NonEmptySlot(
    const UI_SAVE_SLOT_DIALOG_STATE *const s, const int32_t slot_idx,
    const SAVEGAME_INFO *const info)
{
    if (g_TRVersion == 1) {
        UI_BeginAnchor(0.5f, 0.5f);
        UI_BeginStack(UI_STACK_HORIZONTAL);
    } else {
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
    }

    // Level title with the save counter
    UI_Label(info->level_title);
    if (info->counter > 0) {
        UI_Spacer(8.0f, 0.0f);
        UI_LabelFmt("%d", info->counter);
    }

    UI_EndStack();
    if (g_TRVersion == 1) {
        UI_EndAnchor();
    }
}

static void M_EmptySlot(
    const UI_SAVE_SLOT_DIALOG_STATE *const s, const int32_t slot_idx)
{
    UI_BeginAnchor(0.5f, 0.5f);
    UI_LabelFmt(GS(MISC_EMPTY_SLOT_FMT), slot_idx + 1);
    UI_EndAnchor();
}

UI_SAVE_SLOT_DIALOG_STATE *UI_SaveSlotDialog_Init(
    const UI_SAVE_SLOT_DIALOG_TYPE type, const int32_t save_slot)
{
    UI_SAVE_SLOT_DIALOG_STATE *const s =
        Memory_Alloc(sizeof(UI_SAVE_SLOT_DIALOG_STATE));
    s->type = type;

    UI_BasePassportDialog_Init(&s->req, Savegame_GetSlotCount());
    UI_Requester_SelectRow(&s->req, save_slot);
    return s;
}

void UI_SaveSlotDialog_Free(UI_SAVE_SLOT_DIALOG_STATE *const s)
{
    UI_Requester_Free(&s->req);
    Memory_Free(s);
}

UI_SAVE_SLOT_DIALOG_CHOICE UI_SaveSlotDialog_Control(
    UI_SAVE_SLOT_DIALOG_STATE *const s)
{
    UI_BasePassportDialog_Control(&s->req);
    const int32_t sel_row = UI_Requester_GetCurrentRow(&s->req);
    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice == UI_REQUESTER_CANCEL) {
        return (UI_SAVE_SLOT_DIALOG_CHOICE) {
            .action = UI_SAVE_SLOT_DIALOG_CANCEL,
        };
    } else if (
        choice != UI_REQUESTER_NO_CHOICE
        && (s->type == UI_SAVE_SLOT_DIALOG_SAVE_GAME
            || !Savegame_IsSlotFree(choice))) {
        return (UI_SAVE_SLOT_DIALOG_CHOICE) {
            .action = UI_SAVE_SLOT_DIALOG_CONFIRM,
            .slot_num = sel_row,
        };
    }
    return (UI_SAVE_SLOT_DIALOG_CHOICE) {
        .action = UI_SAVE_SLOT_DIALOG_NO_CHOICE,
    };
}

void UI_SaveSlotDialog(const UI_SAVE_SLOT_DIALOG_STATE *const s)
{
    UI_BeginBasePassportDialog();
    const char *title = nullptr;
    switch (s->type) {
    case UI_SAVE_SLOT_DIALOG_SAVE_GAME:
        title = GS(PASSPORT_SAVE_GAME);
        break;
    case UI_SAVE_SLOT_DIALOG_LOAD_GAME:
        title = GS(PASSPORT_LOAD_GAME);
        break;
    case UI_SAVE_SLOT_DIALOG_GENERIC:
        title = GS(PASSPORT_SELECT_SAVE);
        break;
    }
    UI_BeginRequester(&s->req, title);

    const int32_t first = UI_Requester_GetFirstRow(&s->req);
    const int32_t last = UI_Requester_GetLastRow(&s->req);
    for (int32_t i = first; i < last; ++i) {
        UI_BeginRequesterRow(&s->req, i);
        const SAVEGAME_INFO *const info = Savegame_GetSavegameInfo(i);
        if (info != nullptr && info->level_title != nullptr) {
            M_NonEmptySlot(s, i, info);
        } else {
            M_EmptySlot(s, i);
        }
        UI_EndRequesterRow(&s->req, i);
    }

    UI_EndRequester(&s->req);
    UI_EndBasePassportDialog();
}
