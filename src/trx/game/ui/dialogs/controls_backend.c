#include <trx/game/ui/dialogs/controls_backend.h>

#include <trx/config.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input.h>
#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/elements/modal.h>
#include <trx/game/ui/elements/requester.h>

typedef struct {
    GAME_STRING_ID gs_id;
    INPUT_BACKEND backend;
    bool requires_touch;
} M_OPTION;

static const M_OPTION m_Options[] = {
    { .gs_id = GS_ID("general/settings/controls/backend/keyboard"),
      .backend = INPUT_BACKEND_KEYBOARD },
    { .gs_id = GS_ID("general/settings/controls/backend/controller"),
      .backend = INPUT_BACKEND_CONTROLLER },
    { .gs_id = GS_ID("general/settings/controls/backend/touch"),
      .backend = INPUT_BACKEND_TOUCH,
      .requires_touch = true },
    { .gs_id = nullptr },
};

void UI_ControlsBackend_Init(UI_CONTROLS_BACKEND_STATE *const s)
{
    int32_t sel_row = -1;
    s->visible_count = 0;
    for (int32_t i = 0; m_Options[i].gs_id != nullptr; i++) {
        if (m_Options[i].requires_touch
            && !g_Config.input.enable_touch_controls) {
            continue;
        }
        if (!Input_IsBackendEnabled(m_Options[i].backend)) {
            continue;
        }
        if (m_Options[i].backend == g_Config.input.backend) {
            sel_row = s->visible_count;
        }
        s->visible_indices[s->visible_count] = i;
        s->visible_count++;
    }
    UI_Requester_Init(&s->req, s->visible_count, s->visible_count, true);
    if (sel_row != -1) {
        UI_Requester_SelectRow(&s->req, sel_row);
    }
}

void UI_ControlsBackend_Free(UI_CONTROLS_BACKEND_STATE *const s)
{
    UI_Requester_Free(&s->req);
}

int32_t UI_ControlsBackend_Control(UI_CONTROLS_BACKEND_STATE *const s)
{
    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice >= 0) {
        return m_Options[s->visible_indices[choice]].backend;
    }
    return choice;
}

int32_t UI_ControlsBackend_GetSoleOption(
    const UI_CONTROLS_BACKEND_STATE *const s)
{
    if (s->visible_count != 1) {
        return -1;
    }
    return m_Options[s->visible_indices[0]].backend;
}

void UI_ControlsBackend(UI_CONTROLS_BACKEND_STATE *const s)
{
    UI_BeginModal(0.5f, 2.0f / 3.0f);
    UI_BeginRequester(&s->req, GS("general/settings/controls/customize"));

    for (int32_t i = UI_Requester_GetFirstRow(&s->req);
         i < UI_Requester_GetLastRow(&s->req); i++) {
        UI_BeginRequesterRow(&s->req, i);
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GameString_Get(m_Options[s->visible_indices[i]].gs_id));
        UI_EndAnchor();
        UI_EndRequesterRow(&s->req, i);
    }

    UI_EndRequester(&s->req);
    UI_EndModal();
}
