#pragma once

#include <trx/game/items/types.h>

void Interpolation_Enable(void);
void Interpolation_Disable(void);
bool Interpolation_IsEnabled(void);
bool Interpolation_IsActive(void);

double Interpolation_GetWorldRate(void);
double Interpolation_GetCameraRate(void);
double Interpolation_GetRate(void);
void Interpolation_SetRate(double rate);

void Interpolation_Interpolate(void);
void Interpolation_Remember(void);
void Interpolation_RememberItem(ITEM *item);

// Instantly discard interpolation data
void Interpolation_CommitLara(void);
void Interpolation_CommitBraid(void);
