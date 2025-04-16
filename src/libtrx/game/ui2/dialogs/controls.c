#include "game/ui2/dialogs/controls.h"

#include "config.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/ui2/dialogs/controls_backend.h"
#include "game/ui2/dialogs/controls_editor.h"
#include "game/ui2/elements/requester.h"

typedef enum {
    M_PHASE_BACKEND,
    M_PHASE_EDITOR,
} M_PHASE;

void UI2_Controls_Init(UI2_CONTROLS_STATE *const s)
{
    s->events = EventManager_Create();
    s->phase = M_PHASE_BACKEND;
    s->backend = INPUT_BACKEND_KEYBOARD;
    s->active_layout = g_Config.input.keyboard_layout;
    UI2_ControlsBackend_Init(&s->backend_state);
    UI2_ControlsEditor_Init(&s->editor_state, s->events);
}

void UI2_Controls_Free(UI2_CONTROLS_STATE *const s)
{
    UI2_ControlsEditor_Free(&s->editor_state);
    UI2_ControlsBackend_Free(&s->backend_state);
    EventManager_Free(s->events);
    s->events = nullptr;
}

bool UI2_Controls_Control(UI2_CONTROLS_STATE *const s)
{
    switch (s->phase) {
    case M_PHASE_BACKEND: {
        const int32_t choice = UI2_ControlsBackend_Control(&s->backend_state);
        switch (choice) {
        case UI2_REQUESTER_NO_CHOICE:
            return false;
        case UI2_REQUESTER_CANCEL:
            return true;
        case INPUT_BACKEND_KEYBOARD:
            s->backend = choice;
            s->phase = M_PHASE_EDITOR;
            UI2_ControlsEditor_Reinit(
                &s->editor_state, choice, g_Config.input.keyboard_layout);
            break;
        case INPUT_BACKEND_CONTROLLER:
            s->backend = choice;
            s->phase = M_PHASE_EDITOR;
            UI2_ControlsEditor_Reinit(
                &s->editor_state, choice, g_Config.input.controller_layout);
            break;
        }
        break;
    }

    case M_PHASE_EDITOR: {
        const UI2_CONTROLS_CHOICE choice =
            UI2_ControlsEditor_Control(&s->editor_state);
        switch (choice) {
        case UI2_CONTROLS_CHOICE_NOOP:
            break;
        case UI2_CONTROLS_CHOICE_GO_BACK:
            s->phase = M_PHASE_BACKEND;
            break;
        case UI2_CONTROLS_CHOICE_EXIT:
            return true;
        }
        break;
    }
    }

    return false;
}

void UI2_Controls(UI2_CONTROLS_STATE *const s)
{
    switch (s->phase) {
    case M_PHASE_BACKEND:
        UI2_ControlsBackend(&s->backend_state);
        break;
    case M_PHASE_EDITOR:
        UI2_ControlsEditor(&s->editor_state);
        break;
    }
}
