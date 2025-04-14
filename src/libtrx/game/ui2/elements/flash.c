#include "game/ui2/elements/flash.h"

#include "game/ui2/elements/hide.h"
#include "game/ui2/helpers.h"

static void M_Draw(const UI2_NODE *node);

static const UI2_WIDGET_OPS m_Ops = {
    .measure = UI2_MeasureWrapper,
    .layout = UI2_LayoutWrapper,
    .draw = M_Draw,
};

static void M_Draw(const UI2_NODE *const node)
{
    UI2_FLASH_STATE *const s = *(UI2_FLASH_STATE **)node->data;
    UI2_BeginHide(s->count >= 0);
    UI2_DrawWrapper(node);
    UI2_EndHide();
}

void UI2_Flash_Init(UI2_FLASH_STATE *const s, const int32_t rate)
{
    s->count = 0;
    s->rate = rate;
}

void UI2_Flash_Free(UI2_FLASH_STATE *const s)
{
}

void UI2_Flash_Control(UI2_FLASH_STATE *const s)
{
    s->count -= 2;
    if (s->count < -s->rate) {
        s->count = s->rate;
    }
}

void UI2_BeginFlash(UI2_FLASH_STATE *const s)
{
    UI2_NODE *const node = UI2_AllocNode(&m_Ops, sizeof(UI2_FLASH_STATE *));
    *(UI2_FLASH_STATE **)node->data = s;
    UI2_AddChild(node);
    UI2_PushCurrent(node);
}

void UI2_EndFlash(void)
{
    UI2_PopCurrent();
}
