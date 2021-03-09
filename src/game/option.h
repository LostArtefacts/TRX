#ifndef T1M_GAME_OPTION_H
#define T1M_GAME_OPTION_H

#include "game/types.h"

void DoInventoryOptions(INVENTORY_ITEM *inv_item);
void DoPassportOption(INVENTORY_ITEM *inv_item);
void DoGammaOption(INVENTORY_ITEM *inv_item);
void DoCompassOption(INVENTORY_ITEM *inv_item);
void DoDetailOptionHW(INVENTORY_ITEM *inv_item);
void DoDetailOption(INVENTORY_ITEM *inv_item);
void FlashConflicts();
void DefaultConflict();
void DoControlOption(INVENTORY_ITEM *inv_item);
void DoSoundOption(INVENTORY_ITEM *inv_item);
void S_ShowControls();
void S_ChangeCtrlText();
void S_RemoveCtrlText();
void S_RemoveCtrl();

void InitRequester(REQUEST_INFO *req);
void RemoveRequester(REQUEST_INFO *req);
int32_t DisplayRequester(REQUEST_INFO *req);

void T1MInjectGameOption();

#endif
