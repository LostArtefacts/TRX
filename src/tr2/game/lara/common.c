#include "global/vars.h"

#include <libtrx/game/lara/common.h>

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &g_Lara;
}

ITEM *Lara_GetItem(void)
{
    return g_LaraItem;
}
