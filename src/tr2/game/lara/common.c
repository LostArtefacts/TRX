#include "global/vars.h"

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &g_Lara;
}

ITEM *Lara_GetItem(void)
{
    return g_LaraItem;
}

void Lara_InitialiseLoad(const int16_t item_num)
{
    g_Lara.item_num = item_num;
    g_LaraItem = Item_Get(item_num);
}
