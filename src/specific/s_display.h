#ifndef T1M_SPECIFIC_S_DISPLAY_H
#define T1M_SPECIFIC_S_DISPLAY_H

#include <stdint.h>

void S_NoFade();
void S_FadeInInventory(int32_t fade);
void S_FadeOutInventory(int32_t fade);

void S_CopyBufferToScreen();

#endif
