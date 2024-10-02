#include "game/inventory/vars.h"

#include "global/vars.h"

int32_t g_Inv_OptionObjectsCount = 4;

INVENTORY_ITEM *g_Inv_OptionList[] = {
    &g_Inv_Item_Passport,
    &g_Inv_Item_Controls,
    &g_Inv_Item_Sound,
    &g_Inv_Item_Photo,
};
