#pragma once

#include <trx/core/colors.h>

// Washes the screen with a colour that fades out over 16 ticks, as TR4 does
// for a lens flare, a flash grenade and some traps and enemies. A new flash
// replaces the one under way.
void Output_Flash_Trigger(RGB_888 color);

void Output_Flash_Reset(void);
void Output_Flash_Update(void);
void Output_Flash_Draw(void);
