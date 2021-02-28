#ifndef T1M_GAME_OPTION_H
#define T1M_GAME_OPTION_H

#include "game/types.h"

// clang-format off
#define do_inventory_options        ((void          (*)(INVENTORY_ITEM* inv_item))0x0042D770)
// clang-format on

void S_ShowControls();

void T1MInjectGameOption();

#endif
