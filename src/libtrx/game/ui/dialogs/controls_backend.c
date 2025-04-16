#include "game/ui/dialogs/controls_backend.h"

#include "game/game_string.h"
#include "game/input.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/requester.h"

static const GAME_STRING_ID m_Options[] = {
    GS_ID(CONTROLS_BACKEND_KEYBOARD),
    GS_ID(CONTROLS_BACKEND_CONTROLLER),
    nullptr,
};

void UI_ControlsBackend_Init(UI_CONTROLS_BACKEND_STATE *const s)
{
    int32_t count = 0;
    for (count = 0; m_Options[count] != nullptr; count++) { }
    UI_Requester_Init(&s->req, count, count, true);
}

void UI_ControlsBackend_Free(UI_CONTROLS_BACKEND_STATE *const s)
{
    UI_Requester_Free(&s->req);
}

int32_t UI_ControlsBackend_Control(UI_CONTROLS_BACKEND_STATE *const s)
{
    const int32_t choice = UI_Requester_Control(&s->req);
    switch (choice) {
    case 0:
        return INPUT_BACKEND_KEYBOARD;
    case 1:
        return INPUT_BACKEND_CONTROLLER;
    default:
        return choice;
    }
}

void UI_ControlsBackend(UI_CONTROLS_BACKEND_STATE *const s)
{
    UI_BeginModal(0.5f, 2.0f / 3.0f);
    UI_BeginRequester(&s->req, GS(CONTROLS_CUSTOMIZE));

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
