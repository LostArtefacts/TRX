#include "inv.h"

#include "global/vars.h"

int16_t InvKeysCurrent;
int16_t InvKeysObjects;
int16_t InvKeysQtys[24] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

INVENTORY_ITEM *InvKeysList[23] = {
    &InvItemLeadBar,
    &InvItemPuzzle1,
    &InvItemPuzzle2,
    &InvItemPuzzle3,
    &InvItemPuzzle4,
    &InvItemKey1,
    &InvItemKey2,
    &InvItemKey3,
    &InvItemKey4,
    &InvItemPickup1,
    &InvItemPickup2,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

int16_t InvMainCurrent;
int16_t InvMainObjects = 1;
int16_t InvMainQtys[24] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

INVENTORY_ITEM *InvMainList[23] = {
    &InvItemCompass,
    &InvItemPistols,
    &InvItemShotgun,
    &InvItemMagnum,
    &InvItemUzi,
    &InvItemGrenade,
    &InvItemBigMedi,
    &InvItemMedi,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

int16_t InvOptionCurrent;
int16_t InvOptionObjects = 5;
INVENTORY_ITEM *InvOptionList[5] = {
    &InvItemGame,    &InvItemControls,  &InvItemSound,
    &InvItemDetails, &InvItemLarasHome,
};
