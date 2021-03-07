#ifndef T1M_GAME_OPTION_H
#define T1M_GAME_OPTION_H

#include "game/types.h"

// clang-format off
#define DoDetailOption      ((void      (*)(INVENTORY_ITEM* inv_item))0x0042E2D0)
#define DoSoundOption       ((void      (*)(INVENTORY_ITEM* inv_item))0x0042E5C0)
#define DoControlOption     ((void      (*)(INVENTORY_ITEM* inv_item))0x0042EAC0)
#define DisplayRequester    ((int32_t   (*)(REQUEST_INFO* req))0x0042F6F0)
// clang-format on

void DoInventoryOptions(INVENTORY_ITEM *inv_item);
void DoPassportOption(INVENTORY_ITEM *inv_item);
void DoGammaOption(INVENTORY_ITEM *inv_item);
void DoCompassOption(INVENTORY_ITEM *inv_item);
void S_ShowControls();
void InitRequester(REQUEST_INFO *req);

void T1MInjectGameOption();

#endif
