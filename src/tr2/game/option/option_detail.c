#include "game/input.h"
#include "game/option/option.h"
#include "game/text.h"
#include "game/ui2/dialogs/graphic_settings.h"
#include "global/vars.h"

typedef struct {
    struct {
        bool is_ready;
        UI2_GRAPHIC_SETTINGS_STATE state;
    } ui;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *p);
static void M_Shutdown(M_PRIV *p);

static void M_Init(M_PRIV *const p)
{
    p->ui.is_ready = true;
    UI2_GraphicSettings_Init(&p->ui.state);
}

static void M_Shutdown(M_PRIV *const p)
{
    if (p->ui.is_ready) {
        p->ui.is_ready = false;
        UI2_GraphicSettings_Free(&p->ui.state);
    }
}

void Option_Detail_Control(INVENTORY_ITEM *const item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }
    if (!p->ui.is_ready) {
        M_Init(p);
    }
    UI2_GraphicSettings_Control(&p->ui.state);
}

void Option_Detail_Draw(INVENTORY_ITEM *const item)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui.is_ready) {
        UI2_GraphicSettings(&p->ui.state);
    }
}

void Option_Detail_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_Shutdown(p);
}
