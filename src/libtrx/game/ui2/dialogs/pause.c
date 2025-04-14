#include "game/ui2/dialogs/pause.h"

#include "game/game_string.h"
#include "game/ui2/elements/anchor.h"
#include "game/ui2/elements/frame.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/modal.h"
#include "game/ui2/elements/pad.h"
#include "game/ui2/elements/requester.h"

static const GAME_STRING_ID m_Options[2][2] = {
    { GS_ID(PAUSE_CONTINUE), GS_ID(PAUSE_QUIT) },
    { GS_ID(PAUSE_YES), GS_ID(PAUSE_NO) },
};

void UI2_Pause_Init(UI2_PAUSE_STATE *const s)
{
    s->phase = 0;
    UI2_Requester_Init(&s->req, 2, 2, true);
}

void UI2_Pause_Free(UI2_PAUSE_STATE *const s)
{
    UI2_Requester_Free(&s->req);
}

UI2_PAUSE_EXIT_CHOICE UI2_Pause_Control(UI2_PAUSE_STATE *const s)
{
    const int32_t choice = UI2_Requester_Control(&s->req);
    if (s->phase == 0) {
        if (choice == UI2_REQUESTER_CANCEL) {
            return UI2_PAUSE_RESUME_PAUSE;
        } else if (choice == 0) {
            return UI2_PAUSE_EXIT_TO_GAME;
        } else if (choice == 1) {
            s->phase = 1;
            UI2_Requester_Free(&s->req);
            UI2_Requester_Init(&s->req, 2, 2, true);
        }
    } else {
        if (choice == UI2_REQUESTER_CANCEL) {
            s->phase = 0;
        } else if (choice == 0) {
            return UI2_PAUSE_EXIT_TO_TITLE;
        } else if (choice == 1) {
            return UI2_PAUSE_EXIT_TO_GAME;
        }
    }
    return UI2_PAUSE_NOOP;
}

void UI2_Pause(UI2_PAUSE_STATE *const s)
{
    UI2_BeginModal(0.5f, 1.0f);
    UI2_BeginPad(50.0f, 50.0f);
    UI2_BeginRequester(
        &s->req,
        s->phase == 0 ? GS(PAUSE_EXIT_TO_TITLE) : GS(PAUSE_ARE_YOU_SURE));

    for (int32_t i = UI2_Requester_GetFirstRow(&s->req);
         i < UI2_Requester_GetLastRow(&s->req); i++) {
        UI2_BeginRequesterRow(&s->req, i);
        UI2_BeginAnchor(0.5f, 0.5f);
        UI2_Label(GameString_Get(m_Options[s->phase][i]));
        UI2_EndAnchor();
        UI2_EndRequesterRow(&s->req, i);
    }

    UI2_EndRequester(&s->req);
    UI2_EndPad();
    UI2_EndModal();
}
