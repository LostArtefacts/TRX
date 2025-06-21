#include "game/option/option.h"

#include <libtrx/game/input.h>
#include <libtrx/game/ui/dialogs/gameplay_settings.h>

typedef struct {
    struct {
        bool is_ready;
        UI_GAMEPLAY_SETTINGS_STATE state;
    } ui;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *p);
static void M_Close(M_PRIV *p);

static void M_Init(M_PRIV *const p)
{
    p->ui.is_ready = true;
    UI_GameplaySettings_Init(&p->ui.state);
}

static void M_Close(M_PRIV *const p)
{
    if (p->ui.is_ready) {
        p->ui.is_ready = false;
        UI_GameplaySettings_Free(&p->ui.state);
    }
}

void Option_Gameplay_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }
    if (!p->ui.is_ready) {
        M_Init(p);
    }
    if (UI_GameplaySettings_Control(&p->ui.state)) {
        M_Close(p);
        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            inv_item->anim_direction = 1;
            inv_item->goal_frame = inv_item->frames_total - 1;
        }
    } else {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

void Option_Gameplay_Draw(INVENTORY_ITEM *const inv_item)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui.is_ready) {
        UI_GameplaySettings(&p->ui.state);
    }
}

void Option_Gameplay_Close(void)
{
    M_PRIV *const p = &m_Priv;
    M_Close(p);
}
