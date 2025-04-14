#include "game/input.h"
#include "game/option/option.h"
#include "game/text.h"
#include "game/ui/widgets/graphics_dialog.h"
#include "global/vars.h"

static struct {
    UI_WIDGET *widget;
} m_Priv = {};

static void M_ConstructUI(void);
static void M_DestroyUI(void);

static void M_ConstructUI(void)
{
    m_Priv.widget = UI_GraphicsDialog_Create();
}

static void M_DestroyUI(void)
{
    if (m_Priv.widget != nullptr) {
        m_Priv.widget->free(m_Priv.widget);
        m_Priv.widget = nullptr;
    }
}

void Option_Detail_Control(INVENTORY_ITEM *const item, const bool is_busy)
{
    if (m_Priv.widget == nullptr) {
        M_ConstructUI();
    }
    m_Priv.widget->control(m_Priv.widget);
}

void Option_Detail_Draw(INVENTORY_ITEM *const item)
{
    if (m_Priv.widget != nullptr) {
        m_Priv.widget->draw(m_Priv.widget);
    }
}

void Option_Detail_Shutdown(void)
{
    M_DestroyUI();
}
