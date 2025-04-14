#include "game/ui2/dialogs/new_game.h"

#include "game/game_string.h"
#include "game/ui2/elements/anchor.h"
#include "game/ui2/elements/frame.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/modal.h"
#include "game/ui2/elements/pad.h"
#include "game/ui2/elements/requester.h"

static const GAME_STRING_ID m_Options[] = {
    GS_ID(PASSPORT_MODE_NEW_GAME),
    GS_ID(PASSPORT_MODE_NEW_GAME_PLUS),
    GS_ID(PASSPORT_MODE_NEW_GAME_JP),
    GS_ID(PASSPORT_MODE_NEW_GAME_JP_PLUS),
    nullptr,
};

void UI2_NewGame_Init(UI2_NEW_GAME_STATE *const s)
{
    int32_t option_count = 0;
    for (int32_t i = 0; m_Options[i] != nullptr; i++) {
        option_count++;
    }

    UI2_Requester_Init(&s->req, option_count, option_count, true);
}

void UI2_NewGame_Free(UI2_NEW_GAME_STATE *const s)
{
    UI2_Requester_Free(&s->req);
}

int32_t UI2_NewGame_Control(UI2_NEW_GAME_STATE *const s)
{
    return UI2_Requester_Control(&s->req);
}

void UI2_NewGame(UI2_NEW_GAME_STATE *const s)
{
    UI2_BeginModal(0.5f, 2.0f / 3.0f);
    UI2_BeginRequester(&s->req, GS(PASSPORT_SELECT_MODE));

    for (int32_t i = UI2_Requester_GetFirstRow(&s->req);
         i < UI2_Requester_GetLastRow(&s->req); i++) {
        UI2_BeginRequesterRow(&s->req, i);
        UI2_BeginAnchor(0.5f, 0.5f);
        UI2_Label(GameString_Get(m_Options[i]));
        UI2_EndAnchor();
        UI2_EndRequesterRow(&s->req, i);
    }

    UI2_EndRequester(&s->req);
    UI2_EndModal();
}
