#include "game/ui/dialogs/select_level.h"

#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/elements/anchor.h>
#include <libtrx/game/ui/elements/hide.h>
#include <libtrx/game/ui/elements/label.h>
#include <libtrx/game/ui/elements/modal.h>
#include <libtrx/game/ui/elements/offset.h>
#include <libtrx/game/ui/elements/requester.h>
#include <libtrx/game/ui/elements/resize.h>
#include <libtrx/game/ui/elements/spacer.h>
#include <libtrx/game/ui/elements/stack.h>
#include <libtrx/memory.h>
#include <libtrx/vector.h>

typedef enum {
    M_ROW_ROLE_PLAY_LEVEL,
    M_ROW_ROLE_STORY_SO_FAR,
} M_ROW_ROLE;

typedef struct {
    const char *const text;
    M_ROW_ROLE role;
} M_ROW;

typedef struct UI_SELECT_LEVEL_DIALOG_STATE {
    int32_t save_slot;
    bool is_active;
    VECTOR *rows;
    UI_REQUESTER_STATE req;
} UI_SELECT_LEVEL_DIALOG_STATE;

static int32_t M_GetVisibleRows(void);

static int32_t M_GetVisibleRows(void)
{
    const int32_t res_h = Screen_GetResHeightDownscaled(RSR_TEXT);
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

UI_SELECT_LEVEL_DIALOG_STATE *UI_SelectLevelDialog_Init(const int32_t save_slot)
{
    UI_SELECT_LEVEL_DIALOG_STATE *const s =
        Memory_Alloc(sizeof(UI_SELECT_LEVEL_DIALOG_STATE));
    s->save_slot = save_slot;
    s->rows = Vector_Create(sizeof(M_ROW));

    const SAVEGAME_INFO *const info = Savegame_GetSavegameInfo(save_slot);
    ASSERT(info != nullptr);
    if (!info->features.select_level) {
        s->is_active = false;
    } else {
        s->is_active = true;

        const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
        for (int32_t i = 0; i <= info->level_num && i < level_table->count;
             i++) {
            if (level_table->levels[i].type != GFL_GYM) {
                Vector_Add(
                    s->rows,
                    &(M_ROW) {
                        .text = level_table->levels[i].title,
                        .role = M_ROW_ROLE_PLAY_LEVEL,
                    });
            }
        }

        if (g_InvMode == INV_TITLE_MODE && GF_HasAvailableStory(save_slot)) {
            Vector_Add(
                s->rows,
                &(M_ROW) {
                    .text = GS(PASSPORT_STORY_SO_FAR),
                    .role = M_ROW_ROLE_STORY_SO_FAR,
                });
        }
    }

    UI_Requester_Init(&s->req, 0, s->rows->count, true);
    s->req.row_pad = 2.0f;
    s->req.show_arrows = true;
    s->req.reserve_space = true;
    return s;
}

void UI_SelectLevelDialog_Free(UI_SELECT_LEVEL_DIALOG_STATE *const s)
{
    Vector_Free(s->rows);
    UI_Requester_Free(&s->req);
    Memory_Free(s);
}

int32_t UI_SelectLevelDialog_Control(UI_SELECT_LEVEL_DIALOG_STATE *const s)
{
    UI_Requester_SetVisibleRows(&s->req, M_GetVisibleRows());
    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice < 0 || !s->is_active) {
        return UI_SELECT_LEVEL_CHOICE_NOOP;
    }
    const M_ROW *const row = Vector_Get(s->rows, choice);
    if (row->role == M_ROW_ROLE_STORY_SO_FAR) {
        return UI_SELECT_LEVEL_CHOICE_PLAY_STORY_SO_FAR;
    }
    return choice;
}

void UI_SelectLevelDialog(UI_SELECT_LEVEL_DIALOG_STATE *const s)
{
    UI_BeginModal(0.5f, g_InvMode == INV_TITLE_MODE ? 0.72f : 0.55f);
    UI_BeginResize(300.0f, -1.0f);
    UI_BeginRequester(&s->req, GS(PASSPORT_SELECT_LEVEL));

    const SAVEGAME_INFO *info = Savegame_GetSavegameInfo(s->save_slot);
    if (!s->is_active) {
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GS(PASSPORT_LEGACY_SELECT_LEVEL_1));
        UI_EndAnchor();
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GS(PASSPORT_LEGACY_SELECT_LEVEL_2));
        UI_EndAnchor();
    } else {
        for (int32_t i = 0; i < s->rows->count; i++) {
            if (UI_Requester_IsRowVisible(&s->req, i)) {
                const M_ROW *const row = Vector_Get(s->rows, i);
                UI_BeginRequesterRow(&s->req, i);
                if (UI_Requester_IsRowSelected(&s->req, i)) {
                    UI_BeginStackEx((UI_STACK_SETTINGS) {
                        .orientation = UI_STACK_HORIZONTAL,
                        .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE } });
                    UI_BeginOffset(0.0f, -1.0f);
                    UI_Label("\\{button left}");
                    UI_EndOffset();
                    UI_Label(row->text);
                    // balance both sides so that the row text appears centered
                    UI_BeginHide(true);
                    UI_Label("\\{button left}");
                    UI_EndHide();
                    UI_EndStack();
                } else {
                    UI_BeginAnchor(0.5f, 0.5f);
                    UI_Label(row->text);
                    UI_EndAnchor();
                }
                UI_EndRequesterRow(&s->req, i);
            }
        }
    }

    UI_EndRequester(&s->req);
    UI_EndResize();
    UI_EndModal();
}
