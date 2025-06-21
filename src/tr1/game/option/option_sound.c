#include "game/option/option_sound.h"

#include <libtrx/config.h>
#include <libtrx/game/input.h>
#include <libtrx/game/ui.h>

typedef struct {
    UI_SOUND_SETTINGS_STATE *ui;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *p);
static void M_Close(M_PRIV *p);

static void M_Init(M_PRIV *const p)
{
    p->ui = UI_SoundSettings_Init();
}

static void M_Close(M_PRIV *const p)
{
    if (p->ui != nullptr) {
        UI_SoundSettings_Free(p->ui);
        p->ui = nullptr;
    }
}

void Option_Sound_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }
    if (p->ui == nullptr) {
        M_Init(p);
    }
    if (UI_SoundSettings_Control(p->ui)) {
        M_Close(p);
    } else {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

void Option_Sound_Draw(INVENTORY_ITEM *const inv_item)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui != nullptr) {
        UI_SoundSettings(p->ui);
    }
}

void Option_Sound_Close(void)
{
    M_PRIV *const p = &m_Priv;
    M_Close(p);
}
