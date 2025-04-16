#include "game/ui/elements/flash.h"

#include "game/ui/elements/hide.h"
#include "game/ui/helpers.h"

static void M_Draw(const UI_NODE *node);

static const UI_WIDGET_OPS m_Ops = {
    .measure = UI_MeasureWrapper,
    .layout = UI_LayoutWrapper,
    .draw = M_Draw,
};

static void M_Draw(const UI_NODE *const node)
{
    UI_FLASH_STATE *const s = *(UI_FLASH_STATE **)node->data;
    UI_BeginHide(s->count >= 0);
    UI_DrawWrapper(node);
    UI_EndHide();
}

void UI_Flash_Init(UI_FLASH_STATE *const s, const int32_t rate)
{
    s->count = 0;
    s->rate = rate;
}

void UI_Flash_Free(UI_FLASH_STATE *const s)
{
}

void UI_Flash_Control(UI_FLASH_STATE *const s)
{
    s->count -= 2;
    if (s->count < -s->rate) {
        s->count = s->rate;
    }
}

void UI_BeginFlash(UI_FLASH_STATE *const s)
{
    UI_NODE *const node = UI_AllocNode(&m_Ops, sizeof(UI_FLASH_STATE *));
    *(UI_FLASH_STATE **)node->data = s;
    UI_AddChild(node);
    UI_PushCurrent(node);
}

void UI_EndFlash(void)
{
    UI_PopCurrent();
}
