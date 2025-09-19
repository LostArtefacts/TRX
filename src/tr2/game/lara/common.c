#include <libtrx/game/lara/common.h>

static LARA_INFO m_Lara = {};
static ITEM *m_LaraItem = nullptr;

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &m_Lara;
}

ITEM *Lara_GetItem(void)
{
    return m_LaraItem;
}

void Lara_InitialiseLoad(int16_t item_num)
{
    m_Lara.item_num = item_num;
    if (item_num == NO_ITEM) {
        m_LaraItem = nullptr;
    } else {
        m_LaraItem = Item_Get(item_num);
    }
}
