#ifndef T1M_GAME_DINO_H
#define T1M_GAME_DINO_H

#include <stdint.h>

void RaptorControl(int16_t item_num);
void DinoControl(int16_t item_num);
void LaraDinoDeath(ITEM_INFO* item);

void T1MInjectGameDino();

#endif
