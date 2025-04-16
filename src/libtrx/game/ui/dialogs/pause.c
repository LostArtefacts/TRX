#include "game/ui/dialogs/pause.h"

#include "game/game_string.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/requester.h"

static const GAME_STRING_ID m_Options[2][2] = {
    { GS_ID(PAUSE_CONTINUE), GS_ID(PAUSE_QUIT) },
    { GS_ID(PAUSE_YES), GS_ID(PAUSE_NO) },
};

void UI_Pause_Init(UI_PAUSE_STATE *const s)
{
    s->phase = 0;
    UI_Requester_Init(&s->req, 2, 2, true);
}

void UI_Pause_Free(UI_PAUSE_STATE *const s)
{
    UI_Requester_Free(&s->req);
}

UI_PAUSE_EXIT_CHOICE UI_Pause_Control(UI_PAUSE_STATE *const s)
{
    const int32_t choice = UI_Requester_Control(&s->req);
    if (s->phase == 0) {
        if (choice == UI_REQUESTER_CANCEL) {
            return UI_PAUSE_RESUME_PAUSE;
        } else if (choice == 0) {
            return UI_PAUSE_EXIT_TO_GAME;
        } else if (choice == 1) {
            s->phase = 1;
            UI_Requester_Free(&s->req);
            UI_Requester_Init(&s->req, 2, 2, true);
        }
    } else {
        if (choice == UI_REQUESTER_CANCEL) {
            s->phase = 0;
        } else if (choice == 0) {
            return UI_PAUSE_EXIT_TO_TITLE;
        } else if (choice == 1) {
            return UI_PAUSE_EXIT_TO_GAME;
        }
    }
    return UI_PAUSE_NOOP;
}

void UI_Pause(UI_PAUSE_STATE *const s)
{
    UI_BeginModal(0.5f, 1.0f);
    UI_BeginPad(50.0f, 50.0f);
    UI_BeginRequester(
        &s->req,
        s->phase == 0 ? GS(PAUSE_EXIT_TO_TITLE) : GS(PAUSE_ARE_YOU_SURE));

    for (int32_t i = UI_Requester_GetFirstRow(&s->req);
         i < UI_Requester_GetLastRow(&s->req); i++) {
        UI_BeginRequesterRow(&s->req, i);
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GameString_Get(m_Options[s->phase][i]));
        UI_EndAnchor();
        UI_EndRequesterRow(&s->req, i);
    }

    UI_EndRequester(&s->req);
    UI_EndPad();
    UI_EndModal();
}
