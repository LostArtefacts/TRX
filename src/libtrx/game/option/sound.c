#include "game/option/sound.h"

#include "config.h"
#include "game/input.h"
#include "game/ui.h"

typedef struct {
    UI_SETTINGS_STATE *ui_state;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *const p)
{
    if (p->ui_state == nullptr) {
        p->ui_state = UI_SoundSettings_Init();
    }
}

static void M_Shutdown(M_PRIV *const p)
{
    if (p->ui_state != nullptr) {
        UI_SoundSettings_Free(p->ui_state);
        p->ui_state = nullptr;
    }
}

void Option_Sound_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }
    if (p->ui_state == nullptr) {
        M_Init(p);
    }
    if (!UI_SoundSettings_Control(p->ui_state)) {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

void Option_Sound_Draw(INVENTORY_ITEM *const inv_item)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui_state != nullptr) {
        UI_SoundSettings(p->ui_state);
    }
}

void Option_Sound_Close(void)
{
}

void Option_Sound_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_Shutdown(p);
}
