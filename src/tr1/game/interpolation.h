#pragma once

#include "global/types.h"

#include <stdbool.h>

bool Interpolation_IsEnabled(void);
void Interpolation_Disable(void);
void Interpolation_Enable(void);

double Interpolation_GetRate(void);
void Interpolation_SetRate(double rate);

void Interpolation_Commit(void);
void Interpolation_Remember(void);
void Interpolation_RememberItem(ITEM *item);
