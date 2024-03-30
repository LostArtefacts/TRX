#pragma once

#include "global/types.h"

bool Interpolation_IsEnabled(void);
void Interpolation_Disable(void);
void Interpolation_Enable(void);
void Interpolation_Commit(void);
void Interpolation_Remember(void);
void Interpolation_RememberItem(ITEM_INFO *item);
