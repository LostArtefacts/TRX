#include "game/ui/dialogs/new_game.h"

#include "game/game_string.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/requester.h"

static const GAME_STRING_ID m_Options[] = {
    GS_ID(PASSPORT_MODE_NEW_GAME),
    GS_ID(PASSPORT_MODE_NEW_GAME_PLUS),
    GS_ID(PASSPORT_MODE_NEW_GAME_JP),
    GS_ID(PASSPORT_MODE_NEW_GAME_JP_PLUS),
    nullptr,
};

void UI_NewGame_Init(UI_NEW_GAME_STATE *const s)
{
    int32_t option_count = 0;
    for (int32_t i = 0; m_Options[i] != nullptr; i++) {
        option_count++;
    }

    UI_Requester_Init(&s->req, option_count, option_count, true);
}

void UI_NewGame_Free(UI_NEW_GAME_STATE *const s)
{
    UI_Requester_Free(&s->req);
}

int32_t UI_NewGame_Control(UI_NEW_GAME_STATE *const s)
{
    return UI_Requester_Control(&s->req);
}

void UI_NewGame(UI_NEW_GAME_STATE *const s)
{
    UI_BeginModal(0.5f, 2.0f / 3.0f);
    UI_BeginRequester(&s->req, GS(PASSPORT_SELECT_MODE));

    for (int32_t i = UI_Requester_GetFirstRow(&s->req);
         i < UI_Requester_GetLastRow(&s->req); i++) {
        UI_BeginRequesterRow(&s->req, i);
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GameString_Get(m_Options[i]));
        UI_EndAnchor();
        UI_EndRequesterRow(&s->req, i);
    }

    UI_EndRequester(&s->req);
    UI_EndModal();
}
