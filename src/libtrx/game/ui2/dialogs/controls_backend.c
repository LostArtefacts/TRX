#include "game/ui2/dialogs/controls_backend.h"

#include "game/game_string.h"
#include "game/input.h"
#include "game/ui2/elements/anchor.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/modal.h"
#include "game/ui2/elements/requester.h"

static const GAME_STRING_ID m_Options[] = {
    GS_ID(CONTROLS_BACKEND_KEYBOARD),
    GS_ID(CONTROLS_BACKEND_CONTROLLER),
    nullptr,
};

void UI2_ControlsBackend_Init(UI2_CONTROLS_BACKEND_STATE *const s)
{
    int32_t count = 0;
    for (count = 0; m_Options[count] != nullptr; count++) { }
    UI2_Requester_Init(&s->req, count, count, true);
}

void UI2_ControlsBackend_Free(UI2_CONTROLS_BACKEND_STATE *const s)
{
    UI2_Requester_Free(&s->req);
}

int32_t UI2_ControlsBackend_Control(UI2_CONTROLS_BACKEND_STATE *const s)
{
    const int32_t choice = UI2_Requester_Control(&s->req);
    switch (choice) {
    case 0:
        return INPUT_BACKEND_KEYBOARD;
    case 1:
        return INPUT_BACKEND_CONTROLLER;
    default:
        return choice;
    }
}

void UI2_ControlsBackend(UI2_CONTROLS_BACKEND_STATE *const s)
{
    UI2_BeginModal(0.5f, 2.0f / 3.0f);
    UI2_BeginRequester(&s->req, GS(CONTROLS_CUSTOMIZE));

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
