#ifndef T1M_SPECIFIC_S_MISC_H
#define T1M_SPECIFIC_S_MISC_H

#include "global/types.h"

// TODO: these do not belong to specific/ and are badly named

void S_FadeInInventory(int32_t fade);
void S_FadeOutInventory(int32_t fade);
void S_FinishInventory();

RGB888 S_ColourFromPalette(int8_t idx);

int S_GetObjectBounds(int16_t *bptr);

#endif
