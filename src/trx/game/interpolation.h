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

// Returns how far the frame being drawn stands between the last two ticks of
// the interface. This matches the world rate, except while the game exits:
// the world then stands still and only the interface moves.
double Interpolation_GetUIRate(void);
// Sets the interface rate on its own. Interpolation_SetRate sets both.
void Interpolation_SetUIRate(double rate);

void Interpolation_Interpolate(void);
void Interpolation_Remember(void);
void Interpolation_RememberItem(ITEM *item);
void Interpolation_RememberBraid(void);

// Instantly discard interpolation data
void Interpolation_CommitLara(void);
void Interpolation_CommitBraid(void);
