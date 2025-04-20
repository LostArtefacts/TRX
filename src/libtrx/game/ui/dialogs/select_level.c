#include "game/ui/dialogs/select_level.h"

#include "debug.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "game/ui/common.h"
#include "game/ui/dialogs/base_passport.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/hide.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/offset.h"
#include "game/ui/elements/requester.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "memory.h"
#include "vector.h"

// TODO: consolidate this variable
#if TR_VERSION == 1
extern int32_t g_InvMode;
#else
extern int32_t g_Inv_Mode;
#endif

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

UI_SELECT_LEVEL_DIALOG_STATE *UI_SelectLevelDialog_Init(const int32_t save_slot)
{
    UI_SELECT_LEVEL_DIALOG_STATE *const s =
        Memory_Alloc(sizeof(UI_SELECT_LEVEL_DIALOG_STATE));
    s->save_slot = save_slot;
    s->rows = Vector_Create(sizeof(M_ROW));

    const SAVEGAME_INFO *const info = Savegame_GetSavegameInfo(save_slot);
    ASSERT(info != nullptr);
    s->is_active = info->features.select_level;

    if (s->is_active) {
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

#if TR_VERSION == 1
        const INVENTORY_MODE inv_mode = g_InvMode;
#else
        const INVENTORY_MODE inv_mode = g_Inv_Mode;
#endif
        if (inv_mode == INV_TITLE_MODE && GF_HasAvailableStory(save_slot)) {
            Vector_Add(
                s->rows,
                &(M_ROW) {
                    .text = GS(PASSPORT_STORY_SO_FAR),
                    .role = M_ROW_ROLE_STORY_SO_FAR,
                });
        }
    }

    UI_BasePassportDialog_Init(&s->req, s->rows->count);
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
    UI_BasePassportDialog_Control(&s->req);
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
    UI_BeginBasePassportDialog();
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
    UI_EndBasePassportDialog();
}
