#include "game/ui/dialogs/select_level.h"

#include "debug.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/inventory.h"
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

typedef struct {
    const char *const text;
} M_ROW;

typedef struct UI_SELECT_LEVEL_DIALOG_STATE {
    int32_t save_slot;
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
    ASSERT(info->features.select_level);

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i <= info->level_num && i < level_table->count; i++) {
        if (level_table->levels[i].type != GFL_GYM) {
            Vector_Add(
                s->rows, &(M_ROW) { .text = level_table->levels[i].title });
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
    return UI_Requester_Control(&s->req);
}

void UI_SelectLevelDialog(UI_SELECT_LEVEL_DIALOG_STATE *const s)
{
    UI_BeginBasePassportDialog();
    UI_BeginRequester(&s->req, GS(PASSPORT_SELECT_LEVEL));

    const SAVEGAME_INFO *info = Savegame_GetSavegameInfo(s->save_slot);
    for (int32_t i = 0; i < s->rows->count; i++) {
        if (UI_Requester_IsRowVisible(&s->req, i)) {
            const M_ROW *const row = Vector_Get(s->rows, i);
            UI_BeginRequesterRow(&s->req, i);
            UI_BeginAnchor(0.5f, 0.5f);
            UI_Label(row->text);
            UI_EndAnchor();
            UI_EndRequesterRow(&s->req, i);
        }
    }

    UI_EndRequester(&s->req);
    UI_EndBasePassportDialog();
}
