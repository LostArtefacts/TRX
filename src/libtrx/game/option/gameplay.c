#include "game/option/gameplay.h"

#include "game/input.h"
#include "game/ui/dialogs/gameplay_settings.h"

typedef struct {
    UI_SETTINGS_STATE *ui_state;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *p);
static void M_Shutdown(M_PRIV *p);

static void M_Init(M_PRIV *const p)
{
    if (p->ui_state == nullptr) {
        p->ui_state = UI_GameplaySettings_Init();
    }
}

static void M_Shutdown(M_PRIV *const p)
{
    if (p->ui_state != nullptr) {
        UI_GameplaySettings_Free(p->ui_state);
        p->ui_state = nullptr;
    }
}

void Option_Gameplay_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }
    if (p->ui_state == nullptr) {
        M_Init(p);
    }
    if (UI_GameplaySettings_Control(p->ui_state)) {
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
    if (p->ui_state != nullptr) {
        UI_GameplaySettings(p->ui_state);
    }
}

void Option_Gameplay_Close(void)
{
}

void Option_Gameplay_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_Shutdown(p);
}
