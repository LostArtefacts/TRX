#include <trx/game/ui/dialogs/takeover.h>

typedef struct {
    bool held;
    UI_TAKEOVER_CHOICE choice;
} M_STATE;

static const bool
    m_Accepts[UI_TAKEOVER_NUMBER_OF][UI_TAKEOVER_CHOICE_NUMBER_OF] = {
        [UI_TAKEOVER_RING_ENTRY] = {
            [UI_TAKEOVER_CHOICE_CANCEL] = true,
            [UI_TAKEOVER_CHOICE_CONFIRM] = true,
        },
        [UI_TAKEOVER_SAVE_LOAD] = { [UI_TAKEOVER_CHOICE_CANCEL] = true },
    };

static UI_TAKEOVER_HOOKS m_Hooks = {};
static M_STATE m_State[UI_TAKEOVER_NUMBER_OF] = {};

void UI_Takeover_SetHooks(const UI_TAKEOVER_HOOKS hooks)
{
    m_Hooks = hooks;
}

bool UI_Takeover_Offer(const UI_TAKEOVER screen, const int32_t arg)
{
    M_STATE *const state = &m_State[screen];
    state->choice = UI_TAKEOVER_CHOICE_NONE;
    state->held = m_Hooks.offer != nullptr && m_Hooks.offer(screen, arg);
    return state->held;
}

bool UI_Takeover_IsHeld(const UI_TAKEOVER screen)
{
    return m_State[screen].held;
}

bool UI_Takeover_IsAnyHeld(void)
{
    for (int32_t i = 0; i < UI_TAKEOVER_NUMBER_OF; i++) {
        if (m_State[i].held) {
            return true;
        }
    }
    return false;
}

UI_TAKEOVER_CHOICE UI_Takeover_TakeChoice(const UI_TAKEOVER screen)
{
    M_STATE *const state = &m_State[screen];
    const UI_TAKEOVER_CHOICE choice = state->choice;
    if (choice != UI_TAKEOVER_CHOICE_NONE) {
        state->held = false;
        state->choice = UI_TAKEOVER_CHOICE_NONE;
    }
    return choice;
}

void UI_Takeover_Release(const UI_TAKEOVER screen)
{
    M_STATE *const state = &m_State[screen];
    if (!state->held) {
        return;
    }
    state->held = false;
    if (state->choice == UI_TAKEOVER_CHOICE_NONE
        && m_Hooks.release != nullptr) {
        m_Hooks.release(screen);
    }
    state->choice = UI_TAKEOVER_CHOICE_NONE;
}

bool UI_Takeover_AcceptsChoice(
    const UI_TAKEOVER screen, const UI_TAKEOVER_CHOICE choice)
{
    return m_Accepts[screen][choice];
}

void UI_Takeover_Close(
    const UI_TAKEOVER screen, const UI_TAKEOVER_CHOICE choice)
{
    M_STATE *const state = &m_State[screen];
    if (state->held) {
        state->choice = choice;
    }
}
