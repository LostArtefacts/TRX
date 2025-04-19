#include "game/ui/dialogs/save_slot.h"

#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/savegame.h"
#include "game/scaler.h"
#include "game/ui/common.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/hide.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/offset.h"
#include "game/ui/elements/requester.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "game/viewport.h"
#include "memory.h"
#include "utils.h"

#include <stdio.h>

// TODO: consolidate this variable
#if TR_VERSION == 1
extern int32_t g_InvMode;
#else
extern int32_t g_Inv_Mode;
#endif

typedef struct UI_SAVE_SLOT_DIALOG_STATE {
    UI_SAVE_SLOT_DIALOG_TYPE type;
    UI_REQUESTER_STATE req;
} UI_SAVE_SLOT_DIALOG_STATE;

static int32_t M_GetVisibleRows(void);
static bool M_ShowDetails(const UI_SAVE_SLOT_DIALOG_STATE *s, int32_t slot_idx);
static void M_NonEmptySlot(
    const UI_SAVE_SLOT_DIALOG_STATE *s, int32_t slot_idx,
    const SAVEGAME_INFO *info);
static void M_EmptySlot(const UI_SAVE_SLOT_DIALOG_STATE *s, int32_t slot_idx);

static int32_t M_GetVisibleRows(void)
{
    if (TR_VERSION == 2) {
        return 10;
    } else {
        const int32_t res_h =
            Scaler_CalcInverse(Viewport_GetHeight(), SCALER_TARGET_TEXT);
        if (res_h <= 240) {
            return 5;
        } else if (res_h <= 384) {
            return 7;
        } else if (res_h <= 480) {
            return 10;
        } else {
            return 12;
        }
    }
}

static bool M_ShowDetails(
    const UI_SAVE_SLOT_DIALOG_STATE *const s, const int32_t slot_idx)
{
    if (s->type != UI_SAVE_SLOT_DIALOG_LOAD_GAME) {
        return false;
    }
    if (!UI_Requester_IsRowSelected(&s->req, slot_idx)) {
        return false;
    }
    if (TR_VERSION == 2) {
        return false;
    }
    return !Savegame_IsSlotFree(slot_idx);
}

static void M_NonEmptySlot(
    const UI_SAVE_SLOT_DIALOG_STATE *const s, const int32_t slot_idx,
    const SAVEGAME_INFO *const info)
{
    const bool show_details = M_ShowDetails(s, slot_idx);

    if (show_details) {
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        // Balance both sides so that the row text appears centered
        UI_BeginHide(true);
        UI_Label("\\{button right}");
        UI_EndHide();
        if (TR_VERSION == 1) {
            UI_BeginStack(UI_STACK_HORIZONTAL);
        }
    } else {
        if (TR_VERSION == 1) {
            UI_BeginAnchor(0.5f, 0.5f);
            UI_BeginStack(UI_STACK_HORIZONTAL);
        } else {
            UI_BeginStackEx((UI_STACK_SETTINGS) {
                .orientation = UI_STACK_HORIZONTAL,
                .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
            });
        }
    }

    // Level title with the save counter
    UI_Label(info->level_title);
    if (info->counter > 0) {
        UI_Spacer(8.0f, 0.0f);
        char buf[16];
        sprintf(buf, "%d", info->counter);
        UI_Label(buf);
    }

    if (show_details) {
        if (TR_VERSION == 1) {
            UI_EndStack();
        }
        UI_BeginOffset(0.0f, -1.0f);
        UI_Label("\\{button right}");
        UI_EndOffset();
        UI_EndStack();
    } else {
        UI_EndStack();
        if (TR_VERSION == 1) {
            UI_EndAnchor();
        }
    }
}

static void M_EmptySlot(
    const UI_SAVE_SLOT_DIALOG_STATE *const s, const int32_t slot_idx)
{
    char buf[16];
    sprintf(buf, GS(MISC_EMPTY_SLOT_FMT), slot_idx + 1);
    UI_BeginAnchor(0.5f, 0.5f);
    UI_Label(buf);
    UI_EndAnchor();
}

UI_SAVE_SLOT_DIALOG_STATE *UI_SaveSlotDialog_Init(
    const UI_SAVE_SLOT_DIALOG_TYPE type, const int32_t save_slot)
{
    UI_SAVE_SLOT_DIALOG_STATE *const s =
        Memory_Alloc(sizeof(UI_SAVE_SLOT_DIALOG_STATE));
    s->type = type;

    UI_Requester_Init(
        &s->req, M_GetVisibleRows(), Savegame_GetSlotCount(), true);
    s->req.row_pad = 2.0f;
    s->req.row_spacing = TR_VERSION == 1 ? 2.0f : 3.0f;
    s->req.show_arrows = TR_VERSION == 1;
    s->req.reserve_space = true;
    s->req.sel_row = save_slot;
    CLAMP(s->req.sel_row, 0, s->req.max_rows);
    return s;
}

void UI_SaveSlotDialog_Free(UI_SAVE_SLOT_DIALOG_STATE *const s)
{
    UI_Requester_Free(&s->req);
}

UI_SAVE_SLOT_DIALOG_CHOICE UI_SaveSlotDialog_Control(
    UI_SAVE_SLOT_DIALOG_STATE *const s)
{
    UI_Requester_SetVisibleRows(&s->req, M_GetVisibleRows());
    const int32_t sel_row = UI_Requester_GetCurrentRow(&s->req);
    if (M_ShowDetails(s, sel_row) && g_InputDB.menu_right) {
        return (UI_SAVE_SLOT_DIALOG_CHOICE) {
            .action = UI_SAVE_SLOT_DIALOG_DETAILS,
            .slot_num = sel_row,
        };
    }
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
#if TR_VERSION == 1
    const float modal_y = g_InvMode == INV_TITLE_MODE ? 0.72f : 0.55f;
#else
    const float modal_y = g_Inv_Mode == INV_TITLE_MODE ? 0.8f : 0.65f;
#endif
    UI_BeginModal(0.5f, modal_y);
    UI_BeginResize(300.0f, -1.0f);

    const char *const title = (s->type == UI_SAVE_SLOT_DIALOG_SAVE_GAME)
        ? GS(PASSPORT_SAVE_GAME)
        : GS(PASSPORT_LOAD_GAME);
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
    UI_EndResize();
    UI_EndModal();
}
