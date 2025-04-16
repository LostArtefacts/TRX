#include "game/option/option.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/ui2.h>

typedef struct {
    int32_t listeners[2];
    struct {
        bool is_ready;
        UI2_CONTROLS_STATE state;
    } ui;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *p);
static void M_Shutdown(M_PRIV *p);
static void M_HandleLayoutChange(const EVENT *event, void *user_data);
static void M_HandleKeyChange(const EVENT *event, void *user_data);

static void M_Init(M_PRIV *const p)
{
    UI2_Controls_Init(&p->ui.state);
    p->ui.is_ready = true;
    p->listeners[0] = EventManager_Subscribe(
        p->ui.state.events, "layout_change", nullptr, M_HandleLayoutChange, p);
    p->listeners[1] = EventManager_Subscribe(
        p->ui.state.events, "key_change", nullptr, M_HandleKeyChange, p);
}

static void M_Shutdown(M_PRIV *const p)
{
    if (p->ui.is_ready) {
        EventManager_Unsubscribe(p->ui.state.events, p->listeners[0]);
        EventManager_Unsubscribe(p->ui.state.events, p->listeners[1]);
        UI2_Controls_Free(&p->ui.state);
        p->ui.is_ready = false;
    }
}

static void M_HandleLayoutChange(const EVENT *event, void *user_data)
{
    const M_PRIV *const p = user_data;
    switch (p->ui.state.backend) {
    case INPUT_BACKEND_KEYBOARD:
        g_Config.input.keyboard_layout = p->ui.state.active_layout;
        break;
    case INPUT_BACKEND_CONTROLLER:
        g_Config.input.controller_layout = p->ui.state.active_layout;
        break;
    default:
        break;
    }
    Config_Write();
}

static void M_HandleKeyChange(const EVENT *event, void *user_data)
{
    Config_Write();
}

void Option_Controls_Shutdown(void)
{
    M_Shutdown(&m_Priv);
}

void Option_Controls_Control(INVENTORY_ITEM *const item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }

    if (!p->ui.is_ready) {
        M_Init(p);
    }

    if (UI2_Controls_Control(&p->ui.state)) {
        M_Shutdown(p);
    } else {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

void Option_Controls_Draw(INVENTORY_ITEM *const item)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui.is_ready) {
        UI2_Controls(&p->ui.state);
    }
}
